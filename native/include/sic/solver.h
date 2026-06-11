#ifndef SIC_SOLVER_H
#define SIC_SOLVER_H

#include "sic/types.h"
#include "sic/mesh2d.h"
#include "sic/thermal_fem.h"
#include "sic/view_factor.h"
#include "sic/radiation_coupling.h"
#include "sic/newton_raphson.h"
#include "sic/transient_solver.h"
#include <vector>
#include <memory>
#include <map>

namespace sic {

struct SimulationParams {
    bool include_radiation;
    bool steady_state;
    
    real_t initial_temperature;
    real_t ambient_temperature;
    
    real_t time_start;
    real_t time_end;
    real_t time_step;
    
    real_t newton_tolerance;
    index_t newton_max_iter;
    
    index_t radiation_quad_order;
    
    SimulationParams()
        : include_radiation(true)
        , steady_state(true)
        , initial_temperature(300.0)
        , ambient_temperature(300.0)
        , time_start(0.0)
        , time_end(1.0)
        , time_step(0.1)
        , newton_tolerance(1e-6)
        , newton_max_iter(50)
        , radiation_quad_order(4)
    {}
};

struct SimulationResult {
    Vector temperature;
    Vector heat_flux_r;
    Vector heat_flux_z;
    
    std::vector<Vector> temperature_history;
    std::vector<real_t> time_history;
    
    real_t max_temperature;
    real_t min_temperature;
    
    std::vector<real_t> newton_residuals;
    index_t newton_iterations;
    bool converged;
    
    real_t solve_time;
};

class SiCFurnaceSolver {
public:
    SiCFurnaceSolver();
    
    void set_mesh(std::shared_ptr<Mesh2D> mesh);
    void set_params(const SimulationParams& params) { params_ = params; }
    const SimulationParams& params() const { return params_; }
    
    void initialize();
    
    bool solve_steady_state(SimulationResult& result);
    bool solve_transient(SimulationResult& result);
    
    void set_dirichlet_bc(index_t boundary_id, real_t temperature);
    void set_heat_source(index_t region_id, real_t power_density);
    
    std::shared_ptr<Mesh2D> mesh() { return mesh_; }
    std::shared_ptr<ThermalFEM> fem() { return fem_; }
    std::shared_ptr<ViewFactorCalculator> view_factor() { return vf_calc_; }
    std::shared_ptr<RadiationCoupling> radiation() { return rad_coupling_; }
    
    void compute_view_factors();
    
    const DenseMatrix& view_factor_matrix() const { return vf_calc_->view_factor_matrix(); }
    
private:
    std::shared_ptr<Mesh2D> mesh_;
    std::shared_ptr<ThermalFEM> fem_;
    std::shared_ptr<ViewFactorCalculator> vf_calc_;
    std::shared_ptr<RadiationCoupling> rad_coupling_;
    std::shared_ptr<NewtonRaphson> newton_;
    std::shared_ptr<TransientSolver> transient_;
    
    SimulationParams params_;
    
    Vector heat_source_;
    Vector prev_T_transient_;
    Vector mass_diag_;
    
    bool initialized_;
    bool vf_computed_;
    
    struct DirichletBC {
        index_t boundary_id;
        real_t temperature;
    };
    std::vector<DirichletBC> dirichlet_bcs_;
    
    void assemble_residual(const Vector& T, Vector& R);
    void assemble_jacobian(const Vector& T, SparseMatrixCSR& J);
    
    void apply_boundary_conditions();
    void apply_dirichlet_to_system(SparseMatrixCSR& K, Vector& rhs);
};

} 

#endif 
