{
  "targets": [
    {
      "target_name": "sic_furnace_solver",
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "sources": [
        "native/src/napi/binding.cc",
        "native/src/napi/solver_wrap.cc",
        "native/src/napi/mesh_wrap.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "native/include"
      ],
      "defines": ["NAPI_CPP_EXCEPTIONS"],
      "conditions": [
        ["OS==\"win\"", {
          "sources": [
            "native/src/core/mesh2d.cc",
            "native/src/core/geometry.cc",
            "native/src/fem/thermal_fem.cc",
            "native/src/radiation/view_factor.cc",
            "native/src/radiation/radiation_coupling.cc",
            "native/src/solver/linear_solver.cc",
            "native/src/solver/newton_raphson.cc",
            "native/src/solver/transient_solver.cc",
            "native/src/solver/solver.cc",
            "native/src/math/sparse_matrix.cc",
            "native/src/math/dense_matrix.cc",
            "native/src/math/gauss_quadrature.cc"
          ]
        }]
      ]
    }
  ]
}
