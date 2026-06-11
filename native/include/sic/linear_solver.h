#ifndef SIC_LINEAR_SOLVER_H
#define SIC_LINEAR_SOLVER_H

#include "sic/types.h"
#include "sic/sparse_matrix.h"
#include "sic/dense_matrix.h"
#include <vector>
#include <memory>

namespace sic {

class LinearSolver {
public:
    virtual ~LinearSolver() = default;
    
    virtual bool solve(const SparseMatrixCSR& A, const Vector& b, Vector& x) = 0;
    virtual void set_tolerance(real_t tol) { tol_ = tol; }
    virtual void set_max_iterations(index_t maxit) { max_iter_ = maxit; }
    
protected:
    real_t tol_ = 1e-10;
    index_t max_iter_ = 1000;
};

class ConjugateGradient : public LinearSolver {
public:
    bool solve(const SparseMatrixCSR& A, const Vector& b, Vector& x) override;
};

class GaussSeidel : public LinearSolver {
public:
    bool solve(const SparseMatrixCSR& A, const Vector& b, Vector& x) override;
};

class DirectSolver {
public:
    static bool solve_dense(const DenseSymmetricMatrix& A, const Vector& b, Vector& x);
    static bool solve_sparse_ldlt(const SparseMatrixCSR& A, const Vector& b, Vector& x);
};

class BiCGSTAB : public LinearSolver {
public:
    bool solve(const SparseMatrixCSR& A, const Vector& b, Vector& x) override;
};

std::unique_ptr<LinearSolver> create_default_solver();

} 

#endif 
