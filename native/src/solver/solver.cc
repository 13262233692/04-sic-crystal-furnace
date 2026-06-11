#include "sic/solver.h"
#include <cmath>
#include <iostream>
#include <chrono>

namespace sic {

SiCFurnaceSolver::SiCFurnaceSolver()
    : initialized_(false)
    , vf_computed_(false)
    , prev_T_transient_()
    , mass_diag_() {
    
    newton_ = std::make_shared<NewtonRaphson>();
    transient_ = std::make_shared<TransientSolver>();
}

void SiCFurnaceSolver::set_mesh(std::shared_ptr<Mesh2D> mesh) {
    mesh_ = mesh;
    fem_ = std::make_shared<ThermalFEM>(mesh);
    vf_calc_ = std::make_shared<ViewFactorCalculator>(mesh);
    rad_coupling_ = std::make_shared<RadiationCoupling>(mesh, vf_calc_);
    
    initialized_ = false;
    vf_computed_ = false;
}

void SiCFurnaceSolver::initialize() {
    if (!mesh_) {
        throw std::runtime_error("SiCFurnaceSolver: mesh not set");
    }
    
    mesh_->build_boundary_edges();
    
    fem_->initialize();
    fem_->assemble_stiffness_matrix();
    fem_->assemble_mass_matrix();
    
    index_t n = mesh_->num_nodes();
    heat_source_.assign(n, 0.0);
    
    mass_diag_.assign(n, 0.0);
    const auto& M = fem_->mass_matrix();
    for (index_t i = 0; i < n; ++i) {
        mass_diag_[i] = M.get(i, i);
        if (mass_diag_[i] < 1e-30) {
            mass_diag_[i] = 1.0;
        }
    }
    
    vf_calc_->set_quadrature_order(params_.radiation_quad_order);
    vf_calc_->extract_radiation_edges();
    
    rad_coupling_->set_ambient_temperature(params_.ambient_temperature);
    rad_coupling_->set_stefan_boltzmann(STEFAN_BOLTZMANN);
    rad_coupling_->initialize();
    
    newton_->set_tolerance(params_.newton_tolerance);
    newton_->set_max_iterations(params_.newton_max_iter);
    
    initialized_ = true;
}

void SiCFurnaceSolver::set_dirichlet_bc(index_t boundary_id, real_t temperature) {
    DirichletBC bc;
    bc.boundary_id = boundary_id;
    bc.temperature = temperature;
    dirichlet_bcs_.push_back(bc);
}

void SiCFurnaceSolver::set_heat_source(index_t region_id, real_t power_density) {
    for (index_t e = 0; e < mesh_->num_elements(); ++e) {
        const Triangle& tri = mesh_->element(e);
        if (tri.region_id == region_id) {
            real_t area = mesh_->element_area(e);
            const Point2D& p1 = mesh_->node(tri[0]);
            const Point2D& p2 = mesh_->node(tri[1]);
            const Point2D& p3 = mesh_->node(tri[2]);
            real_t r_avg = (p1.r + p2.r + p3.r) / 3.0;
            real_t volume = 2.0 * M_PI * r_avg * area;
            real_t total_power = power_density * volume;
            
            for (int i = 0; i < 3; ++i) {
                heat_source_[tri[i]] += total_power / 3.0;
            }
        }
    }
}

void SiCFurnaceSolver::compute_view_factors() {
    if (!initialized_) initialize();
    vf_calc_->compute_view_factors();
    vf_computed_ = true;
}

void SiCFurnaceSolver::apply_boundary_conditions() {
    for (const auto& bc : dirichlet_bcs_) {
        fem_->apply_dirichlet_bc(bc.boundary_id, bc.temperature);
    }
}

void SiCFurnaceSolver::apply_dirichlet_to_system(SparseMatrixCSR& K, Vector& rhs) {
    for (const auto& bc : dirichlet_bcs_) {
        for (index_t e = 0; e < mesh_->num_edges(); ++e) {
            const Edge& edge = mesh_->edge(e);
            if (edge.boundary_id == bc.boundary_id) {
                for (int i = 0; i < 2; ++i) {
                    K.apply_dirichlet(edge[i], bc.temperature, rhs);
                }
            }
        }
    }
}

void SiCFurnaceSolver::assemble_residual(const Vector& T, Vector& R) {
    index_t n = mesh_->num_nodes();
    R.assign(n, 0.0);
    
    SparseMatrixCSR& K = fem_->stiffness_matrix();
    K.multiply(T, R);
    
    for (index_t i = 0; i < n; ++i) {
        R[i] -= heat_source_[i];
    }
    
    if (params_.include_radiation) {
        Vector q_rad;
        rad_coupling_->compute_radiation_heat_flux(T, q_rad);
        for (index_t i = 0; i < n; ++i) {
            R[i] -= q_rad[i];
        }
    }
    
    for (const auto& bc : dirichlet_bcs_) {
        for (index_t e = 0; e < mesh_->num_edges(); ++e) {
            const Edge& edge = mesh_->edge(e);
            if (edge.boundary_id == bc.boundary_id) {
                for (int i = 0; i < 2; ++i) {
                    index_t node_id = edge[i];
                    R[node_id] = T[node_id] - bc.temperature;
                }
            }
        }
    }
}

void SiCFurnaceSolver::assemble_jacobian(const Vector& T, SparseMatrixCSR& J) {
    index_t n = mesh_->num_nodes();
    
    SparseMatrixCOO J_coo;
    J_coo.n_rows = n;
    J_coo.n_cols = n;
    
    SparseMatrixCSR& K = fem_->stiffness_matrix();
    for (index_t row = 0; row < n; ++row) {
        for (index_t j = K.row_ptr()[row]; j < K.row_ptr()[row + 1]; ++j) {
            J_coo.add(row, K.col_idx()[j], K.values()[j]);
        }
    }
    
    if (params_.include_radiation) {
        Vector R_base;
        assemble_residual(T, R_base);
        
        const auto& rad_node_ids = rad_coupling_->rad_node_ids();
        
        for (index_t ri = 0; ri < static_cast<index_t>(rad_node_ids.size()); ++ri) {
            index_t node_id = rad_node_ids[ri];
            
            Vector T_pert = T;
            real_t T_orig = T_pert[node_id];
            real_t dT = std::max(std::fabs(T_orig) * 1e-6, 1e-3);
            T_pert[node_id] = T_orig + dT;
            
            Vector R_pert;
            assemble_residual(T_pert, R_pert);
            
            for (index_t rj = 0; rj < static_cast<index_t>(rad_node_ids.size()); ++rj) {
                index_t j_id = rad_node_ids[rj];
                real_t dR_dT = (R_pert[j_id] - R_base[j_id]) / dT;
                J_coo.add(j_id, node_id, dR_dT);
            }
        }
    }
    
    J_coo.to_csr(J);
    
    for (const auto& bc : dirichlet_bcs_) {
        for (index_t e = 0; e < mesh_->num_edges(); ++e) {
            const Edge& edge = mesh_->edge(e);
            if (edge.boundary_id == bc.boundary_id) {
                for (int i = 0; i < 2; ++i) {
                    index_t node_id = edge[i];
                    Vector dummy_rhs(n, 0.0);
                    J.apply_dirichlet(node_id, 1.0, dummy_rhs);
                }
            }
        }
    }
}

bool SiCFurnaceSolver::solve_steady_state(SimulationResult& result) {
    if (!initialized_) initialize();
    
    if (params_.include_radiation && !vf_computed_) {
        compute_view_factors();
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    index_t n = mesh_->num_nodes();
    Vector T(n, params_.initial_temperature);
    
    result.temperature_history.clear();
    result.time_history.clear();
    
    if (!params_.include_radiation && !dirichlet_bcs_.empty()) {
        SparseMatrixCSR K_copy = fem_->stiffness_matrix();
        Vector rhs(n, 0.0);
        
        for (index_t i = 0; i < n; ++i) {
            rhs[i] = heat_source_[i];
        }
        
        apply_dirichlet_to_system(K_copy, rhs);
        
        auto solver = create_default_solver();
        bool converged = solver->solve(K_copy, rhs, T);
        
        result.converged = converged;
        result.newton_iterations = converged ? 1 : 0;
        result.newton_residuals = {0.0};
    } else {
        newton_->set_residual_func([this](const Vector& x, Vector& r) {
            assemble_residual(x, r);
        });
        
        newton_->set_jacobian_func([this](const Vector& x, SparseMatrixCSR& j) {
            assemble_jacobian(x, j);
        });
        
        bool converged = newton_->solve(T);
        
        result.converged = converged;
        result.newton_iterations = newton_->iterations();
        result.newton_residuals = newton_->residual_history();
    }
    
    result.temperature = T;
    
    result.max_temperature = T[0];
    result.min_temperature = T[0];
    for (real_t t : T) {
        result.max_temperature = std::max(result.max_temperature, t);
        result.min_temperature = std::min(result.min_temperature, t);
    }
    
    result.heat_flux_r.assign(n, 0.0);
    result.heat_flux_z.assign(n, 0.0);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.solve_time = std::chrono::duration<double>(end_time - start_time).count();
    
    return result.converged;
}

bool SiCFurnaceSolver::solve_transient(SimulationResult& result) {
    if (!initialized_) initialize();
    
    if (params_.include_radiation && !vf_computed_) {
        compute_view_factors();
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    index_t n = mesh_->num_nodes();
    Vector T(n, params_.initial_temperature);
    prev_T_transient_ = T;
    
    result.temperature_history.clear();
    result.time_history.clear();
    result.temperature_history.push_back(T);
    result.time_history.push_back(params_.time_start);
    
    result.converged = true;
    result.newton_iterations = 0;
    
    real_t t = params_.time_start;
    real_t dt = params_.time_step;
    int total_steps = static_cast<int>(std::ceil((params_.time_end - params_.time_start) / dt));
    
    newton_->set_residual_func([this, dt](const Vector& x, Vector& r) {
        index_t n = mesh_->num_nodes();
        
        Vector Kx(n);
        fem_->stiffness_matrix().multiply(x, Kx);
        
        Vector Mx_prev(n);
        fem_->mass_matrix().multiply(prev_T_transient_, Mx_prev);
        
        Vector Mx(n);
        fem_->mass_matrix().multiply(x, Mx);
        
        r.assign(n, 0.0);
        for (index_t i = 0; i < n; ++i) {
            r[i] = Kx[i] + (Mx[i] - Mx_prev[i]) / dt - heat_source_[i];
        }
        
        if (params_.include_radiation) {
            Vector q_rad;
            rad_coupling_->compute_radiation_heat_flux(x, q_rad);
            for (index_t i = 0; i < n; ++i) {
                r[i] -= q_rad[i];
            }
        }
        
        for (const auto& bc : dirichlet_bcs_) {
            for (index_t e = 0; e < mesh_->num_edges(); ++e) {
                const Edge& edge = mesh_->edge(e);
                if (edge.boundary_id == bc.boundary_id) {
                    for (int i = 0; i < 2; ++i) {
                        index_t node_id = edge[i];
                        r[node_id] = x[node_id] - bc.temperature;
                    }
                }
            }
        }
    });
    
    newton_->set_jacobian_func([this, dt](const Vector& x, SparseMatrixCSR& j) {
        index_t n = mesh_->num_nodes();
        
        SparseMatrixCOO J_coo;
        J_coo.n_rows = n;
        J_coo.n_cols = n;
        
        const auto& K = fem_->stiffness_matrix();
        const auto& M = fem_->mass_matrix();
        
        for (index_t row = 0; row < n; ++row) {
            for (index_t jj = K.row_ptr()[row]; jj < K.row_ptr()[row + 1]; ++jj) {
                J_coo.add(row, K.col_idx()[jj], K.values()[jj]);
            }
            for (index_t jj = M.row_ptr()[row]; jj < M.row_ptr()[row + 1]; ++jj) {
                J_coo.add(row, M.col_idx()[jj], M.values()[jj] / dt);
            }
        }
        
        if (params_.include_radiation) {
            Vector R_base;
            assemble_residual(x, R_base);
            
            const auto& rad_node_ids = rad_coupling_->rad_node_ids();
            
            for (index_t ri = 0; ri < static_cast<index_t>(rad_node_ids.size()); ++ri) {
                index_t node_id = rad_node_ids[ri];
                
                Vector T_pert = x;
                real_t T_orig = T_pert[node_id];
                real_t dT = std::max(std::fabs(T_orig) * 1e-6, 1e-3);
                T_pert[node_id] = T_orig + dT;
                
                Vector R_pert;
                assemble_residual(T_pert, R_pert);
                
                for (index_t rj = 0; rj < static_cast<index_t>(rad_node_ids.size()); ++rj) {
                    index_t j_id = rad_node_ids[rj];
                    real_t dR_dT = (R_pert[j_id] - R_base[j_id]) / dT;
                    J_coo.add(j_id, node_id, dR_dT);
                }
            }
        }
        
        J_coo.to_csr(j);
        
        for (const auto& bc : dirichlet_bcs_) {
            for (index_t e = 0; e < mesh_->num_edges(); ++e) {
                const Edge& edge = mesh_->edge(e);
                if (edge.boundary_id == bc.boundary_id) {
                    for (int i = 0; i < 2; ++i) {
                        index_t node_id = edge[i];
                        Vector dummy_rhs(n, 0.0);
                        j.apply_dirichlet(node_id, 1.0, dummy_rhs);
                    }
                }
            }
        }
    });
    
    for (int step = 0; step < total_steps; ++step) {
        prev_T_transient_ = T;
        
        bool converged = newton_->solve(T);
        
        if (!converged) {
            result.converged = false;
            break;
        }
        
        result.newton_iterations += newton_->iterations();
        
        t += dt;
        result.temperature_history.push_back(T);
        result.time_history.push_back(t);
    }
    
    result.temperature = T;
    
    result.max_temperature = T[0];
    result.min_temperature = T[0];
    for (real_t temp : T) {
        result.max_temperature = std::max(result.max_temperature, temp);
        result.min_temperature = std::min(result.min_temperature, temp);
    }
    
    result.heat_flux_r.assign(n, 0.0);
    result.heat_flux_z.assign(n, 0.0);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.solve_time = std::chrono::duration<double>(end_time - start_time).count();
    
    return result.converged;
}

} 
