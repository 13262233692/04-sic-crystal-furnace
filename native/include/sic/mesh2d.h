#ifndef SIC_MESH2D_H
#define SIC_MESH2D_H

#include "sic/types.h"
#include <vector>
#include <map>
#include <memory>

namespace sic {

class Mesh2D {
public:
    Mesh2D();
    
    void clear();
    
    index_t num_nodes() const { return static_cast<index_t>(nodes_.size()); }
    index_t num_elements() const { return static_cast<index_t>(elements_.size()); }
    index_t num_edges() const { return static_cast<index_t>(edges_.size()); }
    index_t num_regions() const { return static_cast<index_t>(regions_.size()); }
    
    Point2D& node(index_t i) { return nodes_[i]; }
    const Point2D& node(index_t i) const { return nodes_[i]; }
    
    Triangle& element(index_t i) { return elements_[i]; }
    const Triangle& element(index_t i) const { return elements_[i]; }
    
    Edge& edge(index_t i) { return edges_[i]; }
    const Edge& edge(index_t i) const { return edges_[i]; }
    
    std::vector<Point2D>& nodes() { return nodes_; }
    const std::vector<Point2D>& nodes() const { return nodes_; }
    
    std::vector<Triangle>& elements() { return elements_; }
    const std::vector<Triangle>& elements() const { return elements_; }
    
    std::vector<Edge>& edges() { return edges_; }
    const std::vector<Edge>& edges() const { return edges_; }
    
    std::map<index_t, RegionInfo>& regions() { return regions_; }
    const std::map<index_t, RegionInfo>& regions() const { return regions_; }
    
    void add_node(real_t r, real_t z);
    void add_element(index_t v1, index_t v2, index_t v3, index_t region_id = 0);
    void add_edge(index_t v1, index_t v2, index_t boundary_id = -1, bool exposed = false);
    void add_region(index_t id, const std::string& name, const MaterialProperties& mat);
    
    void build_boundary_edges();
    void find_exposed_boundaries();
    
    const std::vector<index_t>& node_to_elements(index_t node_id) const;
    const std::vector<index_t>& node_adjacent_nodes(index_t node_id) const;
    
    real_t element_area(index_t elem_id) const;
    
    void build_node_connectivity();
    
private:
    std::vector<Point2D> nodes_;
    std::vector<Triangle> elements_;
    std::vector<Edge> edges_;
    std::map<index_t, RegionInfo> regions_;
    
    mutable std::vector<std::vector<index_t>> node_to_elem_;
    mutable std::vector<std::vector<index_t>> node_adj_nodes_;
    mutable bool connectivity_built_;
};

} 

#endif 
