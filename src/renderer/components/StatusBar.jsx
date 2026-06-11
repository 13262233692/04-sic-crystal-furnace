import React from 'react';

function StatusBar({ mesh, result, isSimulating }) {
    return (
        <div className="status-bar">
            <div className="status-item">
                <div className={`status-dot ${isSimulating ? 'running' : ''}`}></div>
                <span>{isSimulating ? '计算中' : '就绪'}</span>
            </div>
            
            {mesh && (
                <>
                    <div className="status-item">
                        <span>节点: {mesh.numNodes()}</span>
                    </div>
                    <div className="status-item">
                        <span>单元: {mesh.numElements()}</span>
                    </div>
                </>
            )}
            
            {result && (
                <>
                    <div className="status-item">
                        <span>Tmax: {result.maxTemperature.toFixed(1)} K</span>
                    </div>
                    <div className="status-item">
                        <span>Tmin: {result.minTemperature.toFixed(1)} K</span>
                    </div>
                    <div className="status-item">
                        <span>迭代: {result.newtonIterations}</span>
                    </div>
                </>
            )}
            
            <div style={{ marginLeft: 'auto', color: '#64748b' }}>
                SiC Crystal Furnace Thermal Simulator v1.0.0
            </div>
        </div>
    );
}

export default StatusBar;
