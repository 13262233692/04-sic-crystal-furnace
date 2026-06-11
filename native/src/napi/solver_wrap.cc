#include <napi.h>
#include "sic/solver.h"
#include "sic/mesh2d.h"
#include <memory>

class Mesh2DWrap;

class SolverWrap : public Napi::ObjectWrap<SolverWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    SolverWrap(const Napi::CallbackInfo& info);
    
private:
    std::shared_ptr<sic::SiCFurnaceSolver> solver_;
    
    Napi::Value SetMesh(const Napi::CallbackInfo& info);
    Napi::Value SetParams(const Napi::CallbackInfo& info);
    Napi::Value Initialize(const Napi::CallbackInfo& info);
    Napi::Value SolveSteadyState(const Napi::CallbackInfo& info);
    Napi::Value SolveTransient(const Napi::CallbackInfo& info);
    Napi::Value SetDirichletBC(const Napi::CallbackInfo& info);
    Napi::Value SetHeatSource(const Napi::CallbackInfo& info);
    Napi::Value ComputeViewFactors(const Napi::CallbackInfo& info);
    Napi::Value GetViewFactorMatrix(const Napi::CallbackInfo& info);
    Napi::Value GetNumRadiationEdges(const Napi::CallbackInfo& info);
    Napi::Value GetRadiationEdges(const Napi::CallbackInfo& info);
};

Napi::Object SolverWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "SiCFurnaceSolver", {
        InstanceMethod("setMesh", &SolverWrap::SetMesh),
        InstanceMethod("setParams", &SolverWrap::SetParams),
        InstanceMethod("initialize", &SolverWrap::Initialize),
        InstanceMethod("solveSteadyState", &SolverWrap::SolveSteadyState),
        InstanceMethod("solveTransient", &SolverWrap::SolveTransient),
        InstanceMethod("setDirichletBC", &SolverWrap::SetDirichletBC),
        InstanceMethod("setHeatSource", &SolverWrap::SetHeatSource),
        InstanceMethod("computeViewFactors", &SolverWrap::ComputeViewFactors),
        InstanceMethod("getViewFactorMatrix", &SolverWrap::GetViewFactorMatrix),
        InstanceMethod("getNumRadiationEdges", &SolverWrap::GetNumRadiationEdges),
        InstanceMethod("getRadiationEdges", &SolverWrap::GetRadiationEdges)
    });
    
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);
    
    exports.Set("SiCFurnaceSolver", func);
    return exports;
}

SolverWrap::SolverWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<SolverWrap>(info) {
    solver_ = std::make_shared<sic::SiCFurnaceSolver>();
}

Napi::Value SolverWrap::SetMesh(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected Mesh2D object").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Object meshObj = info[0].As<Napi::Object>();
    auto* meshWrap = Napi::ObjectWrap<class Mesh2DWrap>::Unwrap(meshObj);
    
    auto mesh = reinterpret_cast<Mesh2DWrap*>(meshWrap)->GetMesh();
    solver_->set_mesh(mesh);
    
    return env.Undefined();
}

