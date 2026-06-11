#ifndef SIC_GAUSS_QUADRATURE_H
#define SIC_GAUSS_QUADRATURE_H

#include "sic/types.h"
#include <array>
#include <cmath>

namespace sic {

template<int N>
struct GaussLegendre {
    static const std::array<real_t, N> points;
    static const std::array<real_t, N> weights;
};

template<int N>
struct TriangleQuadrature {
    static std::array<std::array<real_t, 2>, N> points;
    static std::array<real_t, N> weights;
};

class Quadrature {
public:
    static void line_gauss(index_t order, std::vector<real_t>& pts, std::vector<real_t>& wts);
    
    static void tri_gauss(index_t order, std::vector<std::array<real_t, 2>>& pts, 
                          std::vector<real_t>& wts);
    
    static index_t tri_num_points(index_t order);
};

} 

#endif 
