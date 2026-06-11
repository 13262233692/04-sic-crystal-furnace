import React, { useState, useEffect, useRef, useCallback } from 'react';
import './App.css';
import ThermalViewer from './components/ThermalViewer';
import ControlPanel from './components/ControlPanel';
import StatusBar from './components/StatusBar';
import ResultPanel from './components/ResultPanel';

const { Mesh2D, SiCFurnaceSolver } = require('bindings')('sic_furnace_solver');

function App() {
    const [mesh, setMesh] = useState(null);
    const [solver, setSolver] = useState(null);
    const [result, setResult] = useState(null);
    const [isSimulating, setIsSimulating] = useState(false);
    const [simulationType, setSimulationType] = useState('steady');
    const [params, setParams] = useState({
        includeRadiation: true,
        initialTemperature: 300,
        ambientTemperature: 300,
        crucibleTemp: 2500,
        outerWallTemp: 800,
        timeStart: 0,
        timeEnd: 3600,
        timeStep: 60,
        newtonTolerance: 1e-6,
        newtonMaxIter: 50,
        radiationQuadOrder: 4,
        heatSourcePower: 10000
    });

    useEffect(() => {
        createSampleMesh();
    }, []);

    const createSampleMesh = useCallback(() => {
        const m = new Mesh2D();
        
        const graphite_k_r = 100;
        const graphite_k_z = 100;
        const graphite_rho = 2200;
        const graphite_cp = 710;
        
        const carbonFelt_k_r = 0.5;
        const carbonFelt_k_z = 0.3;
        const carbonFelt_rho = 150;
        const carbonFelt_cp = 800;
        
        const sicPowder_k_r = 5;
        const sicPowder_k_z = 5;
        const sicPowder_rho = 3100;
        const sicPowder_cp = 1200;
        
        m.addRegion(0, {
            name: '石墨坩埚',
            k_r: graphite_k_r,
            k_z: graphite_k_z,
            rho: graphite_rho,
            cp: graphite_cp,
            emissivity: 0.85
        });
        m.addRegion(1, {
            name: '碳毡保温层',
            k_r: carbonFelt_k_r,
            k_z: carbonFelt_k_z,
            rho: carbonFelt_rho,
            cp: carbonFelt_cp,
            emissivity: 0.6
        });
        m.addRegion(2, {
            name: 'SiC粉料区',
            k_r: sicPowder_k_r,
            k_z: sicPowder_k_z,
            rho: sicPowder_rho,
            cp: sicPowder_cp,
            emissivity: 0.75
        });
        
        const r_inner = 0.05;
        const r_crucible = 0.07;
        const r_outer = 0.10;
        const z_bottom = 0;
        const z_top = 0.15;
        const z_seed = 0.12;
        
        const nr_inner = 6;
        const nr_mid = 8;
        const nr_outer = 6;
        const nz_total = 20;
        
        let nodeId = 0;
        const nodeMap = {};
        
        for (let i = 0; i <= nz_total; i++) {
            const z = z_bottom + (z_top - z_bottom) * i / nz_total;
            
            for (let j = 0; j <= nr_inner; j++) {
                const r = r_inner * j / nr_inner;
                if (z <= z_seed) {
                    nodeMap[`inner_${i}_${j}`] = nodeId;
                    m.addNode(r, z);
                    nodeId++;
                }
            }
            
            for (let j = 1; j <= nr_mid; j++) {
                const r = r_inner + (r_crucible - r_inner) * j / nr_mid;
                nodeMap[`crucible_${i}_${j}`] = nodeId;
                m.addNode(r, z);
                nodeId++;
            }
            
            for (let j = 1; j <= nr_outer; j++) {
                const r = r_crucible + (r_outer - r_crucible) * j / nr_outer;
                nodeMap[`insulation_${i}_${j}`] = nodeId;
                m.addNode(r, z);
                nodeId++;
            }
        }
        
        for (let i = 0; i < nz_total; i++) {
            for (let j = 0; j < nr_inner; j++) {
                if (i < nz_total * 0.8) {
                    const n00 = nodeMap[`inner_${i}_${j}`];
                    const n10 = nodeMap[`inner_${i}_${j+1}`];
                    const n01 = nodeMap[`inner_${i+1}_${j}`];
                    const n11 = nodeMap[`inner_${i+1}_${j+1}`];
                    
                    m.addElement(n00, n10, n11, 2);
                    m.addElement(n00, n11, n01, 2);
                }
            }
            
            for (let j = 0; j < nr_mid; j++) {
                const n00 = nodeMap[`crucible_${i}_${j}`];
                const n10 = nodeMap[`crucible_${i}_${j+1}`];
                const n01 = nodeMap[`crucible_${i+1}_${j}`];
                const n11 = nodeMap[`crucible_${i+1}_${j+1}`];
                
                m.addElement(n00, n10, n11, 0);
                m.addElement(n00, n11, n01, 0);
            }
            
            for (let j = 0; j < nr_outer; j++) {
                const n00 = nodeMap[`insulation_${i}_${j}`];
                const n10 = nodeMap[`insulation_${i}_${j+1}`];
                const n01 = nodeMap[`insulation_${i+1}_${j}`];
                const n11 = nodeMap[`insulation_${i+1}_${j+1}`];
                
                m.addElement(n00, n10, n11, 1);
                m.addElement(n00, n11, n01, 1);
            }
        }
        
        m.buildBoundaryEdges();
        
        const s = new SiCFurnaceSolver();
        s.setMesh(m);
        
        setMesh(m);
        setSolver(s);
        setResult(null);
    }, []);

    const runSimulation = useCallback(() => {
        if (!solver) return;
        
        setIsSimulating(true);
        setResult(null);
        
        setTimeout(() => {
            try {
                solver.setParams({
                    includeRadiation: params.includeRadiation,
                    steadyState: simulationType === 'steady',
                    initialTemperature: params.initialTemperature,
                    ambientTemperature: params.ambientTemperature,
                    timeStart: params.timeStart,
                    timeEnd: params.timeEnd,
                    timeStep: params.timeStep,
                    newtonTolerance: params.newtonTolerance,
                    newtonMaxIter: params.newtonMaxIter,
                    radiationQuadOrder: params.radiationQuadOrder
                });
                
                solver.setDirichletBC(0, params.crucibleTemp);
                solver.setHeatSource(0, params.heatSourcePower);
                solver.initialize();
                
                let simResult;
                if (simulationType === 'steady') {
                    simResult = solver.solveSteadyState();
                } else {
                    simResult = solver.solveTransient();
                }
                
                setResult(simResult);
            } catch (e) {
                console.error('Simulation error:', e);
            }
            setIsSimulating(false);
        }, 50);
    }, [solver, params, simulationType]);

    return (
        <div className="app-container">
            <header className="app-header">
                <div className="app-title">
                    <span className="logo-icon">⚛</span>
                    <h1>SiC 长晶炉热场模拟器</h1>
                    <span className="version">v1.0.0</span>
                </div>
                <div className="header-info">
                    <span className="info-badge">2500°C 级</span>
                    <span className="info-badge">PVT 法</span>
                </div>
            </header>
            
            <div className="app-main">
                <ControlPanel
                    params={params}
                    setParams={setParams}
                    simulationType={simulationType}
                    setSimulationType={setSimulationType}
                    onRunSimulation={runSimulation}
                    isSimulating={isSimulating}
                    mesh={mesh}
                />
                
                <div className="viewer-container">
                    <ThermalViewer
                        mesh={mesh}
                        result={result}
                        isSimulating={isSimulating}
                        simulationType={simulationType}
                    />
                </div>
                
                <ResultPanel result={result} params={params} />
            </div>
            
            <StatusBar
                mesh={mesh}
                result={result}
                isSimulating={isSimulating}
            />
        </div>
    );
}

export default App;
