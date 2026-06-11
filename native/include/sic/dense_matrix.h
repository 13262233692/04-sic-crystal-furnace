#ifndef SIC_DENSE_MATRIX_H
#define SIC_DENSE_MATRIX_H

#include "sic/types.h"
#include <vector>
#include <stdexcept>

namespace sic {

class DenseMatrix {
public:
    DenseMatrix();
    DenseMatrix(index_t rows, index_t cols, real_t value = 0.0);
    
    void resize(index_t rows, index_t cols, real_t value = 0.0);
    
    index_t rows() const { return n_rows_; }
    index_t cols() const { return n_cols_; }
    
    real_t& operator()(index_t i, index_t j);
    real_t operator()(index_t i, index_t j) const;
    
    real_t& at(index_t i, index_t j);
    real_t at(index_t i, index_t j) const;
    
    void zero();
    void set_identity();
    
    void multiply(const Vector& x, Vector& y) const;
    void multiply_transpose(const Vector& x, Vector& y) const;
    
    const std::vector<real_t>& data() const { return data_; }
    std::vector<real_t>& data() { return data_; }
    
private:
    index_t n_rows_;
    index_t n_cols_;
    std::vector<real_t> data_;
};

class DenseSymmetricMatrix {
public:
    DenseSymmetricMatrix();
    explicit DenseSymmetricMatrix(index_t n, real_t value = 0.0);
    
    void resize(index_t n, real_t value = 0.0);
    
    index_t size() const { return n_; }
    
    real_t& operator()(index_t i, index_t j);
    real_t operator()(index_t i, index_t j) const;
    
    void zero();
    void set_identity();
    
    bool cholesky_factorize();
    void cholesky_solve(const Vector& rhs, Vector& x) const;
    
private:
    index_t n_;
    std::vector<real_t> L_;
    bool factorized_;
    
    index_t idx(index_t i, index_t j) const;
};

} 

#endif 
