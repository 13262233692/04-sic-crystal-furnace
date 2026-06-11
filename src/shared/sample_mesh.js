const { Mesh2D } = require('bindings')('sic_furnace_solver');

function createSiCFurnaceMesh(options = {}) {
    const config = {
        crucibleInnerRadius: 0.05,
        crucibleOuterRadius: 0.07,
        insulationOuterRadius: 0.10,
        crucibleHeight: 0.15,
        seedPosition: 0.12,
        crucibleRadialDivisions: 6,
        insulationRadialDivisions: 5,
        axialDivisions: 20,
        ...options
    };

    const mesh = new Mesh2D();

    const graphiteProps = {
        name: '石墨坩埚',
        k_r: 120.0,
        k_z: 120.0,
        rho: 2200.0,
        cp: 710.0,
        emissivity: 0.85
    };

    const carbonFeltProps = {
        name: '碳毡保温层',
        k_r: 0.5,
        k_z: 0.3,
        rho: 150.0,
        cp: 800.0,
        emissivity: 0.55
    };

    const sicPowderProps = {
        name: 'SiC粉料区',
        k_r: 8.0,
        k_z: 8.0,
        rho: 3100.0,
        cp: 1200.0,
        emissivity: 0.75
    };

    const seedProps = {
        name: 'SiC籽晶',
        k_r: 150.0,
        k_z: 150.0,
        rho: 3210.0,
        cp: 1200.0,
        emissivity: 0.8
    };

    mesh.addRegion(0, graphiteProps);
    mesh.addRegion(1, carbonFeltProps);
    mesh.addRegion(2, sicPowderProps);
    mesh.addRegion(3, seedProps);

    const nodeMap = {};
    let nodeId = 0;

    const {
        crucibleInnerRadius: r_inner,
        crucibleOuterRadius: r_cruc,
        insulationOuterRadius: r_ins,
        crucibleHeight: z_top,
        seedPosition: z_seed,
        crucibleRadialDivisions: nr_cruc,
        insulationRadialDivisions: nr_ins,
        axialDivisions: nz
    } = config;

    const z_bottom = 0;

    for (let i = 0; i <= nz; i++) {
        const z = z_bottom + (z_top - z_bottom) * i / nz;

        const innerLayer = [];
        for (let j = 0; j <= nr_cruc; j++) {
            const r = r_inner * j / nr_cruc;
            innerLayer.push(nodeId);
            mesh.addNode(r, z);
            nodeId++;
        }
        nodeMap[`inner_${i}`] = innerLayer;

        const crucibleLayer = [];
        for (let j = 1; j <= nr_cruc; j++) {
            const r = r_inner + (r_cruc - r_inner) * j / nr_cruc;
            crucibleLayer.push(nodeId);
            mesh.addNode(r, z);
            nodeId++;
        }
        nodeMap[`crucible_${i}`] = crucibleLayer;

        const insulationLayer = [];
        for (let j = 1; j <= nr_ins; j++) {
            const r = r_cruc + (r_ins - r_cruc) * j / nr_ins;
            insulationLayer.push(nodeId);
            mesh.addNode(r, z);
            nodeId++;
        }
        nodeMap[`insulation_${i}`] = insulationLayer;
    }

    const seedIdx = Math.floor(z_seed / z_top * nz);

    for (let i = 0; i < nz; i++) {
        const inner0 = nodeMap[`inner_${i}`];
        const inner1 = nodeMap[`inner_${i + 1}`];

        if (i < seedIdx) {
            for (let j = 0; j < nr_cruc; j++) {
                const n00 = inner0[j];
                const n10 = inner0[j + 1];
                const n01 = inner1[j];
                const n11 = inner1[j + 1];

                mesh.addElement(n00, n10, n11, 2);
                mesh.addElement(n00, n11, n01, 2);
            }
        } else if (i === seedIdx) {
            for (let j = 0; j < nr_cruc; j++) {
                const n00 = inner0[j];
                const n10 = inner0[j + 1];
                const n01 = inner1[j];
                const n11 = inner1[j + 1];

                mesh.addElement(n00, n10, n11, 3);
                mesh.addElement(n00, n11, n01, 2);
            }
        } else {
            for (let j = 0; j < nr_cruc; j++) {
                const n00 = inner0[j];
                const n10 = inner0[j + 1];
                const n01 = inner1[j];
                const n11 = inner1[j + 1];

                mesh.addElement(n00, n10, n11, 0);
                mesh.addElement(n00, n11, n01, 0);
            }
        }

        const cruc0 = nodeMap[`crucible_${i}`];
        const cruc1 = nodeMap[`crucible_${i + 1}`];

        for (let j = 0; j < nr_cruc; j++) {
            const n00 = j === 0 ? inner0[inner0.length - 1] : cruc0[j - 1];
            const n10 = cruc0[j];
            const n01 = j === 0 ? inner1[inner1.length - 1] : cruc1[j - 1];
            const n11 = cruc1[j];

            mesh.addElement(n00, n10, n11, 0);
            mesh.addElement(n00, n11, n01, 0);
        }

        const ins0 = nodeMap[`insulation_${i}`];
        const ins1 = nodeMap[`insulation_${i + 1}`];

        for (let j = 0; j < nr_ins; j++) {
            const n00 = j === 0 ? cruc0[cruc0.length - 1] : ins0[j - 1];
            const n10 = ins0[j];
            const n01 = j === 0 ? cruc1[cruc1.length - 1] : ins1[j - 1];
            const n11 = ins1[j];

            mesh.addElement(n00, n10, n11, 1);
            mesh.addElement(n00, n11, n01, 1);
        }
    }

    mesh.buildBoundaryEdges();

    return {
        mesh,
        config,
        regions: {
            crucible: 0,
            insulation: 1,
            powder: 2,
            seed: 3
        }
    };
}

module.exports = { createSiCFurnaceMesh };
