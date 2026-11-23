"use strict";

const NS = "http://www.w3.org/2000/svg";

const state = {
    dataset: "test",
    data: null,
    selectedPipe: null,
    failure: null,
    closures: null,
    positions: new Map(),
    zoom: 1,
    centerX: 500,
    centerY: 350,
    requestNumber: 0
};

const $ = id => document.getElementById(id);

function svgElement(tag, attributes = {}) {
    const element = document.createElementNS(NS, tag);

    for (const [key, value] of Object.entries(attributes)) {
        element.setAttribute(key, String(value));
    }

    return element;
}

function showStatus(message) {
    $("simulationStatus").textContent = message;
}

function setLoading(loading) {
    $("graphLoading").classList.toggle("hidden", !loading);
    $("simulateBtn").disabled = loading;
    $("resetBtn").disabled = loading;
    $("runScenarioBtn").disabled = loading;
}

function formatNumber(value) {
    return Number(value).toLocaleString(undefined, {
        maximumFractionDigits: 2
    });
}

async function loadNetwork() {
    const request = ++state.requestNumber;
    const params = new URLSearchParams({
        dataset: state.dataset,
        failures: state.failure || "-",
        closures: state.closures || "-"
    });

    setLoading(true);

    try {
        const response = await fetch(`/api/network?${params}`);

        const result = await response.json();

        if (!response.ok) {
            throw new Error(
                result.details || result.error ||
                "Network request failed"
            );
        }

        if (request !== state.requestNumber) return;

        state.data = result;

        const selectedStillExists = result.links.some(
            link => link.id === state.selectedPipe
        );

        if (!selectedStillExists) {
            state.selectedPipe = null;
        }

        updateMetrics();
        updatePipeList();
        updateDetails();
        renderGraph();

        showStatus(
            state.failure || state.closures
                ? "Scenario analyzed by C engine"
                : "Network operating normally"
        );

    } catch (error) {
        if (request === state.requestNumber) {
            showStatus(`Error: ${error.message}`);
            console.error(error);
        }
    } finally {
        if (request === state.requestNumber) {
            setLoading(false);
        }
    }
}

function updateMetrics() {
    const summary = state.data.summary;

    $("totalNodes").textContent =
        formatNumber(summary.total_nodes);

    $("totalLinks").textContent =
        formatNumber(summary.total_links);

    $("connectedNodes").textContent =
        formatNumber(summary.connected_nodes);

    $("disconnectedNodes").textContent =
        formatNumber(summary.disconnected_nodes);

    $("disconnectedDemand").textContent =
        formatNumber(summary.disconnected_demand);

    const percentage = summary.total_nodes
        ? 100 * summary.connected_nodes / summary.total_nodes
        : 0;

    $("connectivityBar").style.width = `${percentage}%`;

    $("connectivityText").textContent =
        `${percentage.toFixed(1)}% of network nodes reachable`;

    $("currentFailure").textContent =
        state.failure || "No active failure";

    $("datasetLabel").textContent =
        state.dataset === "net3"
            ? "Dataset: EPANET Net3"
            : "Dataset: Test Network";
}

function updatePipeList() {
    if (!state.data) return;

    const search = $("pipeSearch").value
        .trim()
        .toLowerCase();

    const select = $("pipeSelect");
    select.replaceChildren();

    const pipes = state.data.links.filter(link =>
        link.id.toLowerCase().includes(search)
    );

    for (const link of pipes) {
        const option = document.createElement("option");
        option.value = link.id;
        option.textContent =
            `${link.id} · ${link.source} → ${link.target}`;

        option.selected = link.id === state.selectedPipe;
        select.appendChild(option);
    }
}

