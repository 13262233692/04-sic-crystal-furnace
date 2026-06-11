#include "sic/view_factor.h"
#include "sic/geometry.h"
#include "sic/gauss_quadrature.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace sic {

ViewFactorCalculator::ViewFactorCalculator(std::shared_ptr<Mesh2D> mesh)
    : mesh_(mesh), quad_order_(4) {
}

void ViewFactorCalculator::extract_radiation_edges() {
    rad_edges_.clear();
    
    index_t edge_id = 0;
    for (index_t e = 0; e < mesh_->num_edges(); ++e) {
        const Edge& edge = mesh_->edge(e);
        
        if (!edge.is_exposed) continue;
        
        const Point2D& p1 = mesh_->node(edge[0]);
        const Point2D& p2 = mesh_->node(edge[1]);
        
        RadiationEdge re;
        re.v1 = edge[0];
        re.v2 = edge[1];
        re.midpoint = Point2D(0.5 * (p1.r + p2.r), 0.5 * (p1.z + p2.z));
        re.length = geometry::edge_length(p1, p2);
        re.r_avg = 0.5 * (p1.r + p2.r);
        re.area = 2.0 * M_PI * re.r_avg * re.length;
        re.id = edge_id;
        re.boundary_id = edge.boundary_id;
        
        Point2D tangent = p2 - p1;
        real_t len = re.length;
        if (len > 1e-30) {
            re.normal.r = tangent.z / len;
            re.normal.z = -tangent.r / len;
        } else {
            re.normal.r = 1.0;
            re.normal.z = 0.0;
        }
        
        rad_edges_.push_back(re);
        edge_id++;
    }
    
    index_t n = static_cast<index_t>(rad_edges_.size());
    F_.resize(n, n);
    areas_.resize(n);
    
    for (index_t i = 0; i < n; ++i) {
        areas_[i] = rad_edges_[i].area;
    }
    
    build_acceleration_structure();
}

void ViewFactorCalculator::build_acceleration_structure() {
    index_t n = static_cast<index_t>(rad_edges_.size());
    edge_bboxes_.resize(n);
    
    global_bbox_.r_min = 1e30;
    global_bbox_.r_max = -1e30;
    global_bbox_.z_min = 1e30;
    global_bbox_.z_max = -1e30;
    
    for (index_t i = 0; i < n; ++i) {
        const Point2D& p1 = mesh_->node(rad_edges_[i].v1);
        const Point2D& p2 = mesh_->node(rad_edges_[i].v2);
        
        edge_bboxes_[i].r_min = std::min(p1.r, p2.r);
        edge_bboxes_[i].r_max = std::max(p1.r, p2.r);
        edge_bboxes_[i].z_min = std::min(p1.z, p2.z);
        edge_bboxes_[i].z_max = std::max(p1.z, p2.z);
        
        global_bbox_.r_min = std::min(global_bbox_.r_min, edge_bboxes_[i].r_min);
        global_bbox_.r_max = std::max(global_bbox_.r_max, edge_bboxes_[i].r_max);
        global_bbox_.z_min = std::min(global_bbox_.z_min, edge_bboxes_[i].z_min);
        global_bbox_.z_max = std::max(global_bbox_.z_max, edge_bboxes_[i].z_max);
    }
}

void ViewFactorCalculator::compute_view_factors() {
    index_t n = static_cast<index_t>(rad_edges_.size());
    
    F_.resize(n, n, 0.0);
    
    for (index_t i = 0; i < n; ++i) {
        real_t row_sum = 0.0;
        
        for (index_t j = 0; j < n; ++j) {
            if (i == j) {
                F_(i, j) = 0.0;
                continue;
            }
            
            real_t F_ij = compute_pair_view_factor(i, j);
            F_(i, j) = F_ij;
            row_sum += F_ij;
        }
    }
    
    for (index_t i = 0; i < n; ++i) {
        real_t row_sum = 0.0;
        for (index_t j = 0; j < n; ++j) {
            row_sum += F_(i, j);
        }
        if (row_sum > 1e-10) {
            for (index_t j = 0; j < n; ++j) {
                F_(i, j) /= row_sum;
            }
        }
    }
}

