#ifndef SIC_NEWTON_RAPHSON_H
#define SIC_NEWTON_RAPHSON_H

#include "sic/types.h"
#include "sic/sparse_matrix.h"
#include "sic/linear_solver.h"
#include <vector>
#include <memory>
#include <functional>

namespace sic {

using ResidualFunc = std::function<void(const Vector& x, Vector& r)>;
using JacobianFunc = std::function<void(const Vector& x, SparseMatrixCSR& J)>;

class NewtonRaphson {
public:
    NewtonRaphson();
    
    void set_residual_func(ResidualFunc func) { residual_func_ = std::move(func); }
    void set_jacobian_func(JacobianFunc func) { jacobian_func_ = std::move(func); }
    
    void set_tolerance(real_t tol) { tol_ = tol; }
    real_t tolerance() const { return tol_; }
    
    void set_max_iterations(index_t maxit) { max_iter_ = maxit; }
    index_t max_iterations() const { return max_iter_; }
    
    void set_relaxation(real_t omega) { relaxation_ = omega; }
    real_t relaxation() const { return relaxation_; }
    
    void set_linear_solver(std::unique_ptr<LinearSolver> solver) {
        linear_solver_ = std::move(solver);
    }
    
    bool solve(Vector& x);
    
    index_t iterations() const { return iterations_; }
    real_t final_residual() const { return final_residual_; }
    bool converged() const { return converged_; }
    
    const std::vector<real_t>& residual_history() const { return residual_history_; }
    
private:
    ResidualFunc residual_func_;
    JacobianFunc jacobian_func_;
    
    real_t tol_;
    index_t max_iter_;
    real_t relaxation_;
    
    std::unique_ptr<LinearSolver> linear_solver_;
    
    index_t iterations_;
    real_t final_residual_;
    bool converged_;
    std::vector<real_t> residual_history_;
    
    real_t compute_norm(const Vector& r) const;
};

} 

#endif 