function updateDetails() {
    if (!state.data) return;

    const pipe = state.data.links.find(
        link => link.id === state.selectedPipe
    );

    $("selectedPipe").textContent =
        pipe ? pipe.id : "None";

    const info = $("pipeInformation");
    info.replaceChildren();

    if (!pipe) {
        info.textContent =
            "Select a pipeline from the graph or dropdown.";
        return;
    }

    const fields = [
        ["From", pipe.source],
        ["To", pipe.target],
        ["Type", pipe.type],
        ["Status", pipe.status],
        ["Length", formatNumber(pipe.length)]
    ];

    for (const [label, value] of fields) {
        const row = document.createElement("div");
        const bold = document.createElement("b");

        bold.textContent = `${label}: `;
        row.appendChild(bold);
        row.appendChild(document.createTextNode(String(value)));

        info.appendChild(row);
    }
}

function calculatePositions() {
    const nodes = state.data.nodes;
    const valid = nodes.filter(node => node.has_coordinates);

    state.positions = new Map();

    let minX = 0;
    let maxX = 1;
    let minY = 0;
    let maxY = 1;

    if (valid.length >= 2) {
        minX = Math.min(...valid.map(n => n.x));
        maxX = Math.max(...valid.map(n => n.x));
        minY = Math.min(...valid.map(n => n.y));
        maxY = Math.max(...valid.map(n => n.y));
    }

    const width = Math.max(maxX - minX, 1);
    const height = Math.max(maxY - minY, 1);
    const scale = Math.min(880 / width, 580 / height);

    const occupiedWidth = width * scale;
    const occupiedHeight = height * scale;

    const left = (1000 - occupiedWidth) / 2;
    const top = (700 - occupiedHeight) / 2;

    nodes.forEach((node, index) => {
        if (node.has_coordinates && valid.length >= 2) {
            state.positions.set(node.id, {
                x: left + (node.x - minX) * scale,
                y: 700 - (top + (node.y - minY) * scale)
            });
        } else {
            const angle = 2 * Math.PI * index /
                          Math.max(nodes.length, 1);

            state.positions.set(node.id, {
                x: 500 + 290 * Math.cos(angle),
                y: 350 + 240 * Math.sin(angle)
            });
        }
    });
}

function updateViewBox() {
    const width = 1000 / state.zoom;
    const height = 700 / state.zoom;

    const x = state.centerX - width / 2;
    const y = state.centerY - height / 2;

    $("networkSvg").setAttribute(
        "viewBox",
        `${x} ${y} ${width} ${height}`
    );
}

function selectPipe(pipeId) {
    state.selectedPipe = pipeId;

    updatePipeList();
    updateDetails();
    renderGraph();
}

function renderGraph() {
    if (!state.data) return;

    calculatePositions();

    const svg = $("networkSvg");
    svg.replaceChildren();

    updateViewBox();

    const linkLayer = svgElement("g");
    const nodeLayer = svgElement("g");
    const labelLayer = svgElement("g");

    const nodesById = new Map(
        state.data.nodes.map(node => [node.id, node])
    );

    const showNodeLabels = $("labelsToggle").checked;
    const showPipeLabels = $("pipeLabelsToggle").checked;

    for (const link of state.data.links) {
        const start = state.positions.get(link.source);
        const end = state.positions.get(link.target);

        if (!start || !end) continue;

        const selected = state.selectedPipe === link.id;

        const line = svgElement("line", {
            x1: start.x,
            y1: start.y,
            x2: end.x,
            y2: end.y,
            class:
                "graph-link" +
                (link.status === "failed" ? " failed" : "") +
                (link.status === "closed" ? " closed" : "") +
                (selected ? " selected" : "")
        });

        const title = svgElement("title");
        title.textContent =
            `${link.id}: ${link.source} → ${link.target}`;
        line.appendChild(title);

        const hitTarget = svgElement("line", {
            x1: start.x,
            y1: start.y,
            x2: end.x,
            y2: end.y,
            stroke: "transparent",
            "stroke-width": 14,
            "pointer-events": "stroke",
            cursor: "pointer"
        });

        hitTarget.addEventListener(
            "click",
            () => selectPipe(link.id)
        );

        linkLayer.appendChild(line);
        linkLayer.appendChild(hitTarget);

        if (showPipeLabels) {
            const label = svgElement("text", {
                x: (start.x + end.x) / 2,
                y: (start.y + end.y) / 2 - 5,
                class: "graph-label",
                "text-anchor": "middle"
            });

            label.textContent = link.id;
            labelLayer.appendChild(label);
        }
    }

    for (const node of state.data.nodes) {
        const position = state.positions.get(node.id);

        const circle = svgElement("circle", {
            cx: position.x,
            cy: position.y,
            r: node.source ? 7 : 4.5,
            class:
                "graph-node" +
                (node.source ? " source" : "") +
                (!node.reachable ? " disconnected" : "")
        });

        const title = svgElement("title");
        title.textContent =
            `${node.id} | Demand: ${node.demand}`;

        circle.appendChild(title);
        nodeLayer.appendChild(circle);

        if (showNodeLabels) {
            const label = svgElement("text", {
                x: position.x + 8,
                y: position.y - 8,
                class: "graph-label"
            });

            label.textContent = node.id;
            labelLayer.appendChild(label);
        }
    }

    svg.appendChild(linkLayer);
    svg.appendChild(nodeLayer);
    svg.appendChild(labelLayer);
}