Napi::Value SolverWrap::SetParams(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected params object").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Object paramsObj = info[0].As<Napi::Object>();
    sic::SimulationParams params;
    
    if (paramsObj.Get("includeRadiation").IsBoolean()) {
        params.include_radiation = paramsObj.Get("includeRadiation").As<Napi::Boolean>().Value();
    }
    if (paramsObj.Get("steadyState").IsBoolean()) {
        params.steady_state = paramsObj.Get("steadyState").As<Napi::Boolean>().Value();
    }
    if (paramsObj.Get("initialTemperature").IsNumber()) {
        params.initial_temperature = paramsObj.Get("initialTemperature").As<Napi::Number>().DoubleValue();
    }
    if (paramsObj.Get("ambientTemperature").IsNumber()) {
        params.ambient_temperature = paramsObj.Get("ambientTemperature").As<Napi::Number>().DoubleValue();
    }
    if (paramsObj.Get("timeStart").IsNumber()) {
        params.time_start = paramsObj.Get("timeStart").As<Napi::Number>().DoubleValue();
    }
    if (paramsObj.Get("timeEnd").IsNumber()) {
        params.time_end = paramsObj.Get("timeEnd").As<Napi::Number>().DoubleValue();
    }
    if (paramsObj.Get("timeStep").IsNumber()) {
        params.time_step = paramsObj.Get("timeStep").As<Napi::Number>().DoubleValue();
    }
    if (paramsObj.Get("newtonTolerance").IsNumber()) {
        params.newton_tolerance = paramsObj.Get("newtonTolerance").As<Napi::Number>().DoubleValue();
    }
    if (paramsObj.Get("newtonMaxIter").IsNumber()) {
        params.newton_max_iter = paramsObj.Get("newtonMaxIter").As<Napi::Number>().Int32Value();
    }
    if (paramsObj.Get("radiationQuadOrder").IsNumber()) {
        params.radiation_quad_order = paramsObj.Get("radiationQuadOrder").As<Napi::Number>().Int32Value();
    }
    
    solver_->set_params(params);
    
    return env.Undefined();
}

Napi::Value SolverWrap::Initialize(const Napi::CallbackInfo& info) {
    solver_->initialize();
    return info.Env().Undefined();
}

Napi::Value SolverWrap::SolveSteadyState(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    sic::SimulationResult result;
    bool converged = solver_->solve_steady_state(result);
    
    Napi::Object resultObj = Napi::Object::New(env);
    
    int n = static_cast<int>(result.temperature.size());
    Napi::Float64Array tempArr = Napi::Float64Array::New(env, n);
    for (int i = 0; i < n; ++i) {
        tempArr[i] = result.temperature[i];
    }
    resultObj.Set("temperature", tempArr);
    
    Napi::Float64Array fluxR = Napi::Float64Array::New(env, n);
    Napi::Float64Array fluxZ = Napi::Float64Array::New(env, n);
    for (int i = 0; i < n; ++i) {
        fluxR[i] = result.heat_flux_r[i];
        fluxZ[i] = result.heat_flux_z[i];
    }
    resultObj.Set("heatFluxR", fluxR);
    resultObj.Set("heatFluxZ", fluxZ);
    
    resultObj.Set("maxTemperature", Napi::Number::New(env, result.max_temperature));
    resultObj.Set("minTemperature", Napi::Number::New(env, result.min_temperature));
    
    int nr = static_cast<int>(result.newton_residuals.size());
    Napi::Float64Array residuals = Napi::Float64Array::New(env, nr);
    for (int i = 0; i < nr; ++i) {
        residuals[i] = result.newton_residuals[i];
    }
    resultObj.Set("newtonResiduals", residuals);
    resultObj.Set("newtonIterations", Napi::Number::New(env, result.newton_iterations));
    resultObj.Set("converged", Napi::Boolean::New(env, result.converged));
    resultObj.Set("solveTime", Napi::Number::New(env, result.solve_time));
    
    return resultObj;
}

