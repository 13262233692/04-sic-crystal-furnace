#include "sic/radiation_coupling.h"
#include "sic/geometry.h"
#include <cmath>
#include <iostream>

namespace sic {

RadiationCoupling::RadiationCoupling(std::shared_ptr<Mesh2D> mesh,
                                     std::shared_ptr<ViewFactorCalculator> vf_calc)
    : mesh_(mesh), vf_calc_(vf_calc), T_ambient_(300.0), sigma_(STEFAN_BOLTZMANN) {
}

void RadiationCoupling::initialize() {
    build_rad_node_mapping();
    
    index_t n_rad = static_cast<index_t>(rad_node_ids_.size());
    
    rad_power_.assign(n_rad, 0.0);
    T4_vec_.assign(n_rad, 0.0);
    radiosity_.assign(n_rad, 0.0);
    irradiation_.assign(n_rad, 0.0);
}

void RadiationCoupling::build_rad_node_mapping() {
    rad_node_ids_.clear();
    
    index_t n_edges = static_cast<index_t>(vf_calc_->radiation_edges().size());
    std::set<index_t> node_set;
    
    for (index_t e = 0; e < n_edges; ++e) {
        const auto& re = vf_calc_->radiation_edges()[e];
        node_set.insert(re.v1);
        node_set.insert(re.v2);
    }
    
    for (index_t nid : node_set) {
        rad_node_ids_.push_back(nid);
    }
    
    index_t n_nodes = mesh_->num_nodes();
    node_to_rad_.assign(n_nodes, -1);
    
    for (index_t i = 0; i < static_cast<index_t>(rad_node_ids_.size()); ++i) {
        node_to_rad_[rad_node_ids_[i]] = i;
    }
}

void RadiationCoupling::compute_blackbody_emission(const Vector& T, Vector& Eb) const {
    index_t n = static_cast<index_t>(rad_node_ids_.size());
    Eb.resize(n);
    
    for (index_t i = 0; i < n; ++i) {
        real_t Ti = T[rad_node_ids_[i]];
        Eb[i] = sigma_ * Ti * Ti * Ti * Ti;
    }
}

void RadiationCoupling::compute_radiation_heat_flux(const Vector& temperature, 
                                                    Vector& heat_flux) {
    index_t n_nodes = mesh_->num_nodes();
    heat_flux.assign(n_nodes, 0.0);
    
    index_t n_rad = static_cast<index_t>(rad_node_ids_.size());
    
    Vector Eb;
    compute_blackbody_emission(temperature, Eb);
    
    const DenseMatrix& F = vf_calc_->view_factor_matrix();
    const auto& rad_edges = vf_calc_->radiation_edges();
    const auto& areas = vf_calc_->areas();
    
    index_t n_edges = static_cast<index_t>(rad_edges.size());
    
    Vector J(n_edges, 0.0);
    Vector q_net(n_edges, 0.0);
    
    for (index_t e = 0; e < n_edges; ++e) {
        const auto& re = rad_edges[e];
        
        index_t r1 = node_to_rad_[re.v1];
        index_t r2 = node_to_rad_[re.v2];
        
        real_t eps = 0.0;
        auto it = mesh_->regions().find(re.boundary_id);
        if (it != mesh_->regions().end()) {
            eps = it->second.material.emissivity;
        }
        if (eps < 1e-10) eps = 0.8;
        
        real_t Eb_e = 0.5 * (Eb[node_to_rad_[re.v1]] + Eb[node_to_rad_[re.v2]]);
        
        real_t sum_FJ = 0.0;
        for (index_t j = 0; j < n_edges; ++j) {
            sum_FJ += F(e, j) * J[j];
        }
        
        J[e] = eps * Eb_e + (1.0 - eps) * sum_FJ;
    }
    
    for (int iter = 0; iter < 20; ++iter) {
        Vector J_new = J;
        
        for (index_t e = 0; e < n_edges; ++e) {
            const auto& re = rad_edges[e];
            
            real_t eps = 0.8;
            auto it = mesh_->regions().find(re.boundary_id);
            if (it != mesh_->regions().end()) {
                eps = it->second.material.emissivity;
            }
            if (eps < 1e-10) eps = 0.8;
            
            real_t Eb_e = 0.5 * (Eb[node_to_rad_[re.v1]] + Eb[node_to_rad_[re.v2]]);
            
            real_t sum_FJ = 0.0;
            for (index_t j = 0; j < n_edges; ++j) {
                sum_FJ += F(e, j) * J[j];
            }
            
            J_new[e] = eps * Eb_e + (1.0 - eps) * sum_FJ;
        }
        
        real_t diff = 0.0;
        real_t max_J = 0.0;
        for (index_t e = 0; e < n_edges; ++e) {
            diff = std::max(diff, std::fabs(J_new[e] - J[e]));
            max_J = std::max(max_J, std::fabs(J[e]));
        }
        
        J = J_new;
        
        if (max_J > 0 && diff / max_J < 1e-8) {
            break;
        }
    }
    
    for (index_t e = 0; e < n_edges; ++e) {
        const auto& re = rad_edges[e];
        real_t eps = 0.8;
        auto it = mesh_->regions().find(re.boundary_id);
        if (it != mesh_->regions().end()) {
            eps = it->second.material.emissivity;
        }
        if (eps < 1e-10) eps = 0.8;
        
        real_t Eb_e = 0.5 * (Eb[node_to_rad_[re.v1]] + Eb[node_to_rad_[re.v2]]);
        
        real_t q_e = (eps / (1.0 - eps)) * (Eb_e - J[e]);
        if (!std::isfinite(q_e)) {
            q_e = Eb_e - J[e];
        }
        
        q_net[e] = q_e;
    }
    
    for (index_t e = 0; e < n_edges; ++e) {
        const auto& re = rad_edges[e];
        
        real_t len = re.length;
        real_t half_len = 0.5 * len;
        
        heat_flux[re.v1] += q_net[e] * half_len;
        heat_flux[re.v2] += q_net[e] * half_len;
    }
}

void RadiationCoupling::add_radiation_to_rhs(const Vector& temperature, Vector& rhs) {
    Vector q_rad;
    compute_radiation_heat_flux(temperature, q_rad);
    
    index_t n_nodes = mesh_->num_nodes();
    for (index_t i = 0; i < n_nodes; ++i) {
        rhs[i] += q_rad[i];
    }
}

void RadiationCoupling::add_radiation_to_jacobian(const Vector& temperature, 
                                                  SparseMatrixCOO& jac) {
    index_t n_nodes = mesh_->num_nodes();
    real_t pert = 1e-6;
    
    Vector q_plus;
    Vector q_minus;
    
    for (index_t ri = 0; ri < static_cast<index_t>(rad_node_ids_.size()); ++ri) {
        index_t node_id = rad_node_ids_[ri];
        
        Vector T_perturbed = temperature;
        real_t T_orig = T_perturbed[node_id];
        real_t dT = std::max(std::fabs(T_orig) * 1e-6, 1e-3);
        T_perturbed[node_id] = T_orig + dT;
        
        Vector q_pert;
        compute_radiation_heat_flux(T_perturbed, q_pert);
        
        Vector q_base;
        compute_radiation_heat_flux(temperature, q_base);
        
        for (index_t rj = 0; rj < static_cast<index_t>(rad_node_ids_.size()); ++rj) {
            index_t j_id = rad_node_ids_[rj];
            real_t dq_dT = (q_pert[j_id] - q_base[j_id]) / dT;
            jac.add(j_id, node_id, dq_dT);
        }
    }
}

real_t RadiationCoupling::compute_residual_norm(const Vector& temperature, 
                                                const Vector& rhs_heat) const {
    (void)temperature;
    (void)rhs_heat;
    return 0.0;
}

void RadiationCoupling::solve_radiosity(const Vector& T, Vector& J) const {
    (void)T;
    (void)J;
}

void RadiationCoupling::compute_net_radiation(const Vector& J, Vector& q) const {
    (void)J;
    (void)q;
}

void RadiationCoupling::compute_radiosity_derivative(const Vector& T,
                                                     const Vector& J,
                                                     DenseMatrix& dJ_dT) const {
    (void)T;
    (void)J;
    (void)dJ_dT;
}

} 