function changeZoom(factor) {
    state.zoom = Math.min(
        8,
        Math.max(.7, state.zoom * factor)
    );

    updateViewBox();
}

$("dataset").addEventListener("change", event => {
    state.dataset = event.target.value;
    state.failure = null;
    state.closures = null;
    $("multiFailures").value = "";
    $("closedLinks").value = "";
    state.selectedPipe = null;
    state.zoom = 1;
    $("pipeSearch").value = "";
    loadNetwork();
});

$("pipeSearch").addEventListener(
    "input",
    updatePipeList
);

$("pipeSelect").addEventListener("change", event => {
    selectPipe(event.target.value);
});

$("simulateBtn").addEventListener("click", () => {
    if (!state.selectedPipe) {
        showStatus("Select a pipeline first.");
        return;
    }

    state.failure = state.selectedPipe;
    state.closures = null;
    $("multiFailures").value = state.selectedPipe;
    $("closedLinks").value = "";
    loadNetwork();
});

$("runScenarioBtn").addEventListener("click", () => {
    const failures = $("multiFailures").value
        .split(",").map(x => x.trim()).filter(Boolean);

    const closures = $("closedLinks").value
        .split(",").map(x => x.trim()).filter(Boolean);

    if (!failures.length && !closures.length) {
        showStatus("Enter at least one failure or closure.");
        return;
    }

    const links = new Set(
        state.data.links.map(link => link.id)
    );

    const all = [...failures, ...closures];

    if (all.some(id => !links.has(id))) {
        showStatus("One or more link IDs are invalid.");
        return;
    }

    if (new Set(all).size !== all.length) {
        showStatus("Link IDs must not repeat or conflict.");
        return;
    }

    state.failure = failures.join(",") || null;
    state.closures = closures.join(",") || null;

    loadNetwork();
});

$("resetBtn").addEventListener("click", () => {
    state.failure = null;
    state.closures = null;
    $("multiFailures").value = "";
    $("closedLinks").value = "";
    loadNetwork();
});

$("labelsToggle").addEventListener(
    "change",
    renderGraph
);

$("pipeLabelsToggle").addEventListener(
    "change",
    renderGraph
);

$("zoomIn").addEventListener(
    "click",
    () => changeZoom(1.3)
);

$("zoomOut").addEventListener(
    "click",
    () => changeZoom(1 / 1.3)
);

$("fitBtn").addEventListener("click", () => {
    state.zoom = 1;
    state.centerX = 500;
    state.centerY = 350;
    updateViewBox();
});

loadNetwork();
