#ifndef SIC_THERMAL_FEM_H
#define SIC_THERMAL_FEM_H

#include "sic/types.h"
#include "sic/mesh2d.h"
#include "sic/sparse_matrix.h"
#include <vector>
#include <memory>

namespace sic {

class ThermalFEM {
public:
    explicit ThermalFEM(std::shared_ptr<Mesh2D> mesh);
    
    void initialize();
    
    void assemble_stiffness_matrix();
    void assemble_mass_matrix();
    
    void apply_dirichlet_bc(index_t boundary_id, real_t temperature);
    void apply_neumann_bc(index_t boundary_id, real_t heat_flux);
    
    SparseMatrixCSR& stiffness_matrix() { return K_; }
    const SparseMatrixCSR& stiffness_matrix() const { return K_; }
    
    SparseMatrixCSR& mass_matrix() { return M_; }
    const SparseMatrixCSR& mass_matrix() const { return M_; }
    
    Vector& rhs() { return rhs_; }
    const Vector& rhs() const { return rhs_; }
    
    std::shared_ptr<Mesh2D> mesh() { return mesh_; }
    const std::shared_ptr<Mesh2D> mesh() const { return mesh_; }
    
    void compute_element_matrices(index_t elem_id,
                                  DenseMatrix& Ke,
                                  DenseMatrix& Me,
                                  Vector& fe) const;
    
    real_t compute_radial_circumference(real_t r) const;
    
private:
    std::shared_ptr<Mesh2D> mesh_;
    SparseMatrixCSR K_;
    SparseMatrixCSR M_;
    Vector rhs_;
    
    SparseMatrixCOO K_coo_;
    SparseMatrixCOO M_coo_;
    
    void compute_elem_stiffness(index_t elem_id, const MaterialProperties& mat,
                                DenseMatrix& Ke) const;
    void compute_elem_mass(index_t elem_id, const MaterialProperties& mat,
                           DenseMatrix& Me) const;
    void compute_elem_neumann(index_t edge_id, real_t flux,
                              Vector& fe) const;
    
    index_t boundary_elem_count_;
};

} 

#endif 
