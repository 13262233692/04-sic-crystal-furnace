#include <napi.h>
#include "sic/mesh2d.h"
#include <memory>

class Mesh2DWrap : public Napi::ObjectWrap<Mesh2DWrap> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    Mesh2DWrap(const Napi::CallbackInfo& info);
    
    std::shared_ptr<sic::Mesh2D> GetMesh() { return mesh_; }
    
private:
    std::shared_ptr<sic::Mesh2D> mesh_;
    
    Napi::Value AddNode(const Napi::CallbackInfo& info);
    Napi::Value AddElement(const Napi::CallbackInfo& info);
    Napi::Value AddEdge(const Napi::CallbackInfo& info);
    Napi::Value AddRegion(const Napi::CallbackInfo& info);
    
    Napi::Value GetNumNodes(const Napi::CallbackInfo& info);
    Napi::Value GetNumElements(const Napi::CallbackInfo& info);
    Napi::Value GetNumEdges(const Napi::CallbackInfo& info);
    
    Napi::Value GetNode(const Napi::CallbackInfo& info);
    Napi::Value GetElement(const Napi::CallbackInfo& info);
    Napi::Value GetEdge(const Napi::CallbackInfo& info);
    
    Napi::Value GetNodesArray(const Napi::CallbackInfo& info);
    Napi::Value GetElementsArray(const Napi::CallbackInfo& info);
    Napi::Value GetEdgesArray(const Napi::CallbackInfo& info);
    
    Napi::Value BuildBoundaryEdges(const Napi::CallbackInfo& info);
    Napi::Value Clear(const Napi::CallbackInfo& info);
};

Napi::Object Mesh2DWrap::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "Mesh2D", {
        InstanceMethod("addNode", &Mesh2DWrap::AddNode),
        InstanceMethod("addElement", &Mesh2DWrap::AddElement),
        InstanceMethod("addEdge", &Mesh2DWrap::AddEdge),
        InstanceMethod("addRegion", &Mesh2DWrap::AddRegion),
        InstanceMethod("numNodes", &Mesh2DWrap::GetNumNodes),
        InstanceMethod("numElements", &Mesh2DWrap::GetNumElements),
        InstanceMethod("numEdges", &Mesh2DWrap::GetNumEdges),
        InstanceMethod("getNode", &Mesh2DWrap::GetNode),
        InstanceMethod("getElement", &Mesh2DWrap::GetElement),
        InstanceMethod("getEdge", &Mesh2DWrap::GetEdge),
        InstanceMethod("getNodesArray", &Mesh2DWrap::GetNodesArray),
        InstanceMethod("getElementsArray", &Mesh2DWrap::GetElementsArray),
        InstanceMethod("getEdgesArray", &Mesh2DWrap::GetEdgesArray),
        InstanceMethod("buildBoundaryEdges", &Mesh2DWrap::BuildBoundaryEdges),
        InstanceMethod("clear", &Mesh2DWrap::Clear)
    });
    
    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);
    
    exports.Set("Mesh2D", func);
    return exports;
}

Mesh2DWrap::Mesh2DWrap(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<Mesh2DWrap>(info) {
    mesh_ = std::make_shared<sic::Mesh2D>();
}

Napi::Value Mesh2DWrap::AddNode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected 2 arguments: r, z").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    double r = info[0].As<Napi::Number>().DoubleValue();
    double z = info[1].As<Napi::Number>().DoubleValue();
    
    mesh_->add_node(r, z);
    
    return Napi::Number::New(env, mesh_->num_nodes() - 1);
}

Napi::Value Mesh2DWrap::AddElement(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "Expected at least 3 arguments: v1, v2, v3").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int v1 = info[0].As<Napi::Number>().Int32Value();
    int v2 = info[1].As<Napi::Number>().Int32Value();
    int v3 = info[2].As<Napi::Number>().Int32Value();
    int region_id = info.Length() > 3 ? info[3].As<Napi::Number>().Int32Value() : 0;
    
    mesh_->add_element(v1, v2, v3, region_id);
    
    return Napi::Number::New(env, mesh_->num_elements() - 1);
}

Napi::Value Mesh2DWrap::AddEdge(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected at least 2 arguments: v1, v2").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int v1 = info[0].As<Napi::Number>().Int32Value();
    int v2 = info[1].As<Napi::Number>().Int32Value();
    int boundary_id = info.Length() > 2 ? info[2].As<Napi::Number>().Int32Value() : -1;
    bool exposed = info.Length() > 3 ? info[3].As<Napi::Boolean>().Value() : false;
    
    mesh_->add_edge(v1, v2, boundary_id, exposed);
    
    return Napi::Number::New(env, mesh_->num_edges() - 1);
}

