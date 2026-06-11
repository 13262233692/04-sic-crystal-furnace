#include "sic/linear_solver.h"
#include <cmath>
#include <iostream>

namespace sic {

bool ConjugateGradient::solve(const SparseMatrixCSR& A, const Vector& b, Vector& x) {
    index_t n = A.rows();
    
    if (x.size() != static_cast<size_t>(n)) {
        x.assign(n, 0.0);
    }
    
    Vector r(n);
    Vector p(n);
    Vector Ap(n);
    
    A.multiply(x, r);
    for (index_t i = 0; i < n; ++i) {
        r[i] = b[i] - r[i];
    }
    
    p = r;
    
    real_t r_dot_old = 0.0;
    for (index_t i = 0; i < n; ++i) {
        r_dot_old += r[i] * r[i];
    }
    
    real_t b_norm = 0.0;
    for (index_t i = 0; i < n; ++i) {
        b_norm += b[i] * b[i];
    }
    b_norm = std::sqrt(b_norm);
    
    if (b_norm < 1e-30) {
        x.assign(n, 0.0);
        return true;
    }
    
    real_t tol_abs = tol_ * b_norm;
    
    for (index_t iter = 0; iter < max_iter_; ++iter) {
        A.multiply(p, Ap);
        
        real_t p_dot_Ap = 0.0;
        for (index_t i = 0; i < n; ++i) {
            p_dot_Ap += p[i] * Ap[i];
        }
        
        if (std::fabs(p_dot_Ap) < 1e-30) {
            break;
        }
        
        real_t alpha = r_dot_old / p_dot_Ap;
        
        for (index_t i = 0; i < n; ++i) {
            x[i] += alpha * p[i];
            r[i] -= alpha * Ap[i];
        }
        
        real_t r_dot_new = 0.0;
        for (index_t i = 0; i < n; ++i) {
            r_dot_new += r[i] * r[i];
        }
        
        real_t residual = std::sqrt(r_dot_new);
        if (residual < tol_abs) {
            return true;
        }
        
        real_t beta = r_dot_new / r_dot_old;
        
        for (index_t i = 0; i < n; ++i) {
            p[i] = r[i] + beta * p[i];
        }
        
        r_dot_old = r_dot_new;
    }
    
    return false;
}

bool GaussSeidel::solve(const SparseMatrixCSR& A, const Vector& b, Vector& x) {
    index_t n = A.rows();
    
    if (x.size() != static_cast<size_t>(n)) {
        x.assign(n, 0.0);
    }
    
    for (index_t iter = 0; iter < max_iter_; ++iter) {
        real_t max_diff = 0.0;
        
        for (index_t i = 0; i < n; ++i) {
            real_t sum = b[i];
            real_t diag = 0.0;
            
            for (index_t j = A.row_ptr()[i]; j < A.row_ptr()[i + 1]; ++j) {
                index_t col = A.col_idx()[j];
                real_t val = A.values()[j];
                
                if (col == i) {
                    diag = val;
                } else if (col < i) {
                    sum -= val * x[col];
                } else {
                    sum -= val * x[col];
                }
            }
            
            if (std::fabs(diag) < 1e-30) {
                return false;
            }
            
            real_t x_new = sum / diag;
            max_diff = std::max(max_diff, std::fabs(x_new - x[i]));
            x[i] = x_new;
        }
        
        if (max_diff < tol_) {
            return true;
        }
    }
    
    return false;
}

bool BiCGSTAB::solve(const SparseMatrixCSR& A, const Vector& b, Vector& x) {
    index_t n = A.rows();
    
    if (x.size() != static_cast<size_t>(n)) {
        x.assign(n, 0.0);
    }
    
    Vector r(n);
    A.multiply(x, r);
    for (index_t i = 0; i < n; ++i) {
        r[i] = b[i] - r[i];
    }
    
    Vector r0 = r;
    Vector p = r;
    Vector v(n, 0.0);
    Vector s(n);
    Vector t(n);
    
    real_t rho_prev = 1.0;
    real_t alpha = 1.0;
    real_t omega = 1.0;
    
    real_t b_norm = 0.0;
    for (index_t i = 0; i < n; ++i) {
        b_norm += b[i] * b[i];
    }
    b_norm = std::sqrt(b_norm);
    
    if (b_norm < 1e-30) {
        x.assign(n, 0.0);
        return true;
    }
    
    real_t tol_abs = tol_ * b_norm;
    
    for (index_t iter = 0; iter < max_iter_; ++iter) {
        real_t rho = 0.0;
        for (index_t i = 0; i < n; ++i) {
            rho += r0[i] * r[i];
        }
        
        real_t beta = (rho / rho_prev) * (alpha / omega);
        
        for (index_t i = 0; i < n; ++i) {
            p[i] = r[i] + beta * (p[i] - omega * v[i]);
        }
        
        A.multiply(p, v);
        
        real_t r0_dot_v = 0.0;
        for (index_t i = 0; i < n; ++i) {
            r0_dot_v += r0[i] * v[i];
        }
        
        if (std::fabs(r0_dot_v) < 1e-30) {
            rho_prev = rho;
            continue;
        }
        
        alpha = rho / r0_dot_v;
        
        for (index_t i = 0; i < n; ++i) {
            s[i] = r[i] - alpha * v[i];
        }
        
        A.multiply(s, t);
        
        real_t s_dot = 0.0;
        real_t t_dot = 0.0;
        real_t t_dot_s = 0.0;
        for (index_t i = 0; i < n; ++i) {
            s_dot += s[i] * s[i];
            t_dot += t[i] * t[i];
            t_dot_s += t[i] * s[i];
        }
        
        if (std::sqrt(s_dot) < tol_abs) {
            for (index_t i = 0; i < n; ++i) {
                x[i] += alpha * p[i];
            }
            return true;
        }
        
        if (t_dot < 1e-30) {
            omega = 0.0;
        } else {
            omega = t_dot_s / t_dot;
        }
        
        for (index_t i = 0; i < n; ++i) {
            x[i] += alpha * p[i] + omega * s[i];
            r[i] = s[i] - omega * t[i];
        }
        
        real_t r_norm = 0.0;
        for (index_t i = 0; i < n; ++i) {
            r_norm += r[i] * r[i];
        }
        r_norm = std::sqrt(r_norm);
        
        if (r_norm < tol_abs) {
            return true;
        }
        
        rho_prev = rho;
    }
    
    return false;
}

bool DirectSolver::solve_dense(const DenseSymmetricMatrix& A, const Vector& b, Vector& x) {
    DenseSymmetricMatrix Acopy = A;
    if (!Acopy.cholesky_factorize()) {
        return false;
    }
    Acopy.cholesky_solve(b, x);
    return true;
}

std::unique_ptr<LinearSolver> create_default_solver() {
    auto solver = std::make_unique<BiCGSTAB>();
    solver->set_tolerance(1e-10);
    solver->set_max_iterations(2000);
    return solver;
}

} 
