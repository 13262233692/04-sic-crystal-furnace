#include "sic/geometry.h"
#include <cmath>
#include <algorithm>

namespace sic {
namespace geometry {

real_t tri_area(const Point2D& p1, const Point2D& p2, const Point2D& p3) {
    return 0.5 * std::fabs(
        (p2.r - p1.r) * (p3.z - p1.z) -
        (p2.z - p1.z) * (p3.r - p1.r)
    );
}

Point2D tri_centroid(const Point2D& p1, const Point2D& p2, const Point2D& p3) {
    return Point2D(
        (p1.r + p2.r + p3.r) / 3.0,
        (p1.z + p2.z + p3.z) / 3.0
    );
}

real_t edge_length(const Point2D& p1, const Point2D& p2) {
    real_t dr = p2.r - p1.r;
    real_t dz = p2.z - p1.z;
    return std::sqrt(dr*dr + dz*dz);
}

void tri_shape_deriv(const Point2D& p1, const Point2D& p2, const Point2D& p3,
                     real_t& drN1, real_t& dzN1,
                     real_t& drN2, real_t& dzN2,
                     real_t& drN3, real_t& dzN3) {
    real_t area2 = (p2.r - p1.r) * (p3.z - p1.z) -
                   (p2.z - p1.z) * (p3.r - p1.r);
    
    if (std::fabs(area2) < 1e-30) {
        drN1 = dzN1 = drN2 = dzN2 = drN3 = dzN3 = 0.0;
        return;
    }
    
    real_t inv_2A = 1.0 / area2;
    
    drN1 = (p2.z - p3.z) * inv_2A;
    dzN1 = (p3.r - p2.r) * inv_2A;
    
    drN2 = (p3.z - p1.z) * inv_2A;
    dzN2 = (p1.r - p3.r) * inv_2A;
    
    drN3 = (p1.z - p2.z) * inv_2A;
    dzN3 = (p2.r - p1.r) * inv_2A;
}

void tri_shape_func(real_t xi, real_t eta,
                    real_t& N1, real_t& N2, real_t& N3) {
    N1 = 1.0 - xi - eta;
    N2 = xi;
    N3 = eta;
}

real_t line_circumference_at_r(real_t r1, real_t r2) {
    return M_PI * (r1 + r2);
}

bool point_in_triangle(const Point2D& p, const Point2D& p1,
                       const Point2D& p2, const Point2D& p3) {
    real_t d1 = (p.r - p2.r) * (p1.z - p2.z) - (p1.r - p2.r) * (p.z - p2.z);
    real_t d2 = (p.r - p3.r) * (p2.z - p3.z) - (p2.r - p3.r) * (p.z - p3.z);
    real_t d3 = (p.r - p1.r) * (p3.z - p1.z) - (p3.r - p1.r) * (p.z - p1.z);
    
    bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    
    return !(has_neg && has_pos);
}

static real_t cross2d(const Point2D& a, const Point2D& b) {
    return a.r * b.z - a.z * b.r;
}

bool segments_intersect(const Point2D& a1, const Point2D& a2,
                        const Point2D& b1, const Point2D& b2,
                        Point2D* intersection) {
    Point2D da = a2 - a1;
    Point2D db = b2 - b1;
    Point2D delta = b1 - a1;
    
    real_t denom = cross2d(da, db);
    
    if (std::fabs(denom) < 1e-30) {
        return false;
    }
    
    real_t t = cross2d(delta, db) / denom;
    real_t u = cross2d(delta, da) / denom;
    
    if (t >= 0.0 && t <= 1.0 && u >= 0.0 && u <= 1.0) {
        if (intersection) {
            *intersection = a1 + da * t;
        }
        return true;
    }
    
    return false;
}

real_t point_to_segment_dist(const Point2D& p, const Point2D& a, const Point2D& b) {
    Point2D ab = b - a;
    Point2D ap = p - a;
    
    real_t t = (ap.r * ab.r + ap.z * ab.z);
    real_t len_sq = ab.r*ab.r + ab.z*ab.z;
    
    if (len_sq < 1e-30) {
        return p.dist(a);
    }
    
    t /= len_sq;
    t = std::max(0.0, std::min(1.0, t));
    
    Point2D proj = a + ab * t;
    return p.dist(proj);
}

bool ray_segment_intersect(const Point2D& origin, const Point2D& dir,
                           const Point2D& a, const Point2D& b,
                           real_t& t_param) {
    Point2D seg_dir = b - a;
    Point2D delta = a - origin;
    
    real_t denom = dir.r * seg_dir.z - dir.z * seg_dir.r;
    
    if (std::fabs(denom) < 1e-30) {
        return false;
    }
    
    real_t t = (delta.r * seg_dir.z - delta.z * seg_dir.r) / denom;
    real_t u = (delta.r * dir.z - delta.z * dir.r) / denom;
    
    if (t > 1e-12 && u >= 0.0 && u <= 1.0) {
        t_param = t;
        return true;
    }
    
    return false;
}

} 
} 
