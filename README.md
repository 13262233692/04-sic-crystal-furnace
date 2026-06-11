# SiC 长晶炉热场模拟器

> 面向第三代半导体材料制造的极硬核工业软件 —— 2500°C 级 SiC PVT 长晶炉内部复杂热场模拟系统

## 概述

本软件是一款专门针对碳化硅（SiC）物理气相传输（PVT）法晶体生长炉的热场数值模拟工具。采用 **Electron + React + C++ N-API** 三端架构，通过高精度有限元方法求解高温炉内的热传导与热辐射耦合问题。

## 核心功能

### 🔥 多域网格二维轴对称热传导离散
- 支持石墨坩埚、碳毡保温层、SiC 粉料区等多材料区域
- 各向异性热导率（径向 k_r、轴向 k_z）的有限元离散
- 严密组装刚度矩阵与热容质量矩阵
- 稳态与瞬态两种求解模式

### ☀️ 封闭腔体表面热辐射视角因子计算
- 自动提取所有暴露边界的线段
- 基于高斯积分的视角因子（View Factor）矩阵计算
- 遮挡测试（Shadowing Ray-casting）排除不可见表面
- 斯特藩-玻尔兹曼定律 T⁴ 高度非线性项
- 牛顿-拉夫逊法线性化耦合热传导方程组

### ⚡ 高性能求解器
- BiCGSTAB 迭代线性求解器
- 共轭梯度法 (CG) 备用
- 高斯-赛德尔迭代
- 牛顿-拉夫逊非线性求解器（带松弛因子）
- θ-法时间积分（Crank-Nicolson）

## 技术架构

```
┌─────────────────────────────────────────────────┐
│            Electron + React 前端                │
│  ┌─────────┐  ┌──────────┐  ┌─────────────┐   │
│  │ 控制面板 │  │ 可视化画布│  │ 结果面板    │   │
│  └─────────┘  └──────────┘  └─────────────┘   │
└───────────────────┬─────────────────────────────┘
                    │ IPC
┌───────────────────▼─────────────────────────────┐
│              Node.js 主进程 / N-API             │
│  ┌─────────┐  ┌──────────┐  ┌─────────────┐   │
│  │ Mesh2D  │  │  Solver  │  │  数据接口    │   │
│  └─────────┘  └──────────┘  └─────────────┘   │
└───────────────────┬─────────────────────────────┘
                    │ 原生绑定
┌───────────────────▼─────────────────────────────┐
│              C++ 计算内核                        │
│  ┌──────────────────────────────────────────┐   │
│  │  有限元热传导 (ThermalFEM)               │   │
│  │  视角因子计算 (ViewFactor)               │   │
│  │  辐射耦合 (RadiationCoupling)            │   │
│  │  牛顿-拉夫逊 (NewtonRaphson)             │   │
│  │  瞬态求解器 (TransientSolver)            │   │
│  └──────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
```

## 目录结构

```
04-sic-crystal-furnace/
├── electron/              # Electron 主进程
│   ├── main.js
│   └── preload.js
├── src/
│   ├── renderer/          # React 渲染进程
│   │   ├── components/
│   │   │   ├── ThermalViewer.jsx    # 温度场可视化
│   │   │   ├── ControlPanel.jsx     # 控制面板
│   │   │   ├── ResultPanel.jsx      # 结果面板
│   │   │   └── StatusBar.jsx        # 状态栏
│   │   ├── App.jsx
│   │   ├── App.css
│   │   ├── index.jsx
│   │   └── index.html
│   └── shared/            # 共享工具
│       └── sample_mesh.js
├── native/                # C++ 内核
│   ├── include/sic/       # 头文件
│   │   ├── types.h
│   │   ├── mesh2d.h
│   │   ├── geometry.h
│   │   ├── sparse_matrix.h
│   │   ├── dense_matrix.h
│   │   ├── gauss_quadrature.h
│   │   ├── thermal_fem.h
│   │   ├── view_factor.h
│   │   ├── radiation_coupling.h
│   │   ├── linear_solver.h
│   │   ├── newton_raphson.h
│   │   ├── transient_solver.h
│   │   └── solver.h
│   └── src/
│       ├── core/          # 核心数据结构
│       ├── math/          # 数学工具
│       ├── fem/           # 有限元
│       ├── radiation/     # 热辐射
│       ├── solver/        # 求解器
│       └── napi/          # N-API 绑定
├── test/                  # 测试脚本
├── package.json
├── binding.gyp            # 原生模块构建配置
└── webpack.renderer.js    # Webpack 配置
```

