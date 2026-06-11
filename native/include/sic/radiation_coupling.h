#ifndef SIC_RADIATION_COUPLING_H
#define SIC_RADIATION_COUPLING_H

#include "sic/types.h"
#include "sic/mesh2d.h"
#include "sic/sparse_matrix.h"
#include "sic/view_factor.h"
#include <vector>
#include <memory>

namespace sic {

class RadiationCoupling {
public:
    RadiationCoupling(std::shared_ptr<Mesh2D> mesh,
                      std::shared_ptr<ViewFactorCalculator> vf_calc);
    
    void initialize();
    
    void compute_radiation_heat_flux(const Vector& temperature, Vector& heat_flux);
    
    void assemble_radiation_jacobian(const Vector& temperature, SparseMatrixCSR& J);
    
    void add_radiation_to_rhs(const Vector& temperature, Vector& rhs);
    
    void add_radiation_to_jacobian(const Vector& temperature, SparseMatrixCOO& jac);
    
    void set_ambient_temperature(real_t T_amb) { T_ambient_ = T_amb; }
    real_t ambient_temperature() const { return T_ambient_; }
    
    void set_stefan_boltzmann(real_t sigma) { sigma_ = sigma; }
    real_t stefan_boltzmann() const { return sigma_; }
    
    std::vector<index_t>& rad_node_ids() { return rad_node_ids_; }
    const std::vector<index_t>& rad_node_ids() const { return rad_node_ids_; }
    
    void build_rad_node_mapping();
    
    real_t compute_residual_norm(const Vector& temperature, const Vector& rhs_heat) const;
    
private:
    std::shared_ptr<Mesh2D> mesh_;
    std::shared_ptr<ViewFactorCalculator> vf_calc_;
    
    Vector rad_power_;
    Vector T4_vec_;
    Vector radiosity_;
    Vector irradiation_;
    
    std::vector<index_t> rad_node_ids_;
    std::vector<index_t> node_to_rad_;
    
    real_t T_ambient_;
    real_t sigma_;
    
    void compute_blackbody_emission(const Vector& T, Vector& Eb) const;
    void solve_radiosity(const Vector& T, Vector& J) const;
    void compute_net_radiation(const Vector& J, Vector& q) const;
    
    void compute_radiosity_derivative(const Vector& T,
                                      const Vector& J,
                                      DenseMatrix& dJ_dT) const;
};

} 

#endif 