Napi::Value SolverWrap::SolveTransient(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    sic::SimulationResult result;
    bool converged = solver_->solve_transient(result);
    
    Napi::Object resultObj = Napi::Object::New(env);
    
    int n = static_cast<int>(result.temperature.size());
    Napi::Float64Array tempArr = Napi::Float64Array::New(env, n);
    for (int i = 0; i < n; ++i) {
        tempArr[i] = result.temperature[i];
    }
    resultObj.Set("temperature", tempArr);
    
    int nh = static_cast<int>(result.temperature_history.size());
    Napi::Array tempHist = Napi::Array::New(env, nh);
    for (int i = 0; i < nh; ++i) {
        Napi::Float64Array step = Napi::Float64Array::New(env, n);
        for (int j = 0; j < n; ++j) {
            step[j] = result.temperature_history[i][j];
        }
        tempHist.Set(i, step);
    }
    resultObj.Set("temperatureHistory", tempHist);
    
    Napi::Float64Array timeHist = Napi::Float64Array::New(env, nh);
    for (int i = 0; i < nh; ++i) {
        timeHist[i] = result.time_history[i];
    }
    resultObj.Set("timeHistory", timeHist);
    
    resultObj.Set("maxTemperature", Napi::Number::New(env, result.max_temperature));
    resultObj.Set("minTemperature", Napi::Number::New(env, result.min_temperature));
    resultObj.Set("newtonIterations", Napi::Number::New(env, result.newton_iterations));
    resultObj.Set("converged", Napi::Boolean::New(env, result.converged));
    resultObj.Set("solveTime", Napi::Number::New(env, result.solve_time));
    
    return resultObj;
}

Napi::Value SolverWrap::SetDirichletBC(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected (boundaryId, temperature)").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int boundary_id = info[0].As<Napi::Number>().Int32Value();
    double temperature = info[1].As<Napi::Number>().DoubleValue();
    
    solver_->set_dirichlet_bc(boundary_id, temperature);
    
    return env.Undefined();
}

Napi::Value SolverWrap::SetHeatSource(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected (regionId, powerDensity)").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int region_id = info[0].As<Napi::Number>().Int32Value();
    double power_density = info[1].As<Napi::Number>().DoubleValue();
    
    solver_->set_heat_source(region_id, power_density);
    
    return env.Undefined();
}

Napi::Value SolverWrap::ComputeViewFactors(const Napi::CallbackInfo& info) {
    solver_->compute_view_factors();
    return info.Env().Undefined();
}

Napi::Value SolverWrap::GetViewFactorMatrix(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    const auto& F = solver_->view_factor_matrix();
    int n = F.rows();
    
    Napi::Float64Array matrix = Napi::Float64Array::New(env, n * n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            matrix[i * n + j] = F(i, j);
        }
    }
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("rows", Napi::Number::New(env, n));
    result.Set("cols", Napi::Number::New(env, n));
    result.Set("data", matrix);
    
    return result;
}

Napi::Value SolverWrap::GetNumRadiationEdges(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), solver_->view_factor()->num_rad_edges());
}

Napi::Value SolverWrap::GetRadiationEdges(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    const auto& edges = solver_->view_factor()->radiation_edges();
    int n = static_cast<int>(edges.size());
    
    Napi::Array arr = Napi::Array::New(env, n);
    for (int i = 0; i < n; ++i) {
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("v1", Napi::Number::New(env, edges[i].v1));
        obj.Set("v2", Napi::Number::New(env, edges[i].v2));
        obj.Set("length", Napi::Number::New(env, edges[i].length));
        obj.Set("rAvg", Napi::Number::New(env, edges[i].r_avg));
        obj.Set("area", Napi::Number::New(env, edges[i].area));
        obj.Set("id", Napi::Number::New(env, edges[i].id));
        obj.Set("boundaryId", Napi::Number::New(env, edges[i].boundary_id));
        
        Napi::Object mid = Napi::Object::New(env);
        mid.Set("r", Napi::Number::New(env, edges[i].midpoint.r));
        mid.Set("z", Napi::Number::New(env, edges[i].midpoint.z));
        obj.Set("midpoint", mid);
        
        Napi::Object normal = Napi::Object::New(env);
        normal.Set("r", Napi::Number::New(env, edges[i].normal.r));
        normal.Set("z", Napi::Number::New(env, edges[i].normal.z));
        obj.Set("normal", normal);
        
        arr.Set(i, obj);
    }
    
    return arr;
}

Napi::Object InitSolver(Napi::Env env, Napi::Object exports) {
    return SolverWrap::Init(env, exports);
}
