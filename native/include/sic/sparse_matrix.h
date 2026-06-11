#ifndef SIC_SPARSE_MATRIX_H
#define SIC_SPARSE_MATRIX_H

#include "sic/types.h"
#include <vector>
#include <algorithm>

namespace sic {

class SparseMatrixCSR {
public:
    SparseMatrixCSR();
    SparseMatrixCSR(index_t n_rows, index_t n_cols);
    
    void resize(index_t n_rows, index_t n_cols);
    void reserve(index_t nnz);
    
    void begin_assembly();
    void add(index_t row, index_t col, real_t value);
    void end_assembly();
    
    void set(index_t row, index_t col, real_t value);
    real_t get(index_t row, index_t col) const;
    
    index_t rows() const { return n_rows_; }
    index_t cols() const { return n_cols_; }
    index_t nnz() const { return values_.size(); }
    
    void multiply(const Vector& x, Vector& y) const;
    
    const std::vector<index_t>& row_ptr() const { return row_ptr_; }
    const std::vector<index_t>& col_idx() const { return col_idx_; }
    const std::vector<real_t>& values() const { return values_; }
    
    std::vector<index_t>& row_ptr() { return row_ptr_; }
    std::vector<index_t>& col_idx() { return col_idx_; }
    std::vector<real_t>& values() { return values_; }
    
    void zero();
    
    void apply_dirichlet(index_t row, real_t value, Vector& rhs);
    
private:
    index_t n_rows_;
    index_t n_cols_;
    std::vector<index_t> row_ptr_;
    std::vector<index_t> col_idx_;
    std::vector<real_t> values_;
    
    bool assembled_;
    std::vector<std::vector<std::pair<index_t, real_t>>> temp_entries_;
    
    index_t find_index(index_t row, index_t col) const;
};

class SparseMatrixCOO {
public:
    struct Entry {
        index_t row, col;
        real_t value;
        Entry(index_t r, index_t c, real_t v) : row(r), col(c), value(v) {}
    };
    
    std::vector<Entry> entries;
    index_t n_rows, n_cols;
    
    SparseMatrixCOO() : n_rows(0), n_cols(0) {}
    
    void add(index_t row, index_t col, real_t value) {
        entries.emplace_back(row, col, value);
    }
    
    void clear() { entries.clear(); }
    index_t nnz() const { return entries.size(); }
    
    void to_csr(SparseMatrixCSR& csr) const;
};

} 

#endif 
