"use strict";

(function () {
    const panel = document.createElement("section");
    panel.className = "comparison-panel";
    panel.id = "optimizerPanel";

    panel.innerHTML = `
      <div class="comparison-heading">
        <div>
          <h3>Valve Isolation & Resilience Optimization</h3>
          <p>Person 2's C optimization engine</p>
        </div>
      </div>

      <div class="optimizer-controls">
        <label>Network
          <select id="optDataset">
            <option value="demo">Resilience Demo (Verified Scenario)</option>
            <option value="sample">Sample Network</option>
            <option value="image3">Image 3 Network</option>
          </select>
        </label>

        <label>Failed pipe IDs
          <input id="optFailure" value="P2"
                 placeholder="P5 or P3,P7">
        </label>

        <label>Stuck-open valve IDs
          <input id="optStuck" placeholder="V1">
        </label>

        <label>New valve budget
          <input id="optBudget" type="number"
                 min="0" max="20" value="3">
        </label>

        <button id="runOptimization" class="report-button">
          Run Optimization
        </button>
      </div>

      <p id="optStatus" class="status">
        Ready to run optimization.
      </p>

      <div class="comparison-grid" id="optMetrics"></div>

      <div class="comparison-bottom">
        <div>
          <h4>Isolation & Structural Analysis</h4>
          <div id="optIsolation" class="scenario-info">
            No results yet.
          </div>
        </div>
        <div>
          <h4>Recommended Valves</h4>
          <div id="optPlacement" class="scenario-info">
            No results yet.
          </div>
        </div>
      </div>

      <h4>Optimized Network Visualization</h4>
      <p class="optimizer-description">
        Red = failed pipeline · Turquoise = proposed new valve ·
        Orange = existing valve · Green = water source
      </p>
      <div class="optimizer-network-wrap">
        <svg id="optimizedNetwork"
             viewBox="0 0 1000 560"
             xmlns="http://www.w3.org/2000/svg"></svg>
      </div>

      <h4>Algorithm Comparison</h4>
      <div id="optAlgorithms" class="optimizer-results"></div>
    `;

    const anchor = document.getElementById("comparisonPanel");
    const main = document.querySelector("main");

    if (anchor) anchor.insertAdjacentElement("afterend", panel);
    else main.appendChild(panel);

    const $ = id => document.getElementById(id);
    const fmt = value =>
        typeof value === "number"
            ? value.toLocaleString(undefined, {
                maximumFractionDigits: 3
            })
            : String(value ?? "N/A");

    function writeText(id, lines) {
        $(id).textContent = lines.join("\n");
    }


    function drawOptimizedNetwork(data) {
        const svg = $("optimizedNetwork");
        if (!svg) return;

        svg.replaceChildren();

        const graph = data.graph || {};
        const nodes = graph.nodes || [];
        const links = graph.links || [];

        if (!nodes.length) return;

        const optimization = data.optimization || {};
        const greedy = optimization.greedy || {};

        const selected = new Set(greedy.placed_valves || []);
        const failed = new Set(
            (data.isolation || {}).failed_links || []
        );
        const closed = new Set(
            (data.isolation || {}).valves_to_close || []
        );

        const NS = "http://www.w3.org/2000/svg";

        function element(tag, attrs = {}) {
            const node = document.createElementNS(NS, tag);

            for (const [key, value] of Object.entries(attrs)) {
                node.setAttribute(key, value);
            }

            return node;
        }

        const xs = nodes.map(n => Number(n.x) || 0);
        const ys = nodes.map(n => Number(n.y) || 0);

        const minX = Math.min(...xs);
        const maxX = Math.max(...xs);
        const minY = Math.min(...ys);
        const maxY = Math.max(...ys);

        const width = Math.max(maxX - minX, 1);
        const height = Math.max(maxY - minY, 1);

        const scale = Math.min(850 / width, 430 / height);

        const offsetX = (1000 - width * scale) / 2;
        const offsetY = (560 - height * scale) / 2;

        const positions = new Map();

        nodes.forEach((node, index) => {
            const hasPosition =
                width > 1 || height > 1;

            const angle =
                2 * Math.PI * index / nodes.length;

            positions.set(node.id, hasPosition ? {
                x: offsetX + (Number(node.x) - minX) * scale,
                y: 560 - (
                    offsetY + (Number(node.y) - minY) * scale
                )
            } : {
                x: 500 + 200 * Math.cos(angle),
                y: 280 + 180 * Math.sin(angle)
            });
        });

        const edges = element("g");
        const vertices = element("g");
        const annotations = element("g");

        for (const link of links) {
            const a = positions.get(link.from_id);
            const b = positions.get(link.to_id);

            if (!a || !b) continue;

            const recommended = selected.has(link.id);
            const broken = failed.has(link.id);
            const existing =
                link.has_valve ||
                String(link.link_type).toUpperCase() === "VALVE";

            let color = "#6884a7";
            let strokeWidth = "2.5";
            let dash = "";

            if (existing) {
                color = "#f5b665";
                strokeWidth = "4";
            }

            if (closed.has(link.id)) {
                color = "#f5b665";
                dash = "6 5";
            }

            if (recommended) {
                color = "#49e0ca";
                strokeWidth = "6";
            }

            if (broken) {
                color = "#fb6577";
                strokeWidth = "6";
                dash = "8 5";
            }

            const line = element("line", {
                x1: a.x,
                y1: a.y,
                x2: b.x,
                y2: b.y,
                stroke: color,
                "stroke-width": strokeWidth,
                "stroke-dasharray": dash,
                "stroke-linecap": "round"
            });

            const title = element("title");
            title.textContent =
                `${link.id}: ${link.from_id} → ${link.to_id}` +
                (recommended ? " | Recommended new valve" : "") +
                (broken ? " | Failed" : "");

            line.appendChild(title);
            edges.appendChild(line);

            if (recommended || broken || existing) {
                const x = (a.x + b.x) / 2;
                const y = (a.y + b.y) / 2;

                const label = element("text", {
                    x,
                    y: y - 12,
                    fill: color,
                    "font-size": "17",
                    "font-weight": "700",
                    "text-anchor": "middle"
                });

                label.textContent = link.id;
                annotations.appendChild(label);
            }
        }

        for (const node of nodes) {
            const pos = positions.get(node.id);
            const source =
                Boolean(node.is_source) ||
                /RESERVOIR|TANK/i.test(node.type || "");

            const circle = element("circle", {
                cx: pos.x,
                cy: pos.y,
                r: source ? 10 : 7,
                fill: source ? "#45d69b" : "#699bff",
                stroke: "#dbeaff",
                "stroke-width": source ? 2.5 : 1
            });

            vertices.appendChild(circle);

            const label = element("text", {
                x: pos.x + 10,
                y: pos.y - 10,
                fill: "#e4edff",
                "font-size": "16"
            });

            label.textContent = node.id;
            annotations.appendChild(label);
        }

        svg.append(edges, vertices, annotations);
    }

    function render(data) {
        drawOptimizedNetwork(data);

        const iso = data.isolation || {};
        const structural = data.structural || {};
        const optimization = data.optimization || {};

        const disconnected = data.reachability || {};

        const metrics = [
            ["Disconnected demand", fmt(disconnected.disconnected_demand)],
            ["Isolation disruption", fmt(iso.total_disrupted_demand)],
            ["Bridges", fmt((structural.bridges || []).length)],
            ["Articulation points",
                fmt((structural.articulation_points || []).length)]
        ];

        $("optMetrics").replaceChildren();

        for (const [label, value] of metrics) {
            const card = document.createElement("div");
            card.className = "comparison-card";

            const name = document.createElement("span");
            name.textContent = label;

            const number = document.createElement("strong");
            number.textContent = value;

            card.append(name, number);
            $("optMetrics").appendChild(card);
        }

        writeText("optIsolation", [
            "Isolation feasible: " +
                (iso.isolation_feasible === undefined
                    ? "Not evaluated"
                    : iso.isolation_feasible ? "Yes" : "No"),
            "Valves to close: " +
                (iso.valves_to_close || []).join(", "),
            "Failed links: " +
                (iso.failed_links || []).join(", "),
            "Extra disruption due to stuck valves: " +
                (iso.extra_disrupted || []).join(", ")
        ]);

        const greedy = optimization.greedy || {};

        writeText("optPlacement", [
            "Greedy recommended locations: " +
                (greedy.placed_valves || []).join(", "),
            "Valves selected: " + fmt(greedy.placed_count),
            "Baseline disruption: " + fmt(greedy.baseline_disruption),
            "Optimized disruption: " + fmt(greedy.total_disruption),
            "Reported improvement: " +
                fmt(greedy.improvement_pct) + "%"
        ]);

        const container = $("optAlgorithms");
        container.replaceChildren();

        const table = document.createElement("table");
        table.className = "optimizer-table";

        const header = document.createElement("tr");

        for (const name of [
            "Algorithm", "Improvement", "Runtime",
            "Candidates", "Valves"
        ]) {
            const th = document.createElement("th");
            th.textContent = name;
            header.appendChild(th);
        }

        table.appendChild(header);

        for (const algorithm of [
            "greedy", "random", "topology", "exhaustive"
        ]) {
            const result = optimization[algorithm];
            if (!result) continue;

            const row = document.createElement("tr");
            const values = [
                algorithm.toUpperCase(),
                fmt(result.improvement_pct) + "%",
                fmt(result.runtime_ms) + " ms",
                fmt(result.candidates_evaluated),
                (result.placed_valves || []).join(", ") || "None"
            ];

            for (const value of values) {
                const td = document.createElement("td");
                td.textContent = value;
                row.appendChild(td);
            }

            table.appendChild(row);
        }

        container.appendChild(table);
    }

    $("runOptimization").addEventListener("click", async () => {
        const button = $("runOptimization");
        const status = $("optStatus");

        button.disabled = true;
        status.textContent = "Running C optimization algorithms...";

        const params = new URLSearchParams({
            dataset: $("optDataset").value,
            failures: $("optFailure").value.replace(/\s+/g, ""),
            stuck: $("optStuck").value.replace(/\s+/g, ""),
            budget: $("optBudget").value
        });

        try {
            const response = await fetch(`/api/optimize?${params}`);
            const data = await response.json();

            if (!response.ok) {
                throw new Error(data.details || data.error);
            }

            render(data);
            status.textContent =
                "Optimization completed using the C engine.";
        } catch (error) {
            status.textContent = "Error: " + error.message;
        } finally {
            button.disabled = false;
        }
    });
})();
