#include "sic/newton_raphson.h"
#include <cmath>
#include <iostream>

namespace sic {

NewtonRaphson::NewtonRaphson()
    : tol_(1e-8)
    , max_iter_(50)
    , relaxation_(1.0)
    , iterations_(0)
    , final_residual_(0.0)
    , converged_(false) {
    
    linear_solver_ = create_default_solver();
}

real_t NewtonRaphson::compute_norm(const Vector& r) const {
    real_t sum = 0.0;
    for (real_t val : r) {
        sum += val * val;
    }
    return std::sqrt(sum);
}

bool NewtonRaphson::solve(Vector& x) {
    if (!residual_func_ || !jacobian_func_) {
        throw std::runtime_error("Newton-Raphson: residual or jacobian function not set");
    }
    
    iterations_ = 0;
    converged_ = false;
    residual_history_.clear();
    
    Vector R;
    SparseMatrixCSR J;
    
    residual_func_(x, R);
    real_t res_norm = compute_norm(R);
    residual_history_.push_back(res_norm);
    
    real_t initial_res = res_norm;
    if (initial_res < 1e-30) {
        final_residual_ = res_norm;
        converged_ = true;
        return true;
    }
    
    Vector dx(x.size(), 0.0);
    
    for (index_t iter = 0; iter < max_iter_; ++iter) {
        iterations_ = iter + 1;
        
        jacobian_func_(x, J);
        
        dx.assign(x.size(), 0.0);
        Vector neg_R(R.size());
        for (size_t i = 0; i < R.size(); ++i) {
            neg_R[i] = -R[i];
        }
        
        bool solve_ok = linear_solver_->solve(J, neg_R, dx);
        
        if (!solve_ok) {
            std::cerr << "Newton-Raphson: Linear solver failed at iteration " << iter << std::endl;
            final_residual_ = res_norm;
            return false;
        }
        
        for (size_t i = 0; i < x.size(); ++i) {
            x[i] += relaxation_ * dx[i];
        }
        
        residual_func_(x, R);
        res_norm = compute_norm(R);
        residual_history_.push_back(res_norm);
        
        real_t rel_res = res_norm / initial_res;
        
        if (res_norm < tol_ || rel_res < tol_) {
            converged_ = true;
            final_residual_ = res_norm;
            return true;
        }
    }
    
    final_residual_ = res_norm;
    converged_ = false;
    return false;
}

} 