real_t ViewFactorCalculator::compute_pair_view_factor(index_t i, index_t j) const {
    const RadiationEdge& ei = rad_edges_[i];
    const RadiationEdge& ej = rad_edges_[j];
    
    const Point2D& p1i = mesh_->node(ei.v1);
    const Point2D& p2i = mesh_->node(ei.v2);
    const Point2D& p1j = mesh_->node(ej.v1);
    const Point2D& p2j = mesh_->node(ej.v2);
    
    Point2D mid_i = ei.midpoint;
    Point2D mid_j = ej.midpoint;
    
    Point2D dir_ij = mid_j - mid_i;
    real_t dist = mid_i.dist(mid_j);
    if (dist < 1e-10) return 0.0;
    
    real_t cos_i = (dir_ij.r * ei.normal.r + dir_ij.z * ei.normal.z) / dist;
    real_t cos_j = (-dir_ij.r * ej.normal.r - dir_ij.z * ej.normal.z) / dist;
    
    if (cos_i <= 0.0 || cos_j <= 0.0) {
        return 0.0;
    }
    
    if (check_occlusion(p1i, p1j, i, j) && 
        check_occlusion(p2i, p1j, i, j) &&
        check_occlusion(p1i, p2j, i, j) &&
        check_occlusion(p2i, p2j, i, j)) {
        real_t F_approx = compute_integral_gauss(ei, ej);
        return std::max(0.0, F_approx);
    }
    
    return 0.0;
}

real_t ViewFactorCalculator::compute_integral_gauss(const RadiationEdge& ei,
                                                    const RadiationEdge& ej) const {
    const Point2D& p1i = mesh_->node(ei.v1);
    const Point2D& p2i = mesh_->node(ei.v2);
    const Point2D& p1j = mesh_->node(ej.v1);
    const Point2D& p2j = mesh_->node(ej.v2);
    
    std::vector<real_t> quad_pts;
    std::vector<real_t> quad_wts;
    Quadrature::line_gauss(quad_order_, quad_pts, quad_wts);
    index_t nq = static_cast<index_t>(quad_pts.size());
    
    real_t len_i = ei.length;
    real_t len_j = ej.length;
    real_t half_i = 0.5 * len_i;
    real_t half_j = 0.5 * len_j;
    
    real_t integral = 0.0;
    
    for (index_t qi = 0; qi < nq; ++qi) {
        real_t xi = quad_pts[qi];
        real_t wi = quad_wts[qi];
        
        real_t ti = 0.5 * (1.0 + xi);
        Point2D xi_point = Point2D(
            p1i.r + ti * (p2i.r - p1i.r),
            p1i.z + ti * (p2i.z - p1i.z)
        );
        real_t ri = xi_point.r;
        
        Point2D normal_i = ei.normal;
        
        for (index_t qj = 0; qj < nq; ++qj) {
            real_t xj = quad_pts[qj];
            real_t wj = quad_wts[qj];
            
            real_t tj = 0.5 * (1.0 + xj);
            Point2D xj_point = Point2D(
                p1j.r + tj * (p2j.r - p1j.r),
                p1j.z + tj * (p2j.z - p1j.z)
            );
            real_t rj = xj_point.r;
            
            Point2D normal_j = ej.normal;
            
            Point2D diff = xj_point - xi_point;
            real_t r_dist = std::sqrt(diff.r * diff.r + diff.z * diff.z);
            
            if (r_dist < 1e-10) continue;
            
            real_t cos_i = (diff.r * normal_i.r + diff.z * normal_i.z) / r_dist;
            real_t cos_j = (-diff.r * normal_j.r - diff.z * normal_j.z) / r_dist;
            
            if (cos_i <= 0.0 || cos_j <= 0.0) continue;
            
            real_t integrand = cos_i * cos_j / (M_PI * r_dist * r_dist);
            
            real_t circumference_factor = 2.0 * M_PI * std::min(ri, rj);
            integrand *= circumference_factor;
            
            integral += integrand * ri * rj * half_i * half_j * wi * wj;
        }
    }
    
    real_t Ai = ei.area;
    if (Ai < 1e-30) return 0.0;
    
    return integral / Ai;
}

bool ViewFactorCalculator::check_occlusion(const Point2D& p1, const Point2D& p2,
                                           index_t skip_edge1, index_t skip_edge2) const {
    Point2D dir = p2 - p1;
    real_t total_len = p1.dist(p2);
    
    if (total_len < 1e-12) return true;
    
    Point2D dir_norm = dir * (1.0 / total_len);
    
    index_t n = static_cast<index_t>(rad_edges_.size());
    
    for (index_t k = 0; k < n; ++k) {
        if (k == skip_edge1 || k == skip_edge2) continue;
        
        const Edge& edge = mesh_->edge(
            (k < mesh_->num_edges()) ? k : 0
        );
        
        const Point2D& a = mesh_->node(rad_edges_[k].v1);
        const Point2D& b = mesh_->node(rad_edges_[k].v2);
        
        real_t t = 0.0;
        if (geometry::ray_segment_intersect(p1, dir_norm, a, b, t)) {
            if (t > 1e-6 && t < total_len - 1e-6) {
                return false;
            }
        }
    }
    
    return true;
}

void ViewFactorCalculator::compute_view_factor_surface() {
    compute_view_factors();
}

} 
