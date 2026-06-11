#ifndef SIC_TYPES_H
#define SIC_TYPES_H

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>

namespace sic {

using real_t = double;
using index_t = int;
using Vector = std::vector<real_t>;
using IndexVector = std::vector<index_t>;

struct Point2D {
    real_t r, z;
    Point2D() : r(0.0), z(0.0) {}
    Point2D(real_t r_, real_t z_) : r(r_), z(z_) {}
    
    real_t dist(const Point2D& other) const {
        real_t dr = r - other.r;
        real_t dz = z - other.z;
        return std::sqrt(dr*dr + dz*dz);
    }
    
    Point2D operator+(const Point2D& other) const {
        return Point2D(r + other.r, z + other.z);
    }
    
    Point2D operator-(const Point2D& other) const {
        return Point2D(r - other.r, z - other.z);
    }
    
    Point2D operator*(real_t s) const {
        return Point2D(r * s, z * s);
    }
};

struct Triangle {
    index_t v[3];
    index_t region_id;
    
    Triangle() { v[0] = v[1] = v[2] = 0; region_id = 0; }
    Triangle(index_t a, index_t b, index_t c, index_t reg = 0) {
        v[0] = a; v[1] = b; v[2] = c; region_id = reg;
    }
    
    index_t& operator[](int i) { return v[i]; }
    index_t operator[](int i) const { return v[i]; }
};

struct Edge {
    index_t v[2];
    index_t boundary_id;
    bool is_exposed;
    
    Edge() : boundary_id(-1), is_exposed(false) { v[0] = v[1] = 0; }
    Edge(index_t a, index_t b, index_t bid = -1, bool exposed = false)
        : boundary_id(bid), is_exposed(exposed) {
        v[0] = a; v[1] = b;
    }
    
    index_t& operator[](int i) { return v[i]; }
    index_t operator[](int i) const { return v[i]; }
};

struct MaterialProperties {
    real_t k_r;
    real_t k_z;
    real_t rho;
    real_t cp;
    real_t emissivity;
    
    MaterialProperties()
        : k_r(1.0), k_z(1.0), rho(1.0), cp(1.0), emissivity(0.5) {}
    
    MaterialProperties(real_t kr, real_t kz, real_t density, real_t heat_cap, real_t emis)
        : k_r(kr), k_z(kz), rho(density), cp(heat_cap), emissivity(emis) {}
};

struct RegionInfo {
    std::string name;
    MaterialProperties material;
    index_t id;
};

struct BoundaryCondition {
    enum Type {
        DIRICHLET,
        NEUMANN,
        RADIATION,
        CONVECTION
    };
    
    Type type;
    real_t value;
    real_t reference_temp;
    real_t heat_transfer_coeff;
    index_t boundary_id;
};

const real_t STEFAN_BOLTZMANN = 5.670374419e-8;
const real_t ABS_ZERO = 273.15;

} 

#endif 
