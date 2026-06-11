import React from 'react';

function ResultPanel({ result, params }) {
    if (!result) {
        return (
            <div className="result-panel">
                <div className="panel-section">
                    <h3>结果信息</h3>
                    <div style={{ fontSize: '13px', color: '#64748b', textAlign: 'center', padding: '20px 0' }}>
                        运行模拟后将显示计算结果
                    </div>
                </div>
            </div>
        );
    }

    const formatTemp = (t) => {
        return t.toFixed(1) + ' K';
    };

    const formatTempC = (t) => {
        return (t - 273.15).toFixed(1) + ' °C';
    };

    return (
        <div className="result-panel">
            <div className="panel-section">
                <h3>温度极值</h3>
                <div className="result-stats">
                    <div className="stat-card hot">
                        <div className="stat-label">最高温度</div>
                        <div className="stat-value">{formatTemp(result.maxTemperature)}</div>
                        <div style={{ fontSize: '11px', color: '#94a3b8', marginTop: '2px' }}>
                            {formatTempC(result.maxTemperature)}
                        </div>
                    </div>
                    <div className="stat-card cold">
                        <div className="stat-label">最低温度</div>
                        <div className="stat-value">{formatTemp(result.minTemperature)}</div>
                        <div style={{ fontSize: '11px', color: '#94a3b8', marginTop: '2px' }}>
                            {formatTempC(result.minTemperature)}
                        </div>
                    </div>
                </div>
            </div>

            <div className="panel-section">
                <h3>求解信息</h3>
                <div style={{ fontSize: '13px', color: '#cbd5e1', lineHeight: '1.8' }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                        <span style={{ color: '#94a3b8' }}>求解状态:</span>
                        <span style={{ color: result.converged ? '#22c55e' : '#ef4444', fontWeight: '600' }}>
                            {result.converged ? '✓ 已收敛' : '✗ 未收敛'}
                        </span>
                    </div>
                    <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                        <span style={{ color: '#94a3b8' }}>牛顿迭代数:</span>
                        <span>{result.newtonIterations}</span>
                    </div>
                    <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                        <span style={{ color: '#94a3b8' }}>计算时间:</span>
                        <span>{(result.solveTime * 1000).toFixed(1)} ms</span>
                    </div>
                </div>

                {result.newtonResiduals && result.newtonResiduals.length > 0 && (
                    <div className={`convergence-info ${result.converged ? '' : 'failed'}`}>
                        <div className="conv-label">最终残差</div>
                        <div className="conv-value">
                            {result.newtonResiduals[result.newtonResiduals.length - 1].toExponential(4)}
                        </div>
                    </div>
                )}
            </div>

            <div className="panel-section">
                <h3>物理模型</h3>
                <div style={{ fontSize: '12px', color: '#94a3b8', lineHeight: '1.8' }}>
                    <div>✓ 2D 轴对称热传导</div>
                    <div>✓ 各向异性热导率</div>
                    <div>✓ 有限元 Galerkin 离散</div>
                    <div>✓ {params.includeRadiation ? '✓' : '○'} 表面热辐射</div>
                    <div>✓ 牛顿-拉夫逊线性化</div>
                    <div>✓ BiCGSTAB 迭代求解</div>
                </div>
            </div>

            {result.temperatureHistory && result.temperatureHistory.length > 0 && (
                <div className="panel-section">
                    <h3>时间步信息</h3>
                    <div style={{ fontSize: '13px', color: '#cbd5e1', lineHeight: '1.8' }}>
                        <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                            <span style={{ color: '#94a3b8' }}>时间步数:</span>
                            <span>{result.temperatureHistory.length}</span>
                        </div>
                        <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                            <span style={{ color: '#94a3b8' }}>总时间:</span>
                            <span>{((result.timeHistory?.[result.timeHistory.length - 1] || 0) / 3600).toFixed(2)} h</span>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
}

export default ResultPanel;
