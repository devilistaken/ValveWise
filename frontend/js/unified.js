"use strict";

const $ = id => document.getElementById(id);
const NS = "http://www.w3.org/2000/svg";
let current = null;
let lastDataset = "";
let zoom = 1;
let requestId = 0;

function fmt(value) {
    const n = Number(value);
    return Number.isFinite(n)
        ? n.toLocaleString(undefined,{maximumFractionDigits:2})
        : "N/A";
}

function nodeSvg(tag, attrs={}) {
    const el = document.createElementNS(NS,tag);
    for (const [key,value] of Object.entries(attrs))
        el.setAttribute(key,String(value));
    return el;
}

function textRow(container,label,value) {
    const row = document.createElement("div");
    row.className = "result-row";
    const name = document.createElement("span");
    name.textContent = label;
    const result = document.createElement("strong");
    result.textContent = String(value);
    row.append(name,result);
    container.appendChild(row);
}

function stat(label,value) {
    const card = document.createElement("div");
    card.className = "unified-stat";
    const s = document.createElement("span");
    s.textContent = label;
    const v = document.createElement("strong");
    v.textContent = String(value);
    card.append(s,v);
    return card;
}

function graphPositions(nodes) {
    const valid = nodes.every(n =>
        Number.isFinite(Number(n.x)) &&
        Number.isFinite(Number(n.y))
    );
    const xs = nodes.map(n=>Number(n.x)||0);
    const ys = nodes.map(n=>Number(n.y)||0);
    const xmin=Math.min(...xs), xmax=Math.max(...xs);
    const ymin=Math.min(...ys), ymax=Math.max(...ys);
    const width=xmax-xmin, height=ymax-ymin;
    const useCoordinates=valid && (width>0 || height>0);
    const scale=Math.min(850/Math.max(width,1),460/Math.max(height,1));
    const ox=(1000-width*scale)/2;
    const oy=(600-height*scale)/2;
    const map=new Map();

    nodes.forEach((n,i)=>{
        const a=2*Math.PI*i/Math.max(nodes.length,1);
        map.set(n.id,useCoordinates
            ? {x:ox+(Number(n.x)-xmin)*scale,
               y:600-(oy+(Number(n.y)-ymin)*scale)}
            : {x:500+280*Math.cos(a),
               y:300+200*Math.sin(a)});
    });
    return map;
}

function drawGraph(data) {
    const svg=$("graphSvg");
    svg.replaceChildren();
    svg.setAttribute("viewBox",`0 0 ${1000/zoom} ${600/zoom}`);
    const graph=data.graph||{};
    const nodes=graph.nodes||[];
    const links=graph.links||[];
    if(!nodes.length)return;

    const positions=graphPositions(nodes);
    const proposed=new Set(
        data.optimization?.greedy?.placed_valves||[]
    );
    const failed=new Set(data.isolation?.failed_links||[]);
    const closed=new Set(data.isolation?.valves_to_close||[]);
    const disconnected=new Set(
        (data.reachability?.disconnected_nodes||[])
            .map(n=>typeof n==="string"?n:n.id)
    );

    const edgeLayer=nodeSvg("g");
    const vertexLayer=nodeSvg("g");
    const labels=nodeSvg("g");

    for(const link of links) {
        const a=positions.get(link.from_id);
        const b=positions.get(link.to_id);
        if(!a||!b)continue;

        const isFailed=failed.has(link.id)||
            link.status==="BURST";
        const isProposed=proposed.has(link.id);
        const isExisting=link.has_valve||
            link.link_type==="VALVE";

        let color="#6684a8",width=2.8,dash="";
        if(isExisting){color="#f9b65a";width=4;}
        if(closed.has(link.id)){color="#f9b65a";dash="6 5";}
        if(isProposed){color="#50e5d4";width=6;}
        if(isFailed){color="#fb6475";width=6;dash="8 5";}

        const line=nodeSvg("line",{
            x1:a.x,y1:a.y,x2:b.x,y2:b.y,
            stroke:color,"stroke-width":width,
            "stroke-dasharray":dash,
            class:"graph-pipe"
        });

        const hit=nodeSvg("line",{
            x1:a.x,y1:a.y,x2:b.x,y2:b.y,
            stroke:"transparent","stroke-width":17,
            "pointer-events":"stroke",cursor:"pointer"
        });

        const title=nodeSvg("title");
        title.textContent=`${link.id}: ${link.from_id} → ${link.to_id}`;
        line.appendChild(title);

        hit.addEventListener("click",()=>{
            $("selectionInfo").textContent=
                `${link.id} · ${link.from_id} → ${link.to_id}`+
                ` · ${link.link_type} · ${link.status}`+
                (isProposed?" · RECOMMENDED VALVE":"");
            if(link.link_type==="PIPE") {
                $("failureInput").value=link.id;
                runAnalysis();
            }
        });

        edgeLayer.append(line,hit);

        if(isFailed||isProposed||isExisting){
            const label=nodeSvg("text",{
                x:(a.x+b.x)/2,y:(a.y+b.y)/2-12,
                "text-anchor":"middle",fill:color,
                "font-size":14,"font-weight":700
            });
            label.textContent=link.id;
            labels.appendChild(label);
        }
    }

    for(const n of nodes){
        const p=positions.get(n.id);
        const source=Boolean(n.is_source);
        const lost=disconnected.has(n.id);
        const circle=nodeSvg("circle",{
            cx:p.x,cy:p.y,r:source?10:7,
            fill:source?"#39d8a4":lost?"#fb6475":"#6b9bff",
            stroke:"#e1eafe","stroke-width":1.5
        });
        const label=nodeSvg("text",{
            x:p.x+10,y:p.y-10,class:"graph-label"
        });
        label.textContent=n.id;
        vertexLayer.append(circle);
        labels.appendChild(label);
    }

    svg.append(edgeLayer,vertexLayer,labels);
}

