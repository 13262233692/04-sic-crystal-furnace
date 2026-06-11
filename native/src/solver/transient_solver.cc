#include "sic/transient_solver.h"
#include <iostream>

namespace sic {

TransientSolver::TransientSolver()
    : dt_(1.0)
    , t_start_(0.0)
    , t_end_(1.0)
    , t_current_(0.0)
    , theta_(0.5)
    , step_count_(0)
    , save_interval_(1) {
    
    linear_solver_ = create_default_solver();
}

void TransientSolver::set_matrices(const SparseMatrixCSR& K, const SparseMatrixCSR& M) {
    K_ = K;
    M_ = M;
}

void TransientSolver::set_initial_condition(const Vector& T0) {
    T_ = T0;
    T_prev_ = T0;
    t_current_ = t_start_;
    step_count_ = 0;
    
    solution_hist_.clear();
    time_hist_.clear();
    
    solution_hist_.push_back(T0);
    time_hist_.push_back(t_start_);
}

void TransientSolver::assemble_effective_matrix() {
    index_t n = K_.rows();
    A_.resize(n, n);
    
    SparseMatrixCOO A_coo;
    A_coo.n_rows = n;
    A_coo.n_cols = n;
    
    for (index_t row = 0; row < n; ++row) {
        for (index_t j = K_.row_ptr()[row]; j < K_.row_ptr()[row + 1]; ++j) {
            index_t col = K_.col_idx()[j];
            real_t val = theta_ * K_.values()[j];
            A_coo.add(row, col, val);
        }
    }
    
    for (index_t row = 0; row < n; ++row) {
        for (index_t j = M_.row_ptr()[row]; j < M_.row_ptr()[row + 1]; ++j) {
            index_t col = M_.col_idx()[j];
            real_t val = M_.values()[j] / dt_;
            A_coo.add(row, col, val);
        }
    }
    
    A_coo.to_csr(A_);
}

void TransientSolver::compute_rhs(real_t t_n, real_t t_np1, Vector& rhs) {
    index_t n = K_.rows();
    rhs.assign(n, 0.0);
    
    Vector M_Tprev(n);
    M_.multiply(T_prev_, M_Tprev);
    
    for (index_t i = 0; i < n; ++i) {
        rhs[i] = M_Tprev[i] / dt_;
    }
    
    Vector f_n(n, 0.0);
    Vector f_np1(n, 0.0);
    
    if (rhs_func_) {
        rhs_func_(t_n, f_n);
        rhs_func_(t_np1, f_np1);
    }
    
    for (index_t i = 0; i < n; ++i) {
        rhs[i] += (1.0 - theta_) * f_n[i] + theta_ * f_np1[i];
    }
    
    Vector K_Tprev(n);
    K_.multiply(T_prev_, K_Tprev);
    for (index_t i = 0; i < n; ++i) {
        rhs[i] -= (1.0 - theta_) * K_Tprev[i];
    }
}

void TransientSolver::step() {
    real_t t_n = t_current_;
    real_t t_np1 = t_current_ + dt_;
    
    T_prev_ = T_;
    
    assemble_effective_matrix();
    compute_rhs(t_n, t_np1, rhs_);
    
    linear_solver_->solve(A_, rhs_, T_);
    
    t_current_ = t_np1;
    step_count_++;
    
    if (save_interval_ > 0 && step_count_ % save_interval_ == 0) {
        solution_hist_.push_back(T_);
        time_hist_.push_back(t_current_);
    }
}

void TransientSolver::solve() {
    t_current_ = t_start_;
    step_count_ = 0;
    
    solution_hist_.clear();
    time_hist_.clear();
    solution_hist_.push_back(T_);
    time_hist_.push_back(t_start_);
    
    while (t_current_ < t_end_ - 1e-12) {
        if (t_current_ + dt_ > t_end_) {
            dt_ = t_end_ - t_current_;
        }
        
        step();
    }
}

} 
