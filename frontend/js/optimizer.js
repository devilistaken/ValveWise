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
            <option value="sample">Sample Network</option>
            <option value="image3">Image 3 Network</option>
          </select>
        </label>

        <label>Failed pipe IDs
          <input id="optFailure" value="P5"
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

    function render(data) {
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
