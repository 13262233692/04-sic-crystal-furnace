import React, { useRef, useEffect, useState } from 'react';

function ThermalViewer({ mesh, result, isSimulating, simulationType }) {
    const canvasRef = useRef(null);
    const containerRef = useRef(null);
    const [canvasSize, setCanvasSize] = useState({ width: 800, height: 600 });
    const [timeIndex, setTimeIndex] = useState(0);
    const [isPlaying, setIsPlaying] = useState(false);
    const playIntervalRef = useRef(null);

    useEffect(() => {
        const updateSize = () => {
            if (containerRef.current) {
                const rect = containerRef.current.getBoundingClientRect();
                setCanvasSize({
                    width: Math.floor(rect.width),
                    height: Math.floor(rect.height)
                });
            }
        };

        updateSize();
        window.addEventListener('resize', updateSize);
        return () => window.removeEventListener('resize', updateSize);
    }, []);

    useEffect(() => {
        if (isPlaying && result && simulationType === 'transient' && result.temperatureHistory) {
            playIntervalRef.current = setInterval(() => {
                setTimeIndex(prev => {
                    const next = prev + 1;
                    if (next >= result.temperatureHistory.length) {
                        setIsPlaying(false);
                        return 0;
                    }
                    return next;
                });
            }, 100);
        } else {
            if (playIntervalRef.current) {
                clearInterval(playIntervalRef.current);
            }
        }

        return () => {
            if (playIntervalRef.current) {
                clearInterval(playIntervalRef.current);
            }
        };
    }, [isPlaying, result, simulationType]);

    useEffect(() => {
        if (!canvasRef.current || !mesh) return;

        const canvas = canvasRef.current;
        const ctx = canvas.getContext('2d');
        const dpr = window.devicePixelRatio || 1;

        canvas.width = canvasSize.width * dpr;
        canvas.height = canvasSize.height * dpr;
        ctx.scale(dpr, dpr);

        ctx.fillStyle = '#0c1220';
        ctx.fillRect(0, 0, canvasSize.width, canvasSize.height);

        drawGridBackground(ctx, canvasSize.width, canvasSize.height);

        const padding = 60;
        const viewWidth = canvasSize.width - padding * 2 - 80;
        const viewHeight = canvasSize.height - padding * 2;

        let rMin = 1e30, rMax = -1e30, zMin = 1e30, zMax = -1e30;
        const nodes = mesh.getNodesArray();
        const numNodes = mesh.numNodes();

        for (let i = 0; i < numNodes; i++) {
            const r = nodes[i * 2];
            const z = nodes[i * 2 + 1];
            rMin = Math.min(rMin, r);
            rMax = Math.max(rMax, r);
            zMin = Math.min(zMin, z);
            zMax = Math.max(zMax, z);
        }

        const rRange = rMax - rMin || 1;
        const zRange = zMax - zMin || 1;
        const scale = Math.min(viewWidth / rRange, viewHeight / zRange);

        const offsetX = padding + (viewWidth - rRange * scale) / 2;
        const offsetY = padding + (viewHeight - zRange * scale) / 2;

        const toScreen = (r, z) => ({
            x: offsetX + (r - rMin) * scale,
            y: canvasSize.height - offsetY - (z - zMin) * scale
        });

        if (result && result.temperature) {
            let temperatures;
            if (simulationType === 'transient' && result.temperatureHistory) {
                temperatures = result.temperatureHistory[timeIndex];
            } else {
                temperatures = result.temperature;
            }

            drawTemperatureField(ctx, mesh, temperatures, toScreen);
        }

        drawMesh(ctx, mesh, toScreen, !result);

        drawAxes(ctx, canvasSize, padding, rMin, rMax, zMin, zMax, scale);

        if (result && result.temperature) {
            drawColorbar(ctx, canvasSize, result);
        }
    }, [mesh, result, canvasSize, timeIndex, simulationType]);

    const drawGridBackground = (ctx, w, h) => {
        ctx.strokeStyle = 'rgba(100, 116, 139, 0.1)';
        ctx.lineWidth = 1;

        const gridSize = 50;
        for (let x = 0; x < w; x += gridSize) {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, h);
            ctx.stroke();
        }
        for (let y = 0; y < h; y += gridSize) {
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(w, y);
            ctx.stroke();
        }
    };

    const drawTemperatureField = (ctx, mesh, temperatures, toScreen) => {
        const elements = mesh.getElementsArray();
        const numElements = mesh.numElements();
        const nodes = mesh.getNodesArray();

        let tMin = Infinity, tMax = -Infinity;
        for (let i = 0; i < temperatures.length; i++) {
            tMin = Math.min(tMin, temperatures[i]);
            tMax = Math.max(tMax, temperatures[i]);
        }
        if (tMin === tMax) tMax = tMin + 1;

        for (let i = 0; i < numElements; i++) {
            const v1 = elements[i * 4];
            const v2 = elements[i * 4 + 1];
            const v3 = elements[i * 4 + 2];
            const regionId = elements[i * 4 + 3];

            const p1 = toScreen(nodes[v1 * 2], nodes[v1 * 2 + 1]);
            const p2 = toScreen(nodes[v2 * 2], nodes[v2 * 2 + 1]);
            const p3 = toScreen(nodes[v3 * 2], nodes[v3 * 2 + 1]);

            const t1 = temperatures[v1];
            const t2 = temperatures[v2];
            const t3 = temperatures[v3];
            const tAvg = (t1 + t2 + t3) / 3;

            const color = tempToColor(tAvg, tMin, tMax);

            ctx.fillStyle = color;
            ctx.beginPath();
            ctx.moveTo(p1.x, p1.y);
            ctx.lineTo(p2.x, p2.y);
            ctx.lineTo(p3.x, p3.y);
            ctx.closePath();
            ctx.fill();
        }
    };

    const drawMesh = (ctx, mesh, toScreen, showRegions) => {
        const elements = mesh.getElementsArray();
        const numElements = mesh.numElements();
        const nodes = mesh.getNodesArray();

        const regionColors = {
            0: 'rgba(249, 115, 22, 0.6)',
            1: 'rgba(100, 116, 139, 0.6)',
            2: 'rgba(34, 197, 94, 0.6)'
        };

        if (showRegions) {
            for (let i = 0; i < numElements; i++) {
                const v1 = elements[i * 4];
                const v2 = elements[i * 4 + 1];
                const v3 = elements[i * 4 + 2];
                const regionId = elements[i * 4 + 3];

                const p1 = toScreen(nodes[v1 * 2], nodes[v1 * 2 + 1]);
                const p2 = toScreen(nodes[v2 * 2], nodes[v2 * 2 + 1]);
                const p3 = toScreen(nodes[v3 * 2], nodes[v3 * 2 + 1]);

                ctx.fillStyle = regionColors[regionId] || 'rgba(100, 116, 139, 0.4)';
                ctx.beginPath();
                ctx.moveTo(p1.x, p1.y);
                ctx.lineTo(p2.x, p2.y);
                ctx.lineTo(p3.x, p3.y);
                ctx.closePath();
                ctx.fill();
            }
        }

        ctx.strokeStyle = 'rgba(148, 163, 184, 0.3)';
        ctx.lineWidth = 0.5;

        const edges = new Set();
        for (let i = 0; i < numElements; i++) {
            const v1 = elements[i * 4];
            const v2 = elements[i * 4 + 1];
            const v3 = elements[i * 4 + 2];

            const edgesArr = [
                [Math.min(v1, v2), Math.max(v1, v2)],
                [Math.min(v2, v3), Math.max(v2, v3)],
                [Math.min(v3, v1), Math.max(v3, v1)]
            ];

            for (const [a, b] of edgesArr) {
                const key = `${a}_${b}`;
                if (!edges.has(key)) {
                    edges.add(key);
                    const p1 = toScreen(nodes[a * 2], nodes[a * 2 + 1]);
                    const p2 = toScreen(nodes[b * 2], nodes[b * 2 + 1]);

                    ctx.beginPath();
                    ctx.moveTo(p1.x, p1.y);
                    ctx.lineTo(p2.x, p2.y);
                    ctx.stroke();
                }
            }
        }
    };

    const drawAxes = (ctx, size, padding, rMin, rMax, zMin, zMax, scale) => {
        ctx.strokeStyle = '#64748b';
        ctx.lineWidth = 2;

        ctx.beginPath();
        ctx.moveTo(padding, size.height - padding);
        ctx.lineTo(size.width - padding - 80, size.height - padding);
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(padding, size.height - padding);
        ctx.lineTo(padding, padding);
        ctx.stroke();

        ctx.fillStyle = '#94a3b8';
        ctx.font = '11px sans-serif';
        ctx.textAlign = 'center';

        const nTicks = 5;
        for (let i = 0; i <= nTicks; i++) {
            const t = i / nTicks;
            const r = rMin + t * (rMax - rMin);
            const x = padding + t * (rMax - rMin) * scale;

            ctx.beginPath();
            ctx.moveTo(x, size.height - padding);
            ctx.lineTo(x, size.height - padding + 5);
            ctx.strokeStyle = '#64748b';
            ctx.stroke();

            ctx.fillText((r * 1000).toFixed(0) + ' mm', x, size.height - padding + 18);
        }

        ctx.textAlign = 'right';
        for (let i = 0; i <= nTicks; i++) {
            const t = i / nTicks;
            const z = zMin + t * (zMax - zMin);
            const y = size.height - padding - t * (zMax - zMin) * scale;

            ctx.beginPath();
            ctx.moveTo(padding - 5, y);
            ctx.lineTo(padding, y);
            ctx.strokeStyle = '#64748b';
            ctx.stroke();

            ctx.fillText((z * 1000).toFixed(0) + ' mm', padding - 8, y + 4);
        }

        ctx.fillStyle = '#e2e8f0';
        ctx.font = '12px sans-serif';
        ctx.textAlign = 'center';
        ctx.fillText('径向 r', (size.width - 80) / 2, size.height - 15);

        ctx.save();
        ctx.translate(18, size.height / 2);
        ctx.rotate(-Math.PI / 2);
        ctx.fillText('轴向 z', 0, 0);
        ctx.restore();
    };

    const drawColorbar = (ctx, size, result) => {
        const cbWidth = 20;
        const cbHeight = 250;
        const cbX = size.width - 60;
        const cbY = (size.height - cbHeight) / 2;

        const gradient = ctx.createLinearGradient(cbX, cbY + cbHeight, cbX, cbY);
        gradient.addColorStop(0, '#1e3a8a');
        gradient.addColorStop(0.25, '#0891b2');
        gradient.addColorStop(0.5, '#22c55e');
        gradient.addColorStop(0.75, '#f97316');
        gradient.addColorStop(1, '#ef4444');

        ctx.fillStyle = gradient;
        ctx.fillRect(cbX, cbY, cbWidth, cbHeight);

        ctx.strokeStyle = '#475569';
        ctx.lineWidth = 1;
        ctx.strokeRect(cbX, cbY, cbWidth, cbHeight);

        ctx.fillStyle = '#94a3b8';
        ctx.font = '11px sans-serif';
        ctx.textAlign = 'left';

        const tMin = result.minTemperature || 300;
        const tMax = result.maxTemperature || 2500;

        const nTicks = 5;
        for (let i = 0; i <= nTicks; i++) {
            const t = i / nTicks;
            const temp = tMin + t * (tMax - tMin);
            const y = cbY + cbHeight - t * cbHeight;

            ctx.beginPath();
            ctx.moveTo(cbX + cbWidth, y);
            ctx.lineTo(cbX + cbWidth + 4, y);
            ctx.strokeStyle = '#64748b';
            ctx.stroke();

            ctx.fillText(temp.toFixed(0) + ' K', cbX + cbWidth + 8, y + 4);
        }

        ctx.save();
        ctx.translate(cbX + cbWidth + 30, cbY + cbHeight / 2);
        ctx.rotate(Math.PI / 2);
        ctx.fillStyle = '#cbd5e1';
        ctx.font = '12px sans-serif';
        ctx.textAlign = 'center';
        ctx.fillText('温度 (K)', 0, 0);
        ctx.restore();
    };

    const tempToColor = (temp, tMin, tMax) => {
        const t = Math.max(0, Math.min(1, (temp - tMin) / (tMax - tMin)));

        const colors = [
            [30, 58, 138],
            [8, 145, 178],
            [34, 197, 94],
            [249, 115, 22],
            [239, 68, 68]
        ];

        const n = colors.length - 1;
        const idx = Math.min(n - 1, Math.floor(t * n));
        const frac = t * n - idx;

        const c1 = colors[idx];
        const c2 = colors[idx + 1];

        const r = Math.round(c1[0] + frac * (c2[0] - c1[0]));
        const g = Math.round(c1[1] + frac * (c2[1] - c1[1]));
        const b = Math.round(c1[2] + frac * (c2[2] - c1[2]));

        return `rgb(${r}, ${g}, ${b})`;
    };

    const formatTime = (seconds) => {
        if (seconds < 60) return seconds.toFixed(0) + ' s';
        if (seconds < 3600) return (seconds / 60).toFixed(1) + ' min';
        return (seconds / 3600).toFixed(2) + ' h';
    };

    return (
        <div ref={containerRef} className="viewer-container" style={{ position: 'relative' }}>
            <canvas
                ref={canvasRef}
                className="viewer-canvas"
                style={{ width: canvasSize.width, height: canvasSize.height }}
            />

            {mesh && (
                <div className="mesh-info">
                    <div>节点数: <span>{mesh.numNodes()}</span></div>
                    <div>单元数: <span>{mesh.numElements()}</span></div>
                    <div>边界边: <span>{mesh.numEdges()}</span></div>
                </div>
            )}

            {isSimulating && (
                <div style={{
                    position: 'absolute',
                    top: '50%',
                    left: '50%',
                    transform: 'translate(-50%, -50%)',
                    background: 'rgba(15, 23, 42, 0.9)',
                    padding: '24px 40px',
                    borderRadius: '12px',
                    border: '1px solid #334155',
                    textAlign: 'center'
                }}>
                    <div style={{ fontSize: '18px', color: '#f1f5f9', marginBottom: '8px' }}>
                        ⚙ 正在求解...
                    </div>
                    <div style={{ fontSize: '13px', color: '#94a3b8' }}>
                        正在执行有限元热场计算
                    </div>
                </div>
            )}

            {simulationType === 'transient' && result && result.temperatureHistory && (
                <div className="timeline-slider">
                    <button 
                        className="play-btn"
                        onClick={() => setIsPlaying(!isPlaying)}
                    >
                        {isPlaying ? '⏸ 暂停' : '▶ 播放'}
                    </button>
                    <input
                        type="range"
                        min={0}
                        max={result.temperatureHistory.length - 1}
                        value={timeIndex}
                        onChange={(e) => setTimeIndex(parseInt(e.target.value))}
                    />
                    <div className="timeline-time">
                        {formatTime(result.timeHistory?.[timeIndex] || 0)}
                    </div>
                </div>
            )}
        </div>
    );
}

export default ThermalViewer;
