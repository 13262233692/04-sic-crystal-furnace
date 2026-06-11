#ifndef SIC_GEOMETRY_H
#define SIC_GEOMETRY_H

#include "sic/types.h"
#include <vector>

namespace sic {
namespace geometry {

real_t tri_area(const Point2D& p1, const Point2D& p2, const Point2D& p3);

Point2D tri_centroid(const Point2D& p1, const Point2D& p2, const Point2D& p3);

real_t edge_length(const Point2D& p1, const Point2D& p2);

void tri_shape_deriv(const Point2D& p1, const Point2D& p2, const Point2D& p3,
                     real_t& drN1, real_t& dzN1,
                     real_t& drN2, real_t& dzN2,
                     real_t& drN3, real_t& dzN3);

void tri_shape_func(real_t xi, real_t eta,
                    real_t& N1, real_t& N2, real_t& N3);

real_t line_circumference_at_r(real_t r1, real_t r2);

bool point_in_triangle(const Point2D& p, const Point2D& p1, 
                       const Point2D& p2, const Point2D& p3);

bool segments_intersect(const Point2D& a1, const Point2D& a2,
                        const Point2D& b1, const Point2D& b2,
                        Point2D* intersection = nullptr);

real_t point_to_segment_dist(const Point2D& p, const Point2D& a, const Point2D& b);

bool ray_segment_intersect(const Point2D& origin, const Point2D& dir,
                           const Point2D& a, const Point2D& b,
                           real_t& t_param);

}
} 

#endif 
