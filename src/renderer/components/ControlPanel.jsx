import React from 'react';

function ControlPanel({ 
    params, 
    setParams, 
    simulationType, 
    setSimulationType,
    onRunSimulation, 
    isSimulating,
    mesh 
}) {
    const handleParamChange = (key, value) => {
        setParams(prev => ({
            ...prev,
            [key]: value
        }));
    };

    return (
        <div className="control-panel">
            <div className="panel-section">
                <h3>求解类型</h3>
                <div className="sim-type-tabs">
                    <div 
                        className={`sim-type-tab ${simulationType === 'steady' ? 'active' : ''}`}
                        onClick={() => setSimulationType('steady')}
                    >
                        稳态
                    </div>
                    <div 
                        className={`sim-type-tab ${simulationType === 'transient' ? 'active' : ''}`}
                        onClick={() => setSimulationType('transient')}
                    >
                        瞬态
                    </div>
                </div>
            </div>

            <div className="panel-section">
                <h3>热辐射</h3>
                <div className="checkbox-group">
                    <input
                        type="checkbox"
                        id="includeRadiation"
                        checked={params.includeRadiation}
                        onChange={(e) => handleParamChange('includeRadiation', e.target.checked)}
                    />
                    <label htmlFor="includeRadiation">考虑表面辐射换热</label>
                </div>
                <div className="form-group">
                    <label>环境温度 (K)</label>
                    <input
                        type="number"
                        value={params.ambientTemperature}
                        onChange={(e) => handleParamChange('ambientTemperature', parseFloat(e.target.value))}
                    />
                </div>
                <div className="form-group">
                    <label>辐射积分阶数</label>
                    <input
                        type="number"
                        value={params.radiationQuadOrder}
                        onChange={(e) => handleParamChange('radiationQuadOrder', parseInt(e.target.value))}
                        min={1}
                        max={7}
                    />
                </div>
            </div>

            <div className="panel-section">
                <h3>边界条件</h3>
                <div className="form-group">
                    <label>坩埚内壁温度 (K)</label>
                    <input
                        type="number"
                        value={params.crucibleTemp}
                        onChange={(e) => handleParamChange('crucibleTemp', parseFloat(e.target.value))}
                    />
                </div>
                <div className="form-group">
                    <label>初始温度 (K)</label>
                    <input
                        type="number"
                        value={params.initialTemperature}
                        onChange={(e) => handleParamChange('initialTemperature', parseFloat(e.target.value))}
                    />
                </div>
                <div className="form-group">
                    <label>加热功率密度 (W/m³)</label>
                    <input
                        type="number"
                        value={params.heatSourcePower}
                        onChange={(e) => handleParamChange('heatSourcePower', parseFloat(e.target.value))}
                    />
                </div>
            </div>

            {simulationType === 'transient' && (
                <div className="panel-section">
                    <h3>瞬态设置</h3>
                    <div className="form-group">
                        <label>起始时间 (s)</label>
                        <input
                            type="number"
                            value={params.timeStart}
                            onChange={(e) => handleParamChange('timeStart', parseFloat(e.target.value))}
                        />
                    </div>
                    <div className="form-group">
                        <label>结束时间 (s)</label>
                        <input
                            type="number"
                            value={params.timeEnd}
                            onChange={(e) => handleParamChange('timeEnd', parseFloat(e.target.value))}
                        />
                    </div>
                    <div className="form-group">
                        <label>时间步长 (s)</label>
                        <input
                            type="number"
                            value={params.timeStep}
                            onChange={(e) => handleParamChange('timeStep', parseFloat(e.target.value))}
                        />
                    </div>
                </div>
            )}

            <div className="panel-section">
                <h3>求解器设置</h3>
                <div className="form-group">
                    <label>牛顿收敛容差</label>
                    <input
                        type="number"
                        value={params.newtonTolerance}
                        onChange={(e) => handleParamChange('newtonTolerance', parseFloat(e.target.value))}
                        step="1e-8"
                    />
                </div>
                <div className="form-group">
                    <label>最大牛顿迭代数</label>
                    <input
                        type="number"
                        value={params.newtonMaxIter}
                        onChange={(e) => handleParamChange('newtonMaxIter', parseInt(e.target.value))}
                    />
                </div>
            </div>

            <div className="panel-section">
                <h3>材料区域</h3>
                <div className="region-legend">
                    <div className="region-item">
                        <div className="region-color" style={{ background: 'rgba(249, 115, 22, 0.6)' }}></div>
                        <span>石墨坩埚</span>
                    </div>
                    <div className="region-item">
                        <div className="region-color" style={{ background: 'rgba(100, 116, 139, 0.6)' }}></div>
                        <span>碳毡保温层</span>
                    </div>
                    <div className="region-item">
                        <div className="region-color" style={{ background: 'rgba(34, 197, 94, 0.6)' }}></div>
                        <span>SiC 粉料区</span>
                    </div>
                </div>
            </div>

            <div className="panel-section" style={{ marginTop: 'auto', borderTop: '1px solid #334155' }}>
                <button
                    className={`run-btn ${isSimulating ? 'running' : ''}`}
                    onClick={onRunSimulation}
                    disabled={isSimulating || !mesh}
                >
                    {isSimulating ? '⏳ 计算中...' : '▶ 开始计算'}
                </button>
            </div>
        </div>
    );
}

export default ControlPanel;
