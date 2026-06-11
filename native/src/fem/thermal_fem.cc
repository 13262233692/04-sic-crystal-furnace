#include "sic/thermal_fem.h"
#include "sic/geometry.h"
#include "sic/gauss_quadrature.h"
#include <cmath>
#include <iostream>

namespace sic {

ThermalFEM::ThermalFEM(std::shared_ptr<Mesh2D> mesh)
    : mesh_(mesh), boundary_elem_count_(0) {
}

void ThermalFEM::initialize() {
    index_t n_nodes = mesh_->num_nodes();
    
    K_.resize(n_nodes, n_nodes);
    M_.resize(n_nodes, n_nodes);
    rhs_.assign(n_nodes, 0.0);
    
    K_coo_.n_rows = n_nodes;
    K_coo_.n_cols = n_nodes;
    
    M_coo_.n_rows = n_nodes;
    M_coo_.n_cols = n_nodes;
}

real_t ThermalFEM::compute_radial_circumference(real_t r) const {
    return 2.0 * M_PI * r;
}

void ThermalFEM::assemble_stiffness_matrix() {
    index_t n_elem = mesh_->num_elements();
    
    K_coo_.clear();
    
    for (index_t e = 0; e < n_elem; ++e) {
        const Triangle& tri = mesh_->element(e);
        index_t region_id = tri.region_id;
        
        MaterialProperties mat;
        auto it = mesh_->regions().find(region_id);
        if (it != mesh_->regions().end()) {
            mat = it->second.material;
        }
        
        DenseMatrix Ke(3, 3, 0.0);
        compute_elem_stiffness(e, mat, Ke);
        
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                K_coo_.add(tri[i], tri[j], Ke(i, j));
            }
        }
    }
    
    K_coo_.to_csr(K_);
}

void ThermalFEM::assemble_mass_matrix() {
    index_t n_elem = mesh_->num_elements();
    
    M_coo_.clear();
    
    for (index_t e = 0; e < n_elem; ++e) {
        const Triangle& tri = mesh_->element(e);
        index_t region_id = tri.region_id;
        
        MaterialProperties mat;
        auto it = mesh_->regions().find(region_id);
        if (it != mesh_->regions().end()) {
            mat = it->second.material;
        }
        
        DenseMatrix Me(3, 3, 0.0);
        compute_elem_mass(e, mat, Me);
        
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                M_coo_.add(tri[i], tri[j], Me(i, j));
            }
        }
    }
    
    M_coo_.to_csr(M_);
}

void ThermalFEM::compute_element_matrices(index_t elem_id,
                                          DenseMatrix& Ke,
                                          DenseMatrix& Me,
                                          Vector& fe) const {
    const Triangle& tri = mesh_->element(elem_id);
    index_t region_id = tri.region_id;
    
    MaterialProperties mat;
    auto it = mesh_->regions().find(region_id);
    if (it != mesh_->regions().end()) {
        mat = it->second.material;
    }
    
    Ke.resize(3, 3);
    Me.resize(3, 3);
    fe.assign(3, 0.0);
    
    compute_elem_stiffness(elem_id, mat, Ke);
    compute_elem_mass(elem_id, mat, Me);
}

void ThermalFEM::compute_elem_stiffness(index_t elem_id,
                                        const MaterialProperties& mat,
                                        DenseMatrix& Ke) const {
    const Triangle& tri = mesh_->element(elem_id);
    const Point2D& p1 = mesh_->node(tri[0]);
    const Point2D& p2 = mesh_->node(tri[1]);
    const Point2D& p3 = mesh_->node(tri[2]);
    
    Ke.zero();
    
    std::vector<std::array<real_t, 2>> quad_pts;
    std::vector<real_t> quad_wts;
    Quadrature::tri_gauss(4, quad_pts, quad_wts);
    index_t nqp = static_cast<index_t>(quad_pts.size());
    
    for (index_t q = 0; q < nqp; ++q) {
        real_t xi = quad_pts[q][0];
        real_t eta = quad_pts[q][1];
        real_t wt = quad_wts[q];
        
        real_t N1, N2, N3;
        geometry::tri_shape_func(xi, eta, N1, N2, N3);
        
        real_t r = N1 * p1.r + N2 * p2.r + N3 * p3.r;
        real_t z = N1 * p1.z + N2 * p2.z + N3 * p3.z;
        (void)z;
        
        real_t drN1, dzN1, drN2, dzN2, drN3, dzN3;
        geometry::tri_shape_deriv(p1, p2, p3, drN1, dzN1, drN2, dzN2, drN3, dzN3);
        
        real_t area = geometry::tri_area(p1, p2, p3);
        real_t detJ = 2.0 * area;
        
        real_t circumference = 2.0 * M_PI * r;
        
        real_t dNdr[3] = {drN1, drN2, drN3};
        real_t dNdz[3] = {dzN1, dzN2, dzN3};
        
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                real_t k_term = mat.k_r * dNdr[i] * dNdr[j] +
                                mat.k_z * dNdz[i] * dNdz[j];
                Ke(i, j) += k_term * circumference * detJ * wt;
            }
        }
    }
}