function render(data) {
    current=data;
    const graph=data.graph||{};
    const nodes=graph.nodes||[];
    const links=graph.links||[];
    const reach=data.reachability||{};
    const iso=data.isolation||{};
    const optimization=data.optimization||{};
    const greedy=optimization.greedy||{};
    const structural=data.structural||{};

    const lost=new Set(
        (reach.disconnected_nodes||[])
            .map(n=>typeof n==="string"?n:n.id)
    );
    const junctions=nodes.filter(n=>!n.is_source);
    const demand=junctions.reduce(
        (s,n)=>s+(Number(n.base_demand)||0),0
    );
    const disconnectedDemand=Number(reach.disconnected_demand)||0;

    $("metrics").replaceChildren(
        stat("Junctions",junctions.length),
        stat("Sources",nodes.length-junctions.length),
        stat("Pipelines / Valves",links.length),
        stat("Disconnected demand",fmt(disconnectedDemand)),
        stat("Greedy improvement",fmt(greedy.improvement_pct)+"%")
    );

    const isolation=$("isolationOutput");
    isolation.replaceChildren();
    textRow(isolation,"Isolation feasible",
        iso.isolation_feasible===undefined?"Not assessed":
        iso.isolation_feasible?"Yes":"No");
    textRow(isolation,"Valves to close",
        (iso.valves_to_close||[]).join(", ")||"None");
    textRow(isolation,"Failure(s)",
        (iso.failed_links||[]).join(", ")||"None");
    textRow(isolation,"Disruption after isolation",
        iso.total_disrupted_demand==null
        ?"N/A":fmt(iso.total_disrupted_demand));
    textRow(isolation,"Bridge links",
        (structural.bridges||[]).length);
    textRow(isolation,"Articulation points",
        (structural.articulation_points||[]).length);

    const comparison=$("comparisonOutput");
    comparison.replaceChildren();
    textRow(comparison,"No new valves",
        fmt(greedy.baseline_disruption));
    textRow(comparison,"After proposed placement",
        fmt(greedy.total_disruption));
    textRow(comparison,"Demand saved in model",
        fmt((greedy.baseline_disruption||0)-
            (greedy.total_disruption||0)));
    textRow(comparison,"Modeled improvement",
        fmt(greedy.improvement_pct)+"%");
    textRow(comparison,"New valve locations",
        (greedy.placed_valves||[]).join(", ")||"None");
    textRow(comparison,"Budget used",
        `${greedy.placed_count||0}/${optimization.budget??0}`);

    const benchmark=$("benchmarkOutput");
    benchmark.replaceChildren();
    const table=document.createElement("table");
    table.className="unified-table";
    const head=document.createElement("tr");
    for(const h of ["Algorithm","Disruption","Improvement","Runtime","Candidates","Proposals"]){
        const th=document.createElement("th");
        th.textContent=h;
        head.appendChild(th);
    }
    table.appendChild(head);

    for(const name of ["greedy","random","topology","exhaustive"]){
        const r=optimization[name];
        const tr=document.createElement("tr");
        const values=r
            ? [name,fmt(r.total_disruption),
               fmt(r.improvement_pct)+"%",
               fmt(r.runtime_ms)+" ms",
               fmt(r.candidates_evaluated),
               (r.placed_valves||[]).join(", ")||"None"]
            : [name,"Not evaluated","—","—","—","—"];

        for(const v of values){
            const td=document.createElement("td");
            td.textContent=v;
            tr.appendChild(td);
        }
        table.appendChild(tr);
    }
    benchmark.appendChild(table);

    const critical=$("criticalOutput");
    critical.replaceChildren();
    for(const link of (data.ranked_links||[]).slice(0,8)){
        textRow(critical,link.link_id,
            fmt(link.score)+" disruption score");
    }

    const affected=$("affectedOutput");
    affected.replaceChildren();
    const lostNodes=junctions.filter(n=>lost.has(n.id));
    if(!lostNodes.length)
        affected.textContent="No junctions disconnected by topology.";
    for(const node of lostNodes.slice(0,40))
        textRow(affected,node.id,fmt(node.base_demand));
    if(lostNodes.length>40)
        affected.append(`...and ${lostNodes.length-40} more`);

    drawGraph(data);
    lastDataset=data.dataset||"";
}

