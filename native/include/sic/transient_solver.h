#ifndef SIC_TRANSIENT_SOLVER_H
#define SIC_TRANSIENT_SOLVER_H

#include "sic/types.h"
#include "sic/sparse_matrix.h"
#include "sic/linear_solver.h"
#include <vector>
#include <memory>

namespace sic {

class TransientSolver {
public:
    TransientSolver();
    
    void set_matrices(const SparseMatrixCSR& K, const SparseMatrixCSR& M);
    void set_initial_condition(const Vector& T0);
    
    void set_time_step(real_t dt) { dt_ = dt; }
    real_t time_step() const { return dt_; }
    
    void set_start_time(real_t t) { t_start_ = t; }
    real_t start_time() const { return t_start_; }
    
    void set_end_time(real_t t) { t_end_ = t; }
    real_t end_time() const { return t_end_; }
    
    void set_theta(real_t theta) { theta_ = theta; }
    real_t theta() const { return theta_; }
    
    void set_linear_solver(std::unique_ptr<LinearSolver> solver) {
        linear_solver_ = std::move(solver);
    }
    
    void set_rhs_function(std::function<void(real_t t, Vector& f)> func) {
        rhs_func_ = std::move(func);
    }
    
    void solve();
    
    const Vector& solution() const { return T_; }
    const Vector& solution_prev() const { return T_prev_; }
    real_t current_time() const { return t_current_; }
    
    void step();
    
    const std::vector<Vector>& solution_history() const { return solution_hist_; }
    const std::vector<real_t>& time_history() const { return time_hist_; }
    
    void set_save_interval(index_t n) { save_interval_ = n; }
    
private:
    SparseMatrixCSR K_;
    SparseMatrixCSR M_;
    SparseMatrixCSR A_;
    
    Vector T_;
    Vector T_prev_;
    Vector rhs_;
    
    real_t dt_;
    real_t t_start_;
    real_t t_end_;
    real_t t_current_;
    real_t theta_;
    
    index_t step_count_;
    index_t save_interval_;
    
    std::unique_ptr<LinearSolver> linear_solver_;
    std::function<void(real_t t, Vector& f)> rhs_func_;
    
    std::vector<Vector> solution_hist_;
    std::vector<real_t> time_hist_;
    
    void assemble_effective_matrix();
    void compute_rhs(real_t t_n, real_t t_np1, Vector& rhs);
};

} 

#endif 
