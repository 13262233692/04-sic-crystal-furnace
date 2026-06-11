#include "sic/mesh2d.h"
#include "sic/geometry.h"
#include <stdexcept>
#include <set>
#include <map>

namespace sic {

Mesh2D::Mesh2D() : connectivity_built_(false) {}

void Mesh2D::clear() {
    nodes_.clear();
    elements_.clear();
    edges_.clear();
    regions_.clear();
    node_to_elem_.clear();
    node_adj_nodes_.clear();
    connectivity_built_ = false;
}

void Mesh2D::add_node(real_t r, real_t z) {
    nodes_.emplace_back(r, z);
    connectivity_built_ = false;
}

void Mesh2D::add_element(index_t v1, index_t v2, index_t v3, index_t region_id) {
    elements_.emplace_back(v1, v2, v3, region_id);
    connectivity_built_ = false;
}

void Mesh2D::add_edge(index_t v1, index_t v2, index_t boundary_id, bool exposed) {
    edges_.emplace_back(v1, v2, boundary_id, exposed);
}

void Mesh2D::add_region(index_t id, const std::string& name, const MaterialProperties& mat) {
    RegionInfo info;
    info.id = id;
    info.name = name;
    info.material = mat;
    regions_[id] = info;
}

void Mesh2D::build_node_connectivity() const {
    if (connectivity_built_) return;
    
    index_t n_nodes = num_nodes();
    node_to_elem_.assign(n_nodes, std::vector<index_t>());
    node_adj_nodes_.assign(n_nodes, std::vector<index_t>());
    
    for (index_t e = 0; e < num_elements(); ++e) {
        const Triangle& tri = elements_[e];
        for (int i = 0; i < 3; ++i) {
            index_t vi = tri[i];
            node_to_elem_[vi].push_back(e);
            
            for (int j = 0; j < 3; ++j) {
                if (i != j) {
                    node_adj_nodes_[vi].push_back(tri[j]);
                }
            }
        }
    }
    
    for (index_t n = 0; n < n_nodes; ++n) {
        auto& adj = node_adj_nodes_[n];
        std::sort(adj.begin(), adj.end());
        auto last = std::unique(adj.begin(), adj.end());
        adj.erase(last, adj.end());
    }
    
    connectivity_built_ = true;
}

const std::vector<index_t>& Mesh2D::node_to_elements(index_t node_id) const {
    if (!connectivity_built_) build_node_connectivity();
    return node_to_elem_[node_id];
}

const std::vector<index_t>& Mesh2D::node_adjacent_nodes(index_t node_id) const {
    if (!connectivity_built_) build_node_connectivity();
    return node_adj_nodes_[node_id];
}

real_t Mesh2D::element_area(index_t elem_id) const {
    const Triangle& tri = elements_[elem_id];
    return geometry::tri_area(nodes_[tri[0]], nodes_[tri[1]], nodes_[tri[2]]);
}

struct EdgeKey {
    index_t v1, v2;
    EdgeKey(index_t a, index_t b) {
        if (a < b) { v1 = a; v2 = b; }
        else { v1 = b; v2 = a; }
    }
    bool operator<(const EdgeKey& other) const {
        if (v1 != other.v1) return v1 < other.v1;
        return v2 < other.v2;
    }
};

void Mesh2D::build_boundary_edges() {
    std::map<EdgeKey, int> edge_count;
    std::map<EdgeKey, index_t> edge_boundary;
    
    for (index_t e = 0; e < num_elements(); ++e) {
        const Triangle& tri = elements_[e];
        for (int i = 0; i < 3; ++i) {
            index_t v1 = tri[i];
            index_t v2 = tri[(i + 1) % 3];
            EdgeKey key(v1, v2);
            edge_count[key]++;
        }
    }
    
    edges_.clear();
    for (const auto& entry : edge_count) {
        if (entry.second == 1) {
            add_edge(entry.first.v1, entry.first.v2, 0, true);
        }
    }
    
    find_exposed_boundaries();
}

void Mesh2D::find_exposed_boundaries() {
    build_node_connectivity();
    
    for (auto& edge : edges_) {
        edge.is_exposed = true;
    }
}

} 
