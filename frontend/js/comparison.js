"use strict";

(function () {
    const $ = id => document.getElementById(id);

    let baseline = null;
    let current = null;

    function fmt(value) {
        return Number(value).toLocaleString(undefined, {
            maximumFractionDigits: 2
        });
    }

    function createPanel() {
        if ($ ("comparisonPanel")) return;

        const section = document.createElement("section");
        section.id = "comparisonPanel";
        section.className = "comparison-panel";

        section.innerHTML = `
          <div class="comparison-heading">
            <div>
              <h3>Scenario Impact Comparison</h3>
              <p>Baseline network versus current simulated state</p>
            </div>
            <button id="downloadScenario" class="report-button">
              Export CSV Report
            </button>
          </div>
          <div class="comparison-grid">
            <div class="comparison-card">
              <span>Baseline disconnected demand</span>
              <strong id="baselineDemand">—</strong>
            </div>
            <div class="comparison-card">
              <span>Scenario disconnected demand</span>
              <strong id="scenarioDemand">—</strong>
            </div>
            <div class="comparison-card">
              <span>Additional disconnected demand</span>
              <strong id="demandIncrease">—</strong>
            </div>
            <div class="comparison-card">
              <span>Scenario demand connectivity</span>
              <strong id="scenarioConnectivity">—</strong>
            </div>
          </div>
          <div class="comparison-bottom">
            <div>
              <h4>Disconnected junctions</h4>
              <div id="affectedJunctions" class="affected-list">
                Run a simulation to view the results.
              </div>
            </div>
            <div>
              <h4>Scenario information</h4>
              <div id="scenarioInformation" class="scenario-info">
                No scenario selected.
              </div>
            </div>
          </div>
        `;

        const workspace = document.querySelector(".workspace");
        if (!workspace) return;

        workspace.insertAdjacentElement("afterend", section);

        $("downloadScenario").addEventListener(
            "click",
            downloadReport
        );
    }

    async function fetchBaseline(dataset) {
        const params = new URLSearchParams({
            dataset,
            failures: "-",
            closures: "-"
        });

        const response = await fetch(`/api/network?${params}`);

        if (!response.ok) {
            throw new Error("Baseline request failed");
        }

        return response.json();
    }

    async function update(data) {
        createPanel();

        if (!data || !data.analysis) return;

        current = data;
        const dataset = data.dataset;

        try {
            if (!baseline || baseline.dataset !== dataset) {
                baseline = await fetchBaseline(dataset);
            }

            if (current !== data) return;

            const before = baseline.analysis;
            const after = data.analysis;

            $("baselineDemand").textContent =
                fmt(before.disconnected_demand);

            $("scenarioDemand").textContent =
                fmt(after.disconnected_demand);

            const difference =
                after.disconnected_demand -
                before.disconnected_demand;

            $("demandIncrease").textContent =
                (difference > 0 ? "+" : "") +
                fmt(difference);

            $("scenarioConnectivity").textContent =
                fmt(after.demand_connectivity_percent) + "%";

            const disconnected = data.nodes.filter(
                node => !node.source && !node.reachable
            );

            const container = $("affectedJunctions");
            container.replaceChildren();

            if (!disconnected.length) {
                container.textContent =
                    "No demand junctions disconnected.";
            } else {
                const preview = disconnected.slice(0, 50);

                for (const node of preview) {
                    const row = document.createElement("div");
                    row.className = "affected-item";
                    row.textContent =
                        `${node.id} · Demand: ${fmt(node.demand)}`;
                    container.appendChild(row);
                }

                if (disconnected.length > preview.length) {
                    const extra = document.createElement("p");
                    extra.textContent =
                        `+${disconnected.length - preview.length} more junctions`;
                    container.appendChild(extra);
                }
            }

            const failed = data.links
                .filter(link => link.status === "failed")
                .map(link => link.id);

            const closed = data.links
                .filter(link => link.status === "closed")
                .map(link => link.id);

            $("scenarioInformation").textContent =
                `Dataset: ${dataset}\n` +
                `Failed links: ${failed.join(", ") || "None"}\n` +
                `Closed links: ${closed.join(", ") || "None"}\n` +
                `Disconnected junctions: ${after.disconnected_junctions}\n` +
                `Demand connectivity: ${fmt(after.demand_connectivity_percent)}%`;

        } catch (error) {
            $("scenarioInformation").textContent =
                `Comparison unavailable: ${error.message}`;
        }
    }

    function csvCell(value) {
        const string = String(value ?? "");
        return '"' + string.replace(/"/g, '""') + '"';
    }

    function downloadReport() {
        if (!current || !baseline) return;

        const before = baseline.analysis;
        const after = current.analysis;

        const rows = [
            ["Metric", "Baseline", "Scenario"],
            [
                "Connected junctions",
                before.connected_junctions,
                after.connected_junctions
            ],
            [
                "Disconnected junctions",
                before.disconnected_junctions,
                after.disconnected_junctions
            ],
            [
                "Connected demand",
                before.connected_demand,
                after.connected_demand
            ],
            [
                "Disconnected demand",
                before.disconnected_demand,
                after.disconnected_demand
            ],
            [
                "Demand connectivity (%)",
                before.demand_connectivity_percent,
                after.demand_connectivity_percent
            ]
        ];

        rows.push([]);
        rows.push(["Affected junction", "Demand", "Status"]);

        for (const node of current.nodes) {
            if (!node.source && !node.reachable) {
                rows.push([
                    node.id,
                    node.demand,
                    "Disconnected"
                ]);
            }
        }

        const csv = rows
            .map(row => row.map(csvCell).join(","))
            .join("\r\n");

        const blob = new Blob([csv], {
            type: "text/csv;charset=utf-8"
        });

        const url = URL.createObjectURL(blob);
        const anchor = document.createElement("a");

        anchor.href = url;
        anchor.download =
            `valvewise_${current.dataset}_scenario.csv`;

        document.body.appendChild(anchor);
        anchor.click();
        anchor.remove();

        setTimeout(() => URL.revokeObjectURL(url), 1000);
    }

    window.ValveWiseComparison = { update };
    createPanel();
})();
