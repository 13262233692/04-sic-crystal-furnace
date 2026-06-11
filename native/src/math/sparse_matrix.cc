#include "sic/sparse_matrix.h"
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace sic {

SparseMatrixCSR::SparseMatrixCSR()
    : n_rows_(0), n_cols_(0), assembled_(true) {}

SparseMatrixCSR::SparseMatrixCSR(index_t n_rows, index_t n_cols)
    : n_rows_(n_rows), n_cols_(n_cols), assembled_(true) {
    row_ptr_.resize(n_rows + 1, 0);
}

void SparseMatrixCSR::resize(index_t n_rows, index_t n_cols) {
    n_rows_ = n_rows;
    n_cols_ = n_cols;
    row_ptr_.assign(n_rows + 1, 0);
    col_idx_.clear();
    values_.clear();
    assembled_ = true;
}

void SparseMatrixCSR::reserve(index_t nnz) {
    col_idx_.reserve(nnz);
    values_.reserve(nnz);
}

void SparseMatrixCSR::begin_assembly() {
    assembled_ = false;
    temp_entries_.clear();
    temp_entries_.resize(n_rows_);
}

void SparseMatrixCSR::add(index_t row, index_t col, real_t value) {
    if (row < 0 || row >= n_rows_ || col < 0 || col >= n_cols_) {
        throw std::out_of_range("SparseMatrixCSR::add index out of range");
    }
    
    if (assembled_) {
        index_t idx = find_index(row, col);
        if (idx >= 0) {
            values_[idx] += value;
        } else {
            throw std::runtime_error("Adding to non-existing entry in assembled matrix");
        }
    } else {
        temp_entries_[row].emplace_back(col, value);
    }
}

void SparseMatrixCSR::end_assembly() {
    if (assembled_) return;
    
    col_idx_.clear();
    values_.clear();
    row_ptr_.assign(n_rows_ + 1, 0);
    
    for (index_t row = 0; row < n_rows_; ++row) {
        auto& entries = temp_entries_[row];
        std::sort(entries.begin(), entries.end(),
                  [](const std::pair<index_t, real_t>& a,
                     const std::pair<index_t, real_t>& b) {
                      return a.first < b.first;
                  });
        
        real_t sum = 0.0;
        index_t last_col = -1;
        
        for (auto& entry : entries) {
            if (entry.first == last_col) {
                sum += entry.second;
            } else {
                if (last_col >= 0) {
                    col_idx_.push_back(last_col);
                    values_.push_back(sum);
                }
                last_col = entry.first;
                sum = entry.second;
            }
        }
        if (last_col >= 0) {
            col_idx_.push_back(last_col);
            values_.push_back(sum);
        }
        
        row_ptr_[row + 1] = static_cast<index_t>(col_idx_.size());
    }
    
    temp_entries_.clear();
    assembled_ = true;
}

index_t SparseMatrixCSR::find_index(index_t row, index_t col) const {
    index_t start = row_ptr_[row];
    index_t end = row_ptr_[row + 1];
    
    for (index_t i = start; i < end; ++i) {
        if (col_idx_[i] == col) return i;
    }
    return -1;
}

void SparseMatrixCSR::set(index_t row, index_t col, real_t value) {
    index_t idx = find_index(row, col);
    if (idx >= 0) {
        values_[idx] = value;
    }
}

real_t SparseMatrixCSR::get(index_t row, index_t col) const {
    index_t idx = find_index(row, col);
    return (idx >= 0) ? values_[idx] : 0.0;
}

void SparseMatrixCSR::multiply(const Vector& x, Vector& y) const {
    if (x.size() != static_cast<size_t>(n_cols_)) {
        throw std::invalid_argument("SparseMatrixCSR::multiply size mismatch");
    }
    y.assign(n_rows_, 0.0);
    
    for (index_t row = 0; row < n_rows_; ++row) {
        real_t sum = 0.0;
        for (index_t j = row_ptr_[row]; j < row_ptr_[row + 1]; ++j) {
            sum += values_[j] * x[col_idx_[j]];
        }
        y[row] = sum;
    }
}

void SparseMatrixCSR::zero() {
    std::fill(values_.begin(), values_.end(), 0.0);
}

void SparseMatrixCSR::apply_dirichlet(index_t row, real_t value, Vector& rhs) {
    index_t diag_idx = -1;
    
    for (index_t j = row_ptr_[row]; j < row_ptr_[row + 1]; ++j) {
        index_t col = col_idx_[j];
        if (col == row) {
            diag_idx = j;
        }
        if (col != row) {
            rhs[col] -= values_[j] * value;
            values_[j] = 0.0;
        }
    }
    
    if (diag_idx >= 0) {
        values_[diag_idx] = 1.0;
    }
    
    rhs[row] = value;
}

void SparseMatrixCOO::to_csr(SparseMatrixCSR& csr) const {
    csr.resize(n_rows, n_cols);
    csr.begin_assembly();
    
    for (const auto& e : entries) {
        csr.add(e.row, e.col, e.value);
    }
    
    csr.end_assembly();
}

} 