Napi::Value Mesh2DWrap::AddRegion(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[1].IsObject()) {
        Napi::TypeError::New(env, "Expected (id, properties)").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int id = info[0].As<Napi::Number>().Int32Value();
    Napi::Object props = info[1].As<Napi::Object>();
    
    sic::MaterialProperties mat;
    mat.k_r = props.Get("k_r").IsNumber() ? props.Get("k_r").As<Napi::Number>().DoubleValue() : 1.0;
    mat.k_z = props.Get("k_z").IsNumber() ? props.Get("k_z").As<Napi::Number>().DoubleValue() : 1.0;
    mat.rho = props.Get("rho").IsNumber() ? props.Get("rho").As<Napi::Number>().DoubleValue() : 1.0;
    mat.cp = props.Get("cp").IsNumber() ? props.Get("cp").As<Napi::Number>().DoubleValue() : 1.0;
    mat.emissivity = props.Get("emissivity").IsNumber() ? props.Get("emissivity").As<Napi::Number>().DoubleValue() : 0.5;
    
    std::string name = props.Get("name").IsString() 
        ? props.Get("name").As<Napi::String>().Utf8Value() 
        : ("region_" + std::to_string(id));
    
    mesh_->add_region(id, name, mat);
    
    return env.Undefined();
}

Napi::Value Mesh2DWrap::GetNumNodes(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), mesh_->num_nodes());
}

Napi::Value Mesh2DWrap::GetNumElements(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), mesh_->num_elements());
}

Napi::Value Mesh2DWrap::GetNumEdges(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), mesh_->num_edges());
}

Napi::Value Mesh2DWrap::GetNode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected node index").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int idx = info[0].As<Napi::Number>().Int32Value();
    const auto& node = mesh_->node(idx);
    
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("r", Napi::Number::New(env, node.r));
    obj.Set("z", Napi::Number::New(env, node.z));
    
    return obj;
}

Napi::Value Mesh2DWrap::GetElement(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected element index").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int idx = info[0].As<Napi::Number>().Int32Value();
    const auto& elem = mesh_->element(idx);
    
    Napi::Object obj = Napi::Object::New(env);
    Napi::Array vertices = Napi::Array::New(env, 3);
    vertices.Set(0U, Napi::Number::New(env, elem[0]));
    vertices.Set(1U, Napi::Number::New(env, elem[1]));
    vertices.Set(2U, Napi::Number::New(env, elem[2]));
    obj.Set("vertices", vertices);
    obj.Set("regionId", Napi::Number::New(env, elem.region_id));
    
    return obj;
}

Napi::Value Mesh2DWrap::GetEdge(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected edge index").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int idx = info[0].As<Napi::Number>().Int32Value();
    const auto& edge = mesh_->edge(idx);
    
    Napi::Object obj = Napi::Object::New(env);
    Napi::Array vertices = Napi::Array::New(env, 2);
    vertices.Set(0U, Napi::Number::New(env, edge[0]));
    vertices.Set(1U, Napi::Number::New(env, edge[1]));
    obj.Set("vertices", vertices);
    obj.Set("boundaryId", Napi::Number::New(env, edge.boundary_id));
    obj.Set("isExposed", Napi::Boolean::New(env, edge.is_exposed));
    
    return obj;
}

Napi::Value Mesh2DWrap::GetNodesArray(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    int n = mesh_->num_nodes();
    Napi::Float64Array arr = Napi::Float64Array::New(env, n * 2);
    
    for (int i = 0; i < n; ++i) {
        const auto& node = mesh_->node(i);
        arr[i * 2] = node.r;
        arr[i * 2 + 1] = node.z;
    }
    
    return arr;
}

Napi::Value Mesh2DWrap::GetElementsArray(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    int n = mesh_->num_elements();
    Napi::Int32Array arr = Napi::Int32Array::New(env, n * 4);
    
    for (int i = 0; i < n; ++i) {
        const auto& elem = mesh_->element(i);
        arr[i * 4] = elem[0];
        arr[i * 4 + 1] = elem[1];
        arr[i * 4 + 2] = elem[2];
        arr[i * 4 + 3] = elem.region_id;
    }
    
    return arr;
}

Napi::Value Mesh2DWrap::GetEdgesArray(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    int n = mesh_->num_edges();
    Napi::Int32Array arr = Napi::Int32Array::New(env, n * 3);
    
    for (int i = 0; i < n; ++i) {
        const auto& edge = mesh_->edge(i);
        arr[i * 3] = edge[0];
        arr[i * 3 + 1] = edge[1];
        arr[i * 3 + 2] = edge.boundary_id;
    }
    
    return arr;
}

Napi::Value Mesh2DWrap::BuildBoundaryEdges(const Napi::CallbackInfo& info) {
    mesh_->build_boundary_edges();
    return info.Env().Undefined();
}

Napi::Value Mesh2DWrap::Clear(const Napi::CallbackInfo& info) {
    mesh_->clear();
    return info.Env().Undefined();
}

Napi::Object InitMesh(Napi::Env env, Napi::Object exports) {
    return Mesh2DWrap::Init(env, exports);
}
