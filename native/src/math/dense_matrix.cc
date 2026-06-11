#include "sic/dense_matrix.h"
#include <cmath>
#include <algorithm>

namespace sic {

DenseMatrix::DenseMatrix() : n_rows_(0), n_cols_(0) {}

DenseMatrix::DenseMatrix(index_t rows, index_t cols, real_t value)
    : n_rows_(rows), n_cols_(cols), data_(rows * cols, value) {}

void DenseMatrix::resize(index_t rows, index_t cols, real_t value) {
    n_rows_ = rows;
    n_cols_ = cols;
    data_.assign(rows * cols, value);
}

real_t& DenseMatrix::operator()(index_t i, index_t j) {
    return data_[i * n_cols_ + j];
}

real_t DenseMatrix::operator()(index_t i, index_t j) const {
    return data_[i * n_cols_ + j];
}

real_t& DenseMatrix::at(index_t i, index_t j) {
    if (i < 0 || i >= n_rows_ || j < 0 || j >= n_cols_) {
        throw std::out_of_range("DenseMatrix index out of range");
    }
    return data_[i * n_cols_ + j];
}

real_t DenseMatrix::at(index_t i, index_t j) const {
    if (i < 0 || i >= n_rows_ || j < 0 || j >= n_cols_) {
        throw std::out_of_range("DenseMatrix index out of range");
    }
    return data_[i * n_cols_ + j];
}

void DenseMatrix::zero() {
    std::fill(data_.begin(), data_.end(), 0.0);
}

void DenseMatrix::set_identity() {
    zero();
    index_t n = std::min(n_rows_, n_cols_);
    for (index_t i = 0; i < n; ++i) {
        data_[i * n_cols_ + i] = 1.0;
    }
}

void DenseMatrix::multiply(const Vector& x, Vector& y) const {
    y.assign(n_rows_, 0.0);
    for (index_t i = 0; i < n_rows_; ++i) {
        real_t sum = 0.0;
        for (index_t j = 0; j < n_cols_; ++j) {
            sum += data_[i * n_cols_ + j] * x[j];
        }
        y[i] = sum;
    }
}

void DenseMatrix::multiply_transpose(const Vector& x, Vector& y) const {
    y.assign(n_cols_, 0.0);
    for (index_t j = 0; j < n_cols_; ++j) {
        real_t sum = 0.0;
        for (index_t i = 0; i < n_rows_; ++i) {
            sum += data_[i * n_cols_ + j] * x[i];
        }
        y[j] = sum;
    }
}

DenseSymmetricMatrix::DenseSymmetricMatrix()
    : n_(0), factorized_(false) {}

DenseSymmetricMatrix::DenseSymmetricMatrix(index_t n, real_t value)
    : n_(n), L_(n * (n + 1) / 2, value), factorized_(false) {}

void DenseSymmetricMatrix::resize(index_t n, real_t value) {
    n_ = n;
    L_.assign(n * (n + 1) / 2, value);
    factorized_ = false;
}

index_t DenseSymmetricMatrix::idx(index_t i, index_t j) const {
    if (i >= j) {
        return i * (i + 1) / 2 + j;
    } else {
        return j * (j + 1) / 2 + i;
    }
}

real_t& DenseSymmetricMatrix::operator()(index_t i, index_t j) {
    factorized_ = false;
    return L_[idx(i, j)];
}

real_t DenseSymmetricMatrix::operator()(index_t i, index_t j) const {
    return L_[idx(i, j)];
}

void DenseSymmetricMatrix::zero() {
    std::fill(L_.begin(), L_.end(), 0.0);
    factorized_ = false;
}

void DenseSymmetricMatrix::set_identity() {
    zero();
    for (index_t i = 0; i < n_; ++i) {
        L_[idx(i, i)] = 1.0;
    }
    factorized_ = false;
}

bool DenseSymmetricMatrix::cholesky_factorize() {
    for (index_t j = 0; j < n_; ++j) {
        real_t sum = L_[idx(j, j)];
        for (index_t k = 0; k < j; ++k) {
            sum -= L_[idx(j, k)] * L_[idx(j, k)];
        }
        
        if (sum <= 0.0) return false;
        
        L_[idx(j, j)] = std::sqrt(sum);
        real_t inv_diag = 1.0 / L_[idx(j, j)];
        
        for (index_t i = j + 1; i < n_; ++i) {
            real_t sum2 = L_[idx(i, j)];
            for (index_t k = 0; k < j; ++k) {
                sum2 -= L_[idx(i, k)] * L_[idx(j, k)];
            }
            L_[idx(i, j)] = sum2 * inv_diag;
        }
    }
    
    factorized_ = true;
    return true;
}

void DenseSymmetricMatrix::cholesky_solve(const Vector& rhs, Vector& x) const {
    if (!factorized_) {
        const_cast<DenseSymmetricMatrix*>(this)->cholesky_factorize();
    }
    
    x.assign(n_, 0.0);
    
    for (index_t i = 0; i < n_; ++i) {
        real_t sum = rhs[i];
        for (index_t k = 0; k < i; ++k) {
            sum -= L_[idx(i, k)] * x[k];
        }
        x[i] = sum / L_[idx(i, i)];
    }
    
    for (index_t i = n_ - 1; i >= 0; --i) {
        real_t sum = x[i];
        for (index_t k = i + 1; k < n_; ++k) {
            sum -= L_[idx(k, i)] * x[k];
        }
        x[i] = sum / L_[idx(i, i)];
    }
}

} 