async function runAnalysis(){
    const token=++requestId;
    const button=$("scenarioForm").querySelector('[type="submit"]');
    button.disabled=true;
    $("runStatus").textContent="Running C graph and optimization algorithms...";

    const params=new URLSearchParams({
        dataset:$("datasetInput").value,
        failures:$("failureInput").value.replace(/\s+/g,""),
        stuck:$("stuckInput").value.replace(/\s+/g,""),
        budget:$("budgetInput").value
    });

    try{
        const response=await fetch("/api/optimize?"+params);
        const data=await response.json();
        if(!response.ok)
            throw new Error(data.error||data.details||"Analysis failed");
        if(token!==requestId)return;
        render(data);
        $("runStatus").textContent=
            "Completed: C network analysis and valve optimization.";
    }catch(error){
        if(token===requestId)
            $("runStatus").textContent="Error: "+error.message;
    }finally{
        if(token===requestId)button.disabled=false;
    }
}

function downloadFile(name,content,mime){
    const blob=new Blob([content],{type:mime});
    const url=URL.createObjectURL(blob);
    const a=document.createElement("a");
    a.href=url;
    a.download=name;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(()=>URL.revokeObjectURL(url),1000);
}

$("scenarioForm").addEventListener("submit",event=>{
    event.preventDefault();
    runAnalysis();
});

$("resetScenario").addEventListener("click",()=>{
    $("datasetInput").value="demo";
    $("failureInput").value="P2";
    $("stuckInput").value="";
    $("budgetInput").value="1";
    zoom=1;
    runAnalysis();
});

$("datasetInput").addEventListener("change",()=>{
    $("failureInput").value="";
    $("stuckInput").value="";
    runAnalysis();
});

$("fitGraph").addEventListener("click",()=>{
    zoom=1;
    if(current)drawGraph(current);
});

$("exportJson").addEventListener("click",()=>{
    if(!current)return;
    downloadFile("valvewise_analysis.json",
        JSON.stringify(current,null,2),"application/json");
});

$("exportCsv").addEventListener("click",()=>{
    if(!current)return;
    const opt=current.optimization||{};
    const rows=[["Algorithm","Baseline","After","Improvement %","Valves"]];
    for(const name of ["greedy","random","topology","exhaustive"]){
        const r=opt[name];
        if(!r)continue;
        rows.push([name,r.baseline_disruption,r.total_disruption,
            r.improvement_pct,(r.placed_valves||[]).join(" ")]);
    }
    const csv=rows.map(row=>row.map(v=>
        '"'+String(v??"").replace(/"/g,'""')+'"'
    ).join(",")).join("\r\n");
    downloadFile("valvewise_algorithms.csv",csv,"text/csv");
});

runAnalysis();