void ThermalFEM::compute_elem_mass(index_t elem_id,
                                   const MaterialProperties& mat,
                                   DenseMatrix& Me) const {
    const Triangle& tri = mesh_->element(elem_id);
    const Point2D& p1 = mesh_->node(tri[0]);
    const Point2D& p2 = mesh_->node(tri[1]);
    const Point2D& p3 = mesh_->node(tri[2]);
    
    Me.zero();
    
    std::vector<std::array<real_t, 2>> quad_pts;
    std::vector<real_t> quad_wts;
    Quadrature::tri_gauss(3, quad_pts, quad_wts);
    index_t nqp = static_cast<index_t>(quad_pts.size());
    
    real_t rho_cp = mat.rho * mat.cp;
    
    for (index_t q = 0; q < nqp; ++q) {
        real_t xi = quad_pts[q][0];
        real_t eta = quad_pts[q][1];
        real_t wt = quad_wts[q];
        
        real_t N1, N2, N3;
        geometry::tri_shape_func(xi, eta, N1, N2, N3);
        
        real_t r = N1 * p1.r + N2 * p2.r + N3 * p3.r;
        
        real_t area = geometry::tri_area(p1, p2, p3);
        real_t detJ = 2.0 * area;
        
        real_t circumference = 2.0 * M_PI * r;
        real_t N[3] = {N1, N2, N3};
        
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                Me(i, j) += rho_cp * N[i] * N[j] * circumference * detJ * wt;
            }
        }
    }
}

void ThermalFEM::apply_dirichlet_bc(index_t boundary_id, real_t temperature) {
    for (index_t e = 0; e < mesh_->num_edges(); ++e) {
        const Edge& edge = mesh_->edge(e);
        if (edge.boundary_id == boundary_id) {
            for (int i = 0; i < 2; ++i) {
                index_t node_id = edge[i];
                K_.apply_dirichlet(node_id, temperature, rhs_);
            }
        }
    }
}

void ThermalFEM::apply_neumann_bc(index_t boundary_id, real_t heat_flux) {
    for (index_t e = 0; e < mesh_->num_edges(); ++e) {
        const Edge& edge = mesh_->edge(e);
        if (edge.boundary_id == boundary_id) {
            Vector fe(2, 0.0);
            compute_elem_neumann(e, heat_flux, fe);
            
            for (int i = 0; i < 2; ++i) {
                rhs_[edge[i]] += fe[i];
            }
        }
    }
}

void ThermalFEM::compute_elem_neumann(index_t edge_id, real_t flux,
                                      Vector& fe) const {
    const Edge& edge = mesh_->edge(edge_id);
    const Point2D& p1 = mesh_->node(edge[0]);
    const Point2D& p2 = mesh_->node(edge[1]);
    
    fe.assign(2, 0.0);
    
    std::vector<real_t> quad_pts;
    std::vector<real_t> quad_wts;
    Quadrature::line_gauss(4, quad_pts, quad_wts);
    
    real_t len = geometry::edge_length(p1, p2);
    real_t half_len = 0.5 * len;
    
    for (size_t q = 0; q < quad_pts.size(); ++q) {
        real_t xi = quad_pts[q];
        real_t wt = quad_wts[q];
        
        real_t t = 0.5 * (1.0 + xi);
        real_t r = p1.r + t * (p2.r - p1.r);
        
        real_t circumference = 2.0 * M_PI * r;
        
        real_t N1 = 1.0 - t;
        real_t N2 = t;
        
        fe[0] += flux * N1 * circumference * half_len * wt;
        fe[1] += flux * N2 * circumference * half_len * wt;
    }
}

} 