## 快速开始

### 前置依赖

- Node.js 16+
- Python 3.8+ (node-gyp)
- Windows: Visual Studio Build Tools (C++ 桌面开发)
- macOS: Xcode Command Line Tools
- Linux: build-essential

### 安装与构建

```bash
# 安装依赖
npm install

# 构建 C++ 原生模块
npm run build:addon

# 构建前端
npm run build:renderer

# 完整构建
npm run build
```

### 运行

```bash
# 开发模式
npm run dev

# 生产模式运行
npm start
```

### 运行测试

```bash
node test/test_solver.js
```

## 物理模型

### 热传导方程（2D 轴对称）

$$
\rho c_p \frac{\partial T}{\partial t} = \frac{1}{r}\frac{\partial}{\partial r}\left( r k_r \frac{\partial T}{\partial r} \right) + \frac{\partial}{\partial z}\left( k_z \frac{\partial T}{\partial z} \right) + q_v
$$

### 边界辐射换热

$$
q_{rad} = \sigma \epsilon (T^4 - T_{amb}^4)
$$

### 封闭腔辐射换热（视角因子法）

$$
J_i = \epsilon_i E_{b_i} + (1 - \epsilon_i) \sum_{j=1}^N F_{ij} J_j
$$

$$
q_i = \frac{\epsilon_i}{1 - \epsilon_i} (E_{b_i} - J_i)
$$

其中：
- $E_b = \sigma T^4$ — 黑体辐射力
- $J$ — 有效辐射（Radiosity）
- $F_{ij}$ — 视角因子
- $\sigma = 5.67 \times 10^{-8} \, \text{W/(m}^2\cdot\text{K}^4\text{)}$ — 斯特藩-玻尔兹曼常数

## 数值方法

### 有限元离散
- 三节点三角形单元 (P1)
- Galerkin 加权余量法
- 高斯数值积分（三角形、线段）
- 稀疏矩阵 CSR/COO 存储

### 非线性求解
- 牛顿-拉夫逊迭代
- 数值雅可比（有限差分）
- 可配置松弛因子
- 残差收敛准则

### 时间积分
- θ-方法（θ=0.5 为 Crank-Nicolson）
- 二阶精度（Crank-Nicolson）
- 无条件稳定

## API 接口

### JavaScript API

```javascript
const { Mesh2D, SiCFurnaceSolver } = require('bindings')('sic_furnace_solver');

// 创建网格
const mesh = new Mesh2D();
mesh.addNode(r, z);
mesh.addElement(v1, v2, v3, regionId);
mesh.addRegion(id, materialProperties);

// 配置求解器
const solver = new SiCFurnaceSolver();
solver.setMesh(mesh);
solver.setParams({ includeRadiation: true, ... });
solver.setDirichletBC(boundaryId, temperature);

// 求解
const result = solver.solveSteadyState();
console.log('最高温度:', result.maxTemperature);
```

## 材料参数（典型值）

| 材料 | 热导率 (W/m·K) | 密度 (kg/m³) | 比热容 (J/kg·K) | 发射率 |
|------|----------------|-------------|----------------|--------|
| 石墨 | 100-150 | 2200 | 710 | 0.85 |
| 碳毡 | 0.3-0.5 | 150 | 800 | 0.5-0.6 |
| SiC粉料 | 5-10 | 3100 | 1200 | 0.7-0.8 |
| SiC单晶 | 150-200 | 3210 | 1200 | 0.8 |

## 许可证

MIT License
