#include <napi.h>
#include "sic/mesh2d.h"
#include "sic/solver.h"
#include <memory>

Napi::Object InitMesh(Napi::Env env, Napi::Object exports);
Napi::Object InitSolver(Napi::Env env, Napi::Object exports);

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    InitMesh(env, exports);
    InitSolver(env, exports);
    
    exports.Set(Napi::String::New(env, "version"), 
                Napi::String::New(env, "1.0.0"));
    exports.Set(Napi::String::New(env, "stefanBoltzmann"),
                Napi::Number::New(env, 5.670374419e-8));
    exports.Set(Napi::String::New(env, "absoluteZero"),
                Napi::Number::New(env, 273.15));
    
    return exports;
}
