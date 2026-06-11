#ifndef SIC_VIEW_FACTOR_H
#define SIC_VIEW_FACTOR_H

#include "sic/types.h"
#include "sic/mesh2d.h"
#include "sic/dense_matrix.h"
#include <vector>
#include <memory>

namespace sic {

struct RadiationEdge {
    index_t v1, v2;
    Point2D midpoint;
    real_t length;
    real_t r_avg;
    real_t area;
    Point2D normal;
    index_t id;
    index_t boundary_id;
};

class ViewFactorCalculator {
public:
    explicit ViewFactorCalculator(std::shared_ptr<Mesh2D> mesh);
    
    void set_quadrature_order(index_t order) { quad_order_ = order; }
    index_t quadrature_order() const { return quad_order_; }
    
    void extract_radiation_edges();
    
    void compute_view_factors();
    
    void compute_view_factor_surface();
    
    const std::vector<RadiationEdge>& radiation_edges() const { return rad_edges_; }
    index_t num_rad_edges() const { return static_cast<index_t>(rad_edges_.size()); }
    
    const DenseMatrix& view_factor_matrix() const { return F_; }
    DenseMatrix& view_factor_matrix() { return F_; }
    
    std::vector<real_t>& areas() { return areas_; }
    const std::vector<real_t>& areas() const { return areas_; }
    
    real_t compute_pair_view_factor(index_t i, index_t j) const;
    
    bool check_occlusion(const Point2D& p1, const Point2D& p2,
                         index_t skip_edge1, index_t skip_edge2) const;
    
    void build_acceleration_structure();
    
private:
    std::shared_ptr<Mesh2D> mesh_;
    std::vector<RadiationEdge> rad_edges_;
    DenseMatrix F_;
    std::vector<real_t> areas_;
    index_t quad_order_;
    
    struct BBox {
        real_t r_min, r_max, z_min, z_max;
        bool contains(const Point2D& p) const {
            return p.r >= r_min && p.r <= r_max &&
                   p.z >= z_min && p.z <= z_max;
        }
    };
    
    std::vector<BBox> edge_bboxes_;
    BBox global_bbox_;
    
    real_t compute_integral_gauss(const RadiationEdge& ei, 
                                  const RadiationEdge& ej) const;
    
    real_t integrand_kernel(const Point2D& x, const Point2D& nx,
                            const Point2D& y, const Point2D& ny) const;
};

} 

#endif 
