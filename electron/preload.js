const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
  runSimulation: (params) => ipcRenderer.invoke('run-simulation', params),
  getMeshInfo: () => ipcRenderer.invoke('get-mesh-info'),
  loadMesh: (meshData) => ipcRenderer.invoke('load-mesh', meshData)
});
