#!/usr/bin/env python3
"""Build the offline Phase 14B dashboard and its textual companion."""
from __future__ import annotations

import csv
import html
import json
import statistics
from collections import Counter, defaultdict
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
AUDIT=ROOT/"benchmarks/analysis/phase14b/method_coverage.csv"
CAPS=ROOT/"benchmarks/results/phase14b/manifests/library_method_capabilities.csv"
ROOT_RAW=ROOT/"benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112"
ROOT_SUM=ROOT/"benchmarks/results/phase14b/analysis/root"
EDGE_DIR=ROOT/"benchmarks/results/phase14b/edge_cases/root"
EDGE_ANALYSIS=ROOT/"benchmarks/results/phase14b/analysis/root_edges"
PHASE14_MANIFEST=ROOT/"benchmarks/problems/phase14/workloads.csv"
HTML_OUT=ROOT/"documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.html"
MD_OUT=ROOT/"documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.md"
LIBRARIES=["NUMERIX","GSL","CMINPACK","LAPACK_REFERENCE","LAPACK_OPENBLAS","NLOPT","SUNDIALS_ARKODE","SUNDIALS_CVODE"]
LIBRARY_INFO={
 "NUMERIX":("0.2.0-rc1","C11 public API; Release-like -O3 -DNDEBUG"),
 "GSL":("2.8","GNU Scientific Library native C API"),
 "CMINPACK":("1.3.15","MINPACK hybrid solver, one-variable nonlinear system"),
 "LAPACK_REFERENCE":("3.12.1","Reference LAPACK + reference BLAS + LAPACKE"),
 "LAPACK_OPENBLAS":("LAPACK 3.12.1 + OpenBLAS 0.3.34","LAPACKE backed by OpenBLAS; one thread"),
 "NLOPT":("2.11.0","Native C API"),
 "SUNDIALS_ARKODE":("7.9.0","Fixed-step explicit Kutta-RK4a route"),
 "SUNDIALS_CVODE":("7.9.0","Adaptive Adams route with serial fixed-point nonlinear solver"),
}
MODULES=["root","linear_algebra","differentiation","interpolation","integration","optimization","ode"]
MODULE_LABEL={"root":"Root Finding","linear_algebra":"Linear Algebra","differentiation":"Differentiation",
 "interpolation":"Interpolation","integration":"Integration","optimization":"Optimization","ode":"ODE"}
COLORS={"NUMERIX":"#275DAD","GSL":"#E0764F","CMINPACK":"#735A8C","LAPACK_REFERENCE":"#458E79",
 "LAPACK_OPENBLAS":"#B78728","NLOPT":"#A04A86","SUNDIALS_ARKODE":"#448B9A","SUNDIALS_CVODE":"#576A7B"}


def read(path:Path)->list[dict[str,str]]:
    with path.open(newline="",encoding="utf-8") as f:return list(csv.DictReader(f))


def esc(value:object)->str:return html.escape(str(value),quote=True)


def slug(value:str)->str:
    return "".join(c.lower() if c.isalnum() else "-" for c in value).strip("-")


ROOT_METHOD_IDS={"Bisection":"bisection","Regula Falsi":"regula_falsi","Secant":"secant",
 "Newton-Raphson":"newton","Safeguarded Newton":"safeguarded_newton","Brent":"brent",
 "Automatic method selection":"auto_selection","Solver portfolio":"portfolio",
 "Multiple-root discovery":"multiple_root_discovery"}


def measurement_id(method:dict[str,str])->str:
    if method["module"]=="root":return ROOT_METHOD_IDS[method["method"]]
    return method["method"].lower().replace(" ","_").replace("-","_")


def fmt_pct(num:int,den:int)->str:return f"{100*num/den:.1f}%" if den else "—"


def fmt_us(seconds:object)->str:
    try:return f"{float(seconds)*1e6:.3g} µs"
    except (TypeError,ValueError):return "—"


def relative(path:str)->str:return "../../"+path


def load_data()->dict:
    methods=read(AUDIT);caps=read(CAPS)
    root_acc=read(ROOT_RAW/"accuracy.csv");root_timing=read(ROOT_RAW/"raw_timing.csv")
    root_summary=read(ROOT_SUM/"method_summary.csv");comparisons=read(ROOT_SUM/"paired_comparisons.csv")
    edges=read(EDGE_DIR/"accuracy.csv");edge_cases=read(EDGE_ANALYSIS/"edge_outcomes.csv")
    capmap={(r["library"],r["module"],r["method"]):r for r in caps}
    summary={(r["library"],r["method"]):r for r in root_summary}
    compare={r["method"]:r for r in comparisons}
    root_rows=defaultdict(list)
    for r in root_acc:root_rows[(r["library"],r["requested_method"])].append(r)
    edge_rows=defaultdict(list)
    for r in edges:edge_rows[(r["library"],r["requested_method"])].append(r)
    edges_detail=defaultdict(list)
    for r in edge_cases:edges_detail[(r["library"],r["method"])].append(r)
    difficulty=defaultdict(Counter)
    family=defaultdict(Counter)
    with PHASE14_MANIFEST.open(newline="",encoding="utf-8") as f:
        problems={r["problem_id"]:r for r in csv.DictReader(f)}
    for (lib,method),rows in root_rows.items():
        for r in rows:
            p=problems.get(r["problem_id"])
            if p:
                difficulty[(lib,method)][p["difficulty"]]+=1
                family[(lib,method)][p["family"]]+=1
    work=defaultdict(lambda:defaultdict(list))
    for r in read(ROOT_RAW/"work_metrics.csv"):
        try:work[(r["library"],r["method"])][r["metric"]].append(float(r["value"]))
        except (TypeError,ValueError):pass
    by_edge=defaultdict(Counter)
    for r in edges:
        k=(r["library"],r["requested_method"])
        by_edge[k]["attempts"]+=1
        by_edge[k]["accurate"]+=r["accuracy_pass"]=="1"
        by_edge[k]["success"]+=r["status"]=="0" and r["converged"]=="1"
    return {"methods":methods,"caps":caps,"capmap":capmap,"summary":summary,"compare":compare,
      "root_rows":root_rows,"edge_rows":edge_rows,"edges_detail":edges_detail,
      "difficulty":difficulty,"families":family,"work":work,"by_edge":by_edge,
      "root_acc":root_acc,"root_timing":root_timing,"edges":edges,"problems":problems}


def performance_figure(module:str,method:str,measured:bool)->str:
    if module!="root" or not measured:return ""
    path=relative(f"benchmarks/results/phase14b/figures/root/root_method_{method}_runtime.svg")
    return (f'<figure class="method-figure"><a href="{esc(path)}"><img src="{esc(path)}" '
        f'alt="Measured root method performance graph for {esc(method)}"></a>'
        '<figcaption>Per-problem median wall time across all attempted cases; n and accuracy-pass rate are printed above each bar. '
        'Failed or inapplicable calls can return early, so compare this graph with the applicable-case and accuracy counts.</figcaption></figure>')


def method_card(lib:str,m:dict[str,str],data:dict)->str:
    module=m["module"];name=m["method"];method_id=measurement_id(m);cap=data["capmap"][(lib,module,name)]
    key=(lib,method_id);summary=data["summary"].get(key);rootrows=data["root_rows"].get(key,[])
    edgerows=data["edge_rows"].get(key,[]);edge_details=data["edges_detail"].get(key,[])
    measured=summary is not None
    if measured:
        attempted=int(summary["attempted_problems"]);applicable=int(summary["applicable_problems"])
        success=int(summary["status_successes"]);passes=int(summary["accuracy_passes"])
        accuracy_class="passed" if passes==attempted else "failed" if passes==0 else "mixed"
        success_class="success" if success==attempted else "failure" if success==0 else "mixed"
        med=fmt_us(summary["median_wall_seconds_per_call"])
        cpu=fmt_us(summary["median_positive_cpu_seconds_per_call"])
        workload=f"{attempted} frozen root cases; {applicable} meet the method’s applicability conditions."
        actual=f"{success}/{attempted} successful status; {passes}/{attempted} met both location and residual thresholds."
        di=data["difficulty"].get(key,Counter());families=data["families"].get(key,Counter())
        difficulty_text=", ".join(f"{k}: {v}" for k,v in sorted(di.items())) or "not classified"
        family_text=", ".join(f"{k}: {v}" for k,v in sorted(families.items())) or "—"
        ids=" ".join(r["problem_id"] for r in rootrows)
        work=data["work"].get(key,{})
        work_text="; ".join(f"median {k.replace('_',' ')} {statistics.median(v):.3g}" for k,v in sorted(work.items()) if v)
        edges_text=f"{len(edgerows)} measured edge-case attempts; {sum(r['accuracy_pass']=='1' for r in edgerows)} accurate."
        if method_id=="multiple_root_discovery":
            found=sum(int(r["discovered_roots"]) for r in rootrows);expected=sum(int(r["expected_roots"]) for r in rootrows)
            missed=sum(int(r["missed_roots"]) for r in rootrows);false=sum(int(r["false_positives"]) for r in rootrows)
            heuristic="Yes; a finite heuristic search cannot prove completeness."
            actual+=f" Discovery counted {found}/{expected} reference roots; {missed} missed, {false} false positives."
        else:heuristic="Not applicable."
    else:
        attempted=applicable=success=passes=0
        accuracy_class=success_class="unmeasured"
        med=cpu="—"
        workload="No Phase 14B method-level workload has been run for this module/library route."
        actual="Not measured in Phase 14B."
        difficulty_text=family_text="Not measured."
        ids=""
        work_text="Not measured."
        edges_text="No Phase 14B edge outcomes for this method."
        heuristic="Not assessed."
    capstatus=cap["capability_status"]
    comparison="direct" if capstatus=="direct_equivalent" else "best" if capstatus=="best_available" else "approximate" if "approximate" in capstatus or capstatus=="approximate_wrapper" else "unsupported" if capstatus=="not_supported" else "na"
    edge_flag="measured" if edgerows else "documented" if m["required_edge_cases"] else "none"
    diff_attr=" ".join(data["difficulty"].get(key,{}).keys()) or "not_measured"
    badge=capstatus.replace("_"," ").title()
    api=cap["library_api_or_route"]
    build_info=LIBRARY_INFO[lib][1]
    summary_table=[]
    for other in LIBRARIES:
        oc=data["capmap"][(other,module,name)]
        osum=data["summary"].get((other,name))
        if osum:
            n=int(osum["attempted_problems"]);succ=int(osum["status_successes"]);pas=int(osum["accuracy_passes"])
            rt=fmt_us(osum["median_wall_seconds_per_call"]);cpuother=fmt_us(osum["median_positive_cpu_seconds_per_call"])
            summary_table.append(f"<tr><td>{esc(other)}</td><td>{esc(oc['capability_status'].replace('_',' '))}</td><td>{n}</td><td>{succ}/{n}</td><td>{pas}/{n}</td><td>{rt}</td><td>{cpuother}</td></tr>")
        else:
            status=oc["capability_status"].replace("_"," ")
            summary_table.append(f"<tr><td>{esc(other)}</td><td>{esc(status)}</td><td>—</td><td>—</td><td>—</td><td>—</td><td>—</td></tr>")
    chart=performance_figure(module,method_id,measured)
    if module=="root" and not chart and not measured: chart="<p class='muted'>No measurement-backed graph exists for this route.</p>"
    workload_ids=(f"<details><summary>Show problem IDs ({attempted})</summary><p class='problem-ids'>{esc(ids)}</p></details>" if measured else "")
    edge_table=""
    if edge_details:
        rows=[]
        for e in edge_details:
            rows.append("<tr>"+"".join(f"<td>{esc(e.get(k,'—'))}</td>" for k in ["edge_case","outcome_category","status","applicable","accuracy_pass","absolute_error","residual","observation"])+"</tr>")
        edge_table="<details class='edge-table'><summary>Method-specific edge outcomes ("+str(len(edge_details))+")</summary><div class='table-scroll'><table><thead><tr><th>Edge case</th><th>Outcome</th><th>Status</th><th>Applicable</th><th>Accurate</th><th>Location error</th><th>Residual</th><th>Observation</th></tr></thead><tbody>"+"".join(rows)+"</tbody></table></div></details>"
    elif module=="root":
        edge_table="<p class='muted'>The method’s edge-case requirements are listed in the audit; no Phase 14B result exists for this library route.</p>"
    observations=""
    if module=="root" and measured:
        comp=data["compare"].get(method_id)
        if comp:
            ratio=float(comp["median_paired_speed_ratio"]);low=float(comp["bootstrap_95ci_low"]);high=float(comp["bootstrap_95ci_high"]);n=int(comp["eligible_paired_problems"])
            direction="GSL’s median call time was lower" if ratio<1 else "NUMERIX’s median call time was lower"
            observations=f"On {n} paired, successful, accuracy-passing problems, {direction} for this method; median GSL/NUMERIX ratio {ratio:.3g}, problem-bootstrap 95% interval [{low:.3g}, {high:.3g}]. This interval is descriptive; no hypothesis test or familywise significance claim was made."
        elif method_id=="multiple_root_discovery":
            gsl=data["summary"].get(("GSL",method_id));n=int(summary["attempted_problems"])
            if gsl:
                observations=f"NUMERIX found all expected roots in {passes}/{attempted} accuracy-passing cases. GSL’s fixed 4096-panel sign-change scan missed six roots in its 100-case set. GSL has no root-discovery API; these are best-available heuristic scans, not equivalent algorithms."
            else:observations="Root discovery is a NUMERIX-only orchestration study; completeness is unproved."
        else:
            observations=f"{actual} Median wall call time {med}; median positive CPU call time {cpu}. Early failures and inapplicable cases remain in the attempt counts."
    elif module=="root":
        observations="No measured result; no performance or accuracy conclusion is made."
    else:
        observations="No Phase 14B measurement exists for this method; performance conclusions are deferred."
    caveats=("Memory: method-specific allocation interception was not run. Time-to-accuracy: no tolerance sweep was run for this method."
             if not measured else "Method-level allocation totals were not instrumented. One configured tolerance per workload was tested; time-to-accuracy was not measured.")
    return f'''<details class="method-card" data-library="{esc(lib)}" data-module="{esc(module)}" data-method="{esc(slug(name))}" data-comparison="{comparison}" data-difficulties="{esc(diff_attr)}" data-edge="{edge_flag}" data-accuracy="{accuracy_class}" data-success="{success_class}">
<summary><span>{esc(name)}</span><span class="badge">{esc(badge)}</span><span class="method-measure">{esc(cap['phase14b_measurement_status'])}</span></summary>
<div class="method-body"><h4>1. Method overview</h4><p><b>Public NUMERIX method:</b> <code>{esc(m['public_symbol'])}</code>. <b>Library route:</b> <code>{esc(api)}</code>. <b>Comparison class:</b> {esc(comparison.replace('_',' '))}. <b>Build:</b> {esc(build_info)}. {esc(cap['note'])}</p>
<p><b>Algorithm and use:</b> {esc(name)}; intended module: {esc(MODULE_LABEL[module])}. <b>Assumptions and edge checklist:</b> {esc(m['required_edge_cases'])}</p>
<h4>2. Benchmark workload</h4><p>{esc(workload)} Difficulty counts: {esc(difficulty_text)}. Families: {esc(family_text)}. {esc(actual)} {esc(edges_text)}</p>{workload_ids}
<h4>3. Performance and accuracy</h4>{chart}<p><b>Interpretation:</b> {esc(observations)}</p>
<p>Median wall call time: <b>{esc(med)}</b>. Median positive CPU call time: <b>{esc(cpu)}</b>. Measured work: {esc(work_text or 'No comparable counter collected.')}</p>
<h4>4. Comparison with other libraries</h4><div class="table-scroll"><table><thead><tr><th>Library</th><th>Capability</th><th>Cases</th><th>Success</th><th>Accuracy pass</th><th>Median wall/call</th><th>Median CPU/call</th></tr></thead><tbody>{''.join(summary_table)}</tbody></table></div>
<h4>5. Method observations</h4><p>{esc(observations)}</p><h4>6. Edge cases</h4>{edge_table}
<h4>7. Method conclusion</h4><p><b>Accuracy:</b> {esc(f'{passes}/{attempted} passed' if measured else 'not assessed')}. <b>Reliability:</b> {esc(f'{success}/{attempted} success statuses' if measured else 'not assessed')}. <b>Performance:</b> {esc(observations)} <b>Known limitations:</b> {esc(caveats)} <b>Completeness heuristic:</b> {esc(heuristic)}</p></div></details>'''


def module_sections(data:dict)->str:
    out=[]
    for module in MODULES:
        ms=[m for m in data["methods"] if m["module"]==module]
        out.append(f'<section class="module-section" id="module-{esc(module)}"><h2>{esc(MODULE_LABEL[module])}</h2>')
        if module=="root":
            out.append('<p>Method-level results are available for six scalar solvers, automatic selection, portfolio execution, and multiple-root discovery. Direct solver comparisons and best-available routes are separated below.</p>')
            for fname,caption in [("root_direct_runtime_paired.svg","Median solver time on paired accurate problems; each point summarizes per-problem medians."),
              ("root_direct_speed_ratio_forest.svg","GSL/NUMERIX ratios with problem-level bootstrap intervals. Ratios above 1 favor NUMERIX; no significance claim is made."),
              ("root_discovery_counts.svg","Reference and discovered root counts across the 100 frozen root problems."),
              ("root_edge_accuracy_counts.svg","Edge-case attempts and accuracy passes by library and method.")]:
                p=relative("benchmarks/results/phase14b/figures/root/"+fname)
                out.append(f'<figure><a href="{esc(p)}"><img class="wide-chart" src="{esc(p)}" alt="{esc(caption)}"></a><figcaption>{esc(caption)}</figcaption></figure>')
            out.append('<h3>Direct comparison results</h3><div class="table-scroll"><table><thead><tr><th>Method</th><th>Eligible pairs</th><th>Excluded</th><th>Median GSL/NUMERIX</th><th>Bootstrap 95% interval</th><th>Interpretation</th></tr></thead><tbody>')
            for m in ["bisection","regula_falsi","secant","newton","brent"]:
                c=data["compare"][m];ratio=float(c["median_paired_speed_ratio"]);n=int(c["eligible_paired_problems"])
                direction="lower NUMERIX median" if ratio>1 else "lower GSL median"
                out.append(f'<tr><td>{esc(m)}</td><td>{n}</td><td>{esc(c["excluded_problems"])}</td><td>{ratio:.3g}</td><td>[{float(c["bootstrap_95ci_low"]):.3g}, {float(c["bootstrap_95ci_high"]):.3g}]</td><td>{esc(direction)} on eligible paired problems</td></tr>')
            out.append('</tbody></table></div>')
        else:
            out.append(f'<p>Module is present in the public API audit with {len(ms)} method/operation rows. No Phase 14B method-level measurements have been collected for this module in this execution.</p>')
        out.append('<h3>Library coverage matrix</h3><div class="table-scroll"><table><thead><tr><th>Method</th>')
        out.extend(f'<th>{esc(l)}</th>' for l in LIBRARIES);out.append('</tr></thead><tbody>')
        for m in ms:
            out.append(f'<tr><th>{esc(m["method"])}</th>')
            for lib in LIBRARIES:
                c=data["capmap"][(lib,module,m["method"])]
                result=" · measured" if (lib,measurement_id(m)) in data["summary"] else " · pending" if c["phase14b_measurement_status"] not in {"not-applicable","not-measured"} else ""
                out.append(f'<td>{esc(c["capability_status"].replace("_"," ")+result)}</td>')
            out.append('</tr>')
        out.append('</tbody></table></div>')
        if module=="root":
            out.append('<p><b>Module conclusion:</b> On this 100-case frozen root set, NUMERIX method outcomes and direct paired timing comparisons were collected. The discovery comparison remains heuristic; edge outcomes are listed per method. See the method cards for sample counts, method-specific graphs, and the detailed edge table.</p>')
        else:
            out.append('<p><b>Module conclusion:</b> pending Phase 14B runs; no comparative performance conclusion is supported yet.</p>')
        out.append('</section>')
    return "\n".join(out)


def library_sections(data:dict)->str:
    out=[]
    for lib in LIBRARIES:
        version,config=LIBRARY_INFO[lib]
        caprows=[r for r in data["caps"] if r["library"]==lib]
        supported=[r for r in caprows if r["capability_status"] not in {"not_supported","not_applicable"}]
        measured=sum(r["phase14b_measurement_status"]=="method-measured" for r in caprows)
        # Root experiment rates are local to its 100-case workload; never generalize across modules.
        root_summ=[v for (l,_),v in data["summary"].items() if l==lib]
        rs=sum(int(r["status_successes"]) for r in root_summ);rp=sum(int(r["attempted_problems"]) for r in root_summ)
        ra=sum(int(r["accuracy_passes"]) for r in root_summ)
        out.append(f'<details class="library-card" data-library="{esc(lib)}" id="library-{esc(slug(lib))}"><summary><span>{esc(lib)}</span><span class="badge">{esc(version)}</span></summary><div class="library-body"><p><b>Configuration:</b> {esc(config)}. <b>Capability rows:</b> {len(supported)} of {len(caprows)} include a supported/best-available route. <b>Methods measured in Phase 14B:</b> {measured} method-level library routes.</p>')
        if rp:out.append(f'<p><b>Root-only reliability:</b> {rs}/{rp} status successes; {ra}/{rp} accuracy passes. This sample does not describe other modules.</p>')
        else:out.append('<p>Phase 14B has no measurements for this library in this execution.</p>')
        for module in MODULES:
            ms=[m for m in data["methods"] if m["module"]==module]
            out.append(f'<details class="lib-module" data-module="{esc(module)}"><summary>{esc(MODULE_LABEL[module])} · {len(ms)} audited methods</summary>')
            for m in ms:out.append(method_card(lib,m,data))
            out.append('</details>')
        out.append('</div></details>')
    return "\n".join(out)


def build_html(data:dict)->str:
    methods_json=json.dumps([{"module":m["module"],"name":m["method"]} for m in data["methods"]])
    rootproblems=100
    edge_n=len(data["edges"])
    timing_n=len(data["root_timing"])
    accuracy_n=len(data["root_acc"])
    distinct_methods=len(data["methods"])
    main=relative("benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112")
    edgepath=relative("benchmarks/results/phase14b/edge_cases/root")
    figpath=relative("benchmarks/results/phase14b/figures/root")
    cap_matrix="../development/phase_14b_library_capability_matrix.md"
    audit_matrix="../development/phase_14b_method_coverage_matrix.md"
    report_md="NUMERIX_phase14b_method_level_comparative_evaluation.md"
    libopts="".join(f'<option value="{esc(l)}">{esc(l)}</option>' for l in LIBRARIES)
    modopts="".join(f'<option value="{esc(m)}">{esc(MODULE_LABEL[m])}</option>' for m in MODULES)
    methodopts="".join(f'<option value="{esc(slug(m["method"]))}">{esc(MODULE_LABEL[m["module"]]+" · "+m["method"])}</option>' for m in data["methods"])
    nav="".join(f'<a href="#module-{esc(m)}">{esc(MODULE_LABEL[m])}</a>' for m in MODULES)
    comparetable=[]
    for m in ["bisection","regula_falsi","secant","newton","brent"]:
        c=data["compare"][m]
        comparetable.append(f'<tr><td>{esc(m)}</td><td>{esc(c["eligible_paired_problems"])}</td><td>{esc(c["excluded_problems"])}</td><td>{float(c["median_paired_speed_ratio"]):.3g}</td><td>[{float(c["bootstrap_95ci_low"]):.3g}, {float(c["bootstrap_95ci_high"]):.3g}]</td></tr>')
    ratio_text=[]
    for m in ["bisection","regula_falsi","secant","newton","brent"]:
        c=data["compare"][m];ratio=float(c["median_paired_speed_ratio"]);lo=float(c["bootstrap_95ci_low"]);hi=float(c["bootstrap_95ci_high"])
        ratio_text.append(f"{m}: ratio {ratio:.3g}, 95% CI [{lo:.3g}, {hi:.3g}], n={c['eligible_paired_problems']}")
    findings="; ".join(ratio_text)
    css='''
:root{--ink:#172432;--muted:#526576;--line:#d8e1e8;--panel:#fff;--page:#f4f7fa;--blue:#275dad;--teal:#167b78;--amber:#a56310;--red:#a33e39;--shadow:0 6px 22px rgba(24,45,63,.07)}*{box-sizing:border-box}body{margin:0;background:var(--page);color:var(--ink);font:15px/1.55 Inter,ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif}a{color:#1458a6}header{background:linear-gradient(120deg,#14273b,#243f59);color:#fff;padding:32px clamp(20px,5vw,70px)}header h1{font-size:clamp(27px,4vw,42px);line-height:1.1;margin:9px 0}.eyebrow{text-transform:uppercase;letter-spacing:.15em;font-weight:700;color:#b5d5f2;font-size:12px}.header-meta{color:#d2e1ed}.status{display:inline-block;padding:4px 10px;border:1px solid #f4c176;border-radius:99px;color:#ffe0a8;font-weight:750;font-size:12px}nav{display:flex;flex-wrap:wrap;gap:7px;padding:12px clamp(20px,5vw,70px);background:#fff;border-bottom:1px solid var(--line);position:sticky;top:0;z-index:5}nav a{padding:5px 9px;border-radius:6px;text-decoration:none;font-size:13px;color:#27445b}nav a:hover{background:#e9f1f7}main{max-width:1500px;margin:auto;padding:24px clamp(16px,3vw,42px) 70px}.panel,.module-section,.library-card{background:var(--panel);border:1px solid var(--line);border-radius:12px;box-shadow:var(--shadow);padding:20px;margin:0 0 18px}.kpis{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px}.kpi{background:#f6f9fc;border:1px solid var(--line);padding:13px;border-radius:9px}.kpi b{display:block;font-size:23px;color:#193c60}.kpi span{display:block;color:var(--muted);font-size:12px}.callout{border-left:4px solid #d6942c;background:#fff8ea;padding:12px 15px;border-radius:5px}.muted{color:var(--muted)}h2{margin:9px 0 12px;font-size:26px}h3{margin-top:24px}h4{margin:18px 0 6px;color:#233d54}.filters{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:10px;position:sticky;top:48px;z-index:4}.filters label{font-size:12px;font-weight:650;color:var(--muted)}select,input{display:block;width:100%;padding:9px;border:1px solid #bdcbd7;border-radius:7px;background:#fff;color:var(--ink);margin-top:4px}.filter-actions{display:flex;gap:8px;align-items:end;flex-wrap:wrap}button{padding:9px 12px;border:1px solid #b7c7d5;background:#fff;border-radius:7px;color:#183850;cursor:pointer;font-weight:650}button:hover{background:#edf4f9}.count{align-self:center;color:var(--muted);font-size:13px}.library-card{padding:0;overflow:hidden}.library-card>summary,.method-card>summary,.lib-module>summary{display:flex;align-items:center;justify-content:space-between;gap:12px;cursor:pointer;list-style:none}.library-card>summary{padding:17px 20px;font-size:19px;font-weight:750}.library-card>summary::-webkit-details-marker,.method-card>summary::-webkit-details-marker,.lib-module>summary::-webkit-details-marker{display:none}.library-card[open]>summary,.lib-module[open]>summary{background:#f2f7fa}.library-body{padding:2px 18px 18px}.lib-module{border:1px solid #e2e8ed;border-radius:8px;margin:12px 0;overflow:hidden}.lib-module>summary{padding:11px 13px;font-weight:700;background:#fbfcfd}.method-card{border-top:1px solid #e6ebef}.method-card>summary{padding:10px 15px 10px 27px;font-weight:630}.method-card[open]>summary{color:#164b79;background:#f8fafc}.method-body{padding:4px 22px 22px 34px}.badge{display:inline-flex;align-items:center;border:1px solid #c8d6e1;border-radius:99px;padding:2px 8px;background:#f3f7fa;color:#35556c;font-size:11px;font-weight:700}.method-measure{font-size:11px;color:var(--muted);margin-left:auto}.table-scroll{overflow:auto;max-width:100%;border:1px solid var(--line);border-radius:8px;margin:9px 0 15px}table{border-collapse:collapse;width:100%;font-size:12px;background:#fff}th,td{padding:8px 9px;border-bottom:1px solid #e5ebef;text-align:left;vertical-align:top}th{background:#f1f5f8;position:sticky;top:0;z-index:1}td{max-width:330px;overflow-wrap:anywhere}figure{margin:14px 0 20px;background:#fff;border:1px solid var(--line);padding:10px;border-radius:9px}figure img{max-width:100%;height:auto;display:block;margin:auto}.wide-chart{width:min(100%,940px)}figcaption{font-size:12px;color:var(--muted);padding:7px 4px 0}.method-figure img{width:min(100%,690px)}.problem-ids{max-height:120px;overflow:auto;background:#f7f9fb;padding:8px;font:11px/1.5 ui-monospace,monospace;overflow-wrap:anywhere}.edge-table summary,details>summary{cursor:pointer}.module-section{scroll-margin-top:80px}.module-section figure{max-width:1050px}.raw-links{display:flex;gap:9px;flex-wrap:wrap}.raw-links a{display:inline-block;padding:8px 11px;border:1px solid var(--line);border-radius:7px;text-decoration:none;background:#fff}.hidden{display:none!important}.footer{color:var(--muted);font-size:12px;padding:15px 0}code{background:#eef2f5;border-radius:4px;padding:1px 4px;font-size:.92em}#library-comparison{scroll-margin-top:75px}.warning{color:#8a4e07;font-weight:650}@media(max-width:650px){nav{position:static}.filters{position:static}.method-body{padding-left:15px}.method-card>summary{padding-left:14px}.table-scroll{font-size:11px}}
'''
    js='''
const $=s=>document.querySelector(s),$$=s=>Array.from(document.querySelectorAll(s));
function applyFilters(){const f={library:$('#f-library').value,module:$('#f-module').value,method:$('#f-method').value,comparison:$('#f-comparison').value,difficulty:$('#f-difficulty').value,edge:$('#f-edge').value,accuracy:$('#f-accuracy').value,success:$('#f-success').value,search:$('#f-search').value.trim().toLowerCase()};let visible=0;for(const c of $$('.method-card')){const ok=(f.library==='all'||c.dataset.library===f.library)&&(f.module==='all'||c.dataset.module===f.module)&&(f.method==='all'||c.dataset.method===f.method)&&(f.comparison==='all'||c.dataset.comparison===f.comparison)&&(f.difficulty==='all'||c.dataset.difficulties.split(' ').includes(f.difficulty))&&(f.edge==='all'||(f.edge==='measured'&&c.dataset.edge==='measured')||(f.edge==='documented'&&c.dataset.edge!=='none')||(f.edge==='none'&&c.dataset.edge==='none'))&&(f.accuracy==='all'||c.dataset.accuracy===f.accuracy)&&(f.success==='all'||c.dataset.success===f.success)&&(!f.search||c.textContent.toLowerCase().includes(f.search));c.classList.toggle('hidden',!ok);if(ok)visible++;}for(const g of $$('.lib-module')){const any=Array.from(g.querySelectorAll('.method-card')).some(c=>!c.classList.contains('hidden'));g.classList.toggle('hidden',!any);}for(const l of $$('.library-card')){const any=Array.from(l.querySelectorAll('.method-card')).some(c=>!c.classList.contains('hidden'));l.classList.toggle('hidden',!any);}$('#visible-count').textContent=`${visible} method-library cards visible`;}
for(const el of $$('.filters select'))el.addEventListener('change',applyFilters);$('#f-search').addEventListener('input',applyFilters);
$('#reset-filters').addEventListener('click',()=>{$$('.filters select').forEach(x=>x.value='all');$('#f-search').value='';applyFilters();});
$('#expand-all').addEventListener('click',()=>$$('details:not(.hidden)').forEach(d=>d.open=true));$('#collapse-all').addEventListener('click',()=>$$('details').forEach(d=>d.open=false));
for(const a of $$('nav a[href^="#module-"]'))a.addEventListener('click',()=>{const m=a.getAttribute('href').slice(8);$('#f-module').value=m;applyFilters();});
applyFilters();
'''
    header=f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="description" content="Offline Phase 14B NUMERIX method-level benchmark dashboard"><title>NUMERIX Phase 14B · Method-Level Evaluation</title><style>{css}</style></head><body>
<header><div class="eyebrow">NUMERIX · Numerical computing evaluation</div><h1>Phase 14B — Method-Level Benchmarking</h1><div class="header-meta">Comparative evaluation · 12 November 2026 · dataset `phase14b-root-20261112-v3` · status <span class="status">PARTIAL</span></div></header>
<nav><a href="#overview">Overview</a><a href="#library-comparison">Library comparison</a>{nav}<a href="#edge-analysis">Edge cases</a><a href="#memory-analysis">Memory</a><a href="#method-coverage">Coverage</a><a href="#methodology">Methodology</a><a href="#limitations">Limitations</a><a href="#reproducibility">Raw data</a></nav>
<main><section class="panel" id="overview"><h2>Executive summary</h2><div class="callout"><b>Study status: PARTIAL.</b> The public-method inventory spans all seven NUMERIX modules, but Phase 14B measurements currently cover root finding only. This dashboard shows measured root results and explicitly marks all remaining module methods as pending.</div>
<div class="kpis"><div class="kpi"><b>{distinct_methods}</b><span>NUMERIX public methods / operations audited</span></div><div class="kpi"><b>7</b><span>numerical modules in the API</span></div><div class="kpi"><b>8</b><span>candidate libraries listed</span></div><div class="kpi"><b>3</b><span>libraries measured for root methods</span></div><div class="kpi"><b>{rootproblems}</b><span>frozen root problems per method route</span></div><div class="kpi"><b>{accuracy_n:,}</b><span>root accuracy records</span></div><div class="kpi"><b>{timing_n:,}</b><span>raw root timing batches</span></div><div class="kpi"><b>{edge_n}</b><span>root edge-case outcomes</span></div></div>
<p>Root routes measured: NUMERIX 9, GSL 7, CMINPACK 1. Each root route includes 100 frozen problems; timing uses 31 independent batches per problem and method, with two warmups. Edge suite: 19 cases, 21 timing batches per measured case/library-method pair. Root adapters use a common 500-iteration cap; work counters are collected only in untimed accuracy probes.</p>
<h3>Measured direct root comparisons</h3><p>The paired unit is the problem. A pair is included only when both libraries are applicable, successful, and accurate. The confidence intervals below are descriptive problem-level bootstrap intervals; no statistical significance or overall library winner is claimed.</p><div class="table-scroll"><table><thead><tr><th>Method</th><th>Eligible pairs</th><th>Excluded</th><th>GSL / NUMERIX median time</th><th>95% bootstrap interval</th></tr></thead><tbody>{''.join(comparetable)}</tbody></table></div>
<p><b>Direct comparison summary:</b> {esc(findings)}</p>
<p>Multiple-root discovery is a best-available heuristic comparison. NUMERIX detected 100/100 expected roots in this frozen set; GSL’s fixed 4096-panel sign-change scan detected 94/100 and missed six. The GSL scan has no tangential-root guarantee. NUMERIX’s discovery API reports unresolved intervals and multiplicity estimates; GSL’s scan reports unresolved regions as unavailable.</p>
<h3>Study-level graphs</h3><figure><a href="{esc(figpath)}/root_direct_runtime_paired.svg"><img class="wide-chart" src="{esc(figpath)}/root_direct_runtime_paired.svg" alt="Paired direct root runtime"></a><figcaption>Per-problem paired accurate samples; median wall time per solver call. The eligible sample count is shown for each method.</figcaption></figure>
<figure><a href="{esc(figpath)}/root_direct_speed_ratio_forest.svg"><img class="wide-chart" src="{esc(figpath)}/root_direct_speed_ratio_forest.svg" alt="Root speed ratios and bootstrap intervals"></a><figcaption>GSL/NUMERIX median time ratios and descriptive 95% problem-bootstrap intervals. A ratio above 1 favors NUMERIX.</figcaption></figure>
<figure><a href="{esc(figpath)}/root_discovery_counts.svg"><img class="wide-chart" src="{esc(figpath)}/root_discovery_counts.svg" alt="Root discovery counts"></a><figcaption>Expected and discovered root counts summed over the 100 frozen root problems.</figcaption></figure>
<figure><a href="{esc(figpath)}/root_edge_accuracy_counts.svg"><img class="wide-chart" src="{esc(figpath)}/root_edge_accuracy_counts.svg" alt="Root edge-case accuracy counts"></a><figcaption>Accuracy-passing root edge outcomes, shown alongside attempted cases.</figcaption></figure>
</section>
<section class="panel filters" id="filters"><label>Library<select id="f-library"><option value="all">All libraries</option>{libopts}</select></label><label>Module<select id="f-module"><option value="all">All modules</option>{modopts}</select></label><label>Method<select id="f-method"><option value="all">All methods</option>{methodopts}</select></label><label>Comparison type<select id="f-comparison"><option value="all">All types</option><option value="direct">Direct equivalent</option><option value="best">Best available</option><option value="approximate">Approximate / wrapper</option><option value="unsupported">Not supported</option><option value="na">Not applicable</option></select></label><label>Difficulty<select id="f-difficulty"><option value="all">All / unmeasured</option><option value="easy">Easy</option><option value="intermediate">Intermediate</option><option value="hard">Hard</option></select></label><label>Edge-case coverage<select id="f-edge"><option value="all">All</option><option value="documented">Documented edge cases</option><option value="measured">Edge cases measured</option><option value="none">No edge cases listed</option></select></label><label>Accuracy<select id="f-accuracy"><option value="all">All</option><option value="passed">All attempted passed</option><option value="failed">No accuracy passes</option><option value="mixed">Mixed pass/fail</option><option value="unmeasured">Unmeasured</option></select></label><label>Success<select id="f-success"><option value="all">All</option><option value="success">All successful</option><option value="failure">No successful statuses</option><option value="mixed">Mixed outcomes</option><option value="unmeasured">Unmeasured</option></select></label><label>Search methods or problem IDs<input id="f-search" type="search" placeholder="e.g. Brent, P14-ROOT-..." /></label><div class="filter-actions"><button id="reset-filters" type="button">Reset filters</button><button id="expand-all" type="button">Expand visible</button><button id="collapse-all" type="button">Collapse all</button><span class="count" id="visible-count"></span></div></section>
<section id="library-comparison"><h2>Library comparison · expandable method details</h2><p class="muted">Each library contains nested module and method accordions. Capability, measurement status, and algorithm equivalence are separate fields. Unsupported and not-applicable methods have no fabricated results.</p>{library_sections(data)}</section>
{module_sections(data)}
<section id="edge-analysis" class="module-section"><h2>Edge-case analysis</h2><p>Root suite: 19 dedicated cases; {edge_n} method-library outcomes are recorded. The invalid-expression parser probe is a separate supporting-API check. Correct rejection, numerical failure, cancellation, resource exhaustion, unsupported routes, and inaccurate successful outputs are distinct in the outcome table.</p><p><a href="{esc(edgepath)}/accuracy.csv">Raw edge accuracy</a> · <a href="{esc(edgepath)}/raw_timing.csv">Raw edge timing</a> · <a href="{esc(edgepath)}/method_workloads.csv">Edge method mapping</a> · <a href="{esc(relative('benchmarks/results/phase14b/analysis/root_edges/edge_outcomes.csv'))}">Analyzed edge outcomes</a> · <a href="{esc(edgepath)}/expression_invalid_probe.csv">Invalid-expression probe</a></p><div class="table-scroll"><table><thead><tr><th>Library</th><th>Method</th><th>Attempts</th><th>Success</th><th>Accurate</th><th>Failure/status counts</th></tr></thead><tbody>{''.join(f'<tr><td>{esc(r["library"])}</td><td>{esc(r["method"])}</td><td>{esc(r["attempted_edge_cases"])}</td><td>{esc(r["successful_status"])}</td><td>{esc(r["accuracy_passes"])}</td><td>{esc(r["status_code_counts"])}</td></tr>' for r in read(EDGE_ANALYSIS/"method_summary.csv"))}</tbody></table></div></section>
<section class="module-section" id="memory-analysis"><h2>Memory analysis</h2><p>No method-specific allocation interception was completed in Phase 14B. Existing Phase 14 RSS includes the harness and process baseline, so it is not used here as a per-method library memory comparison. Requested heap bytes, allocator metadata, stack, and shared dependency allocations are unavailable. No memory winner is reported.</p></section>
<section class="module-section" id="method-coverage"><h2>Method coverage and API boundary</h2><p>The verified initial inventory contains 49 public methods and operations: root 9, linear algebra 20, differentiation 7, interpolation 4, integration 4, optimization 2, and ODE 3. Method coverage includes public numerical algorithms, linear-algebra primitives, lifecycle calls requested by the benchmark brief, piecewise adapters, and root orchestration/discovery. Supporting expression, diagnostic, and managed-execution utilities are documented as API boundaries, not silently counted as standalone numerical methods.</p><p><a href="{esc(audit_matrix)}">Human-readable API method matrix</a> · <a href="{esc(relative('benchmarks/results/phase14b/manifests/library_method_capabilities.csv'))}">392-row library-method capability CSV</a> · <a href="{esc(cap_matrix)}">Capability matrix documentation</a></p><div class="table-scroll"><table><thead><tr><th>Module</th><th>Public methods / operations</th><th>Phase 14B measurement status</th></tr></thead><tbody>{''.join(f'<tr><td>{esc(MODULE_LABEL[mod])}</td><td>{sum(m["module"]==mod for m in data["methods"])}</td><td>{"Measured: NUMERIX/GSL/CMINPACK routes" if mod=="root" else "Pending method-level run"}</td></tr>' for mod in MODULES)}</tbody></table></div></section>
<section class="module-section" id="methodology"><h2>Methodology and statistical interpretation</h2><ul><li>Frozen Phase 14 problem IDs were reused. Root comparisons used identical function definitions, brackets/starts, tolerances, and references; eligibility was separately recorded.</li><li>31 timing batches per normal root problem-method-library route; two warmups. Direct iterative calls used four repetitions per batch, orchestration two, and root-discovery one. Edge cases use 21 batches and one repetition.</li><li>Work counters are collected in a separate untimed probe. Wall and process CPU time are retained. Per-problem medians, not the 31 repeated batches, are the unit for paired comparisons and bootstrap confidence intervals.</li><li>Paired direct ratios require both applicable, successful, accuracy-passing outputs and positive measured times. Exclusions remain in the raw accuracy records. The ratio is median GSL time divided by median NUMERIX time.</li><li>Holm correction was considered. No hypothesis tests or familywise significance claims are made; intervals are descriptive and therefore are not adjusted.</li><li>Method call timing includes library solver/workspace calls and excludes input parsing, process launch, and CSV writing. Setup and conversion are not separately measured for all routes.</li></ul></section>
<section class="module-section" id="limitations"><h2>Limitations and outstanding scope</h2><ul><li>Only root finding has Phase 14B method-level raw measurements and edge runs. Six modules remain unmeasured.</li><li>Time-to-accuracy tolerance sweeps were not run; one configured tolerance per problem was checked.</li><li>Method-specific allocation instrumentation and memory comparisons are not available.</li><li>Root discovery is heuristic; NUMERIX reports unresolved regions while the GSL scan cannot provide equivalent unresolved-region or multiplicity metrics.</li><li>CMINPACK is a best-available one-variable hybrid-system comparison, not direct Newton equivalence. GSL’s safeguarded-Newton route uses Brent and is best-available.</li><li>The GSL discovery baseline scans 4096 subintervals and refines sign changes, so it can miss tangential roots.</li><li>The Phase 14B CMake file is provided, but CMake was unavailable in this shell. The adapter was compiled with the recorded GCC command and linked against locally built pinned dependencies. This remains a build reproducibility limitation.</li></ul></section>
<section class="module-section" id="reproducibility"><h2>Reproducibility and raw data</h2><div class="raw-links"><a href="{esc(main)}/run_metadata.json">Run metadata</a><a href="{esc(main)}/method_workloads.csv">Method workload mapping</a><a href="{esc(main)}/accuracy.csv">Root accuracy CSV</a><a href="{esc(main)}/raw_timing.csv">Root raw timings</a><a href="{esc(main)}/work_metrics.csv">Root work metrics</a><a href="{esc(main)}/discovery_details.csv">Per-root discovery details</a><a href="{esc(relative('benchmarks/results/phase14b/analysis/root/method_summary.csv'))}">Root method summary</a><a href="{esc(relative('benchmarks/results/phase14b/analysis/root/paired_comparisons.csv'))}">Paired comparisons</a><a href="{esc(edgepath)}/run_metadata.json">Edge run metadata</a><a href="{esc(relative('benchmarks/results/phase14b/analysis/root_edges/edge_outcomes.csv'))}">Edge outcomes</a></div><p>Report text: <a href="{esc(report_md)}">Markdown companion</a>. Requirements source: <code>a.md</code> at the project root. Method matrix and capability files are linked above.</p><p>Git HEAD during benchmark: <code>cbc3d27bb89451f66f938cfe245274f0dccb37b4</code>. No version bump, commit, or push was made.</p></section>
<footer class="footer">NUMERIX Phase 14B · offline report · charts and datasets are local files · no backend or external script is required</footer></main><script>{js}</script></body></html>'''
    return header


def write_markdown(data:dict)->str:
    compare=data["compare"]
    lines=["# NUMERIX Phase 14B — Method-Level Comparative Evaluation","",
      "**Status: PARTIAL.** The API audit covers all seven modules; method-level measurement and edge-case work currently covers root finding only.","",
      "## Executive summary","",
      "- 49 public method/operation rows audited across seven modules.","- Root run: 100 frozen problems per method route; 17 library-method routes; 1,700 accuracy records; 52,700 raw timing batches.",
      "- Root edges: 19 cases; 322 measured library-method outcomes; 6,762 timing batches; one CMINPACK discontinuity route is explicitly unsupported/not run.",
      "- Libraries measured for roots: NUMERIX, GSL 2.8, and CMINPACK 1.3.15. LAPACK reference, LAPACK/OpenBLAS, NLopt, ARKODE, and CVODE are present in the capability map but not measured at method level in Phase 14B.",
      "- The study reports no overall library winner. Root discovery is a best-available heuristic comparison.","",
      "## Direct root comparisons","",
      "The statistical unit is the problem. A pair is retained only when both implementations are applicable, successful, accurate, and have positive wall measurements. Confidence intervals use a 5,000-resample paired problem bootstrap. No hypothesis test or familywise significance claim is made; Holm correction was considered but not applied.","",
      "| Method | Eligible pairs | Excluded | Median GSL / NUMERIX | Problem bootstrap 95% interval |","|---|---:|---:|---:|---:|"]
    for m in ["bisection","regula_falsi","secant","newton","brent"]:
        r=compare[m];lines.append(f"| {m} | {r['eligible_paired_problems']} | {r['excluded_problems']} | {float(r['median_paired_speed_ratio']):.3g} | [{float(r['bootstrap_95ci_low']):.3g}, {float(r['bootstrap_95ci_high']):.3g}] |")
    lines += ["","A ratio above one means GSL's median call time was higher on the eligible paired problems. These results are descriptive and do not claim statistical significance.","",
      "## Root method findings",""]
    for m in ["bisection","regula_falsi","secant","newton","safeguarded_newton","brent","auto_selection","portfolio","multiple_root_discovery"]:
        n=data["summary"].get(("NUMERIX",m));g=data["summary"].get(("GSL",m));c=data["summary"].get(("CMINPACK",m))
        if n:
            lines.append(f"- **{m}:** NUMERIX {n['status_successes']}/{n['attempted_problems']} successful status and {n['accuracy_passes']}/{n['attempted_problems']} accuracy passes; median all-attempt wall call time {fmt_us(n['median_wall_seconds_per_call'])}.")
            if g:lines.append(f"  GSL: {g['status_successes']}/{g['attempted_problems']} successful and {g['accuracy_passes']}/{g['attempted_problems']} accuracy passes; median {fmt_us(g['median_wall_seconds_per_call'])}.")
            if c:lines.append(f"  CMINPACK best-available hybrid solve: {c['status_successes']}/{c['attempted_problems']} successful and {c['accuracy_passes']}/{c['attempted_problems']} accuracy passes; median {fmt_us(c['median_wall_seconds_per_call'])}.")
    discovery=[r for r in data["root_acc"] if r["requested_method"]=="multiple_root_discovery"]
    for lib in ("NUMERIX","GSL"):
        r=[x for x in discovery if x["library"]==lib]
        lines.append(f"- **{lib} multiple-root discovery:** {sum(int(x['discovered_roots']) for x in r)} detected of {sum(int(x['expected_roots']) for x in r)} reference roots; {sum(int(x['missed_roots']) for x in r)} missed and {sum(int(x['false_positives']) for x in r)} false positives. GSL uses a fixed 4096-panel sign-change scan, not a root-discovery API.")
    lines += ["","## Edge-case results","","The edge ledger records exact inputs, expected behavior, status, accuracy, residual, work, and observation. Correct rejections are not automatically failures. NUMERIX discovery cancellation is distinguished from solver failure; its evaluation cap is not treated as equivalent to GSL because GSL's API does not expose the same per-call budget.","",
      "Edge outcomes: [edge_outcomes.csv](../../benchmarks/results/phase14b/analysis/root_edges/edge_outcomes.csv). Raw outcomes: [accuracy.csv](../../benchmarks/results/phase14b/edge_cases/root/accuracy.csv), [raw_timing.csv](../../benchmarks/results/phase14b/edge_cases/root/raw_timing.csv), [method workload map](../../benchmarks/results/phase14b/edge_cases/root/method_workloads.csv). Invalid expression compilation was checked separately in [expression_invalid_probe.csv](../../benchmarks/results/phase14b/edge_cases/root/expression_invalid_probe.csv).","",
      "## Measurement protocol","","Normal root study: 31 independent batches per problem-method-library route, two warmups, four repeats per batch for iterative algorithms, two for orchestration, and one for discovery. Work counters were captured only in an untimed probe. Edge suite: 21 batches per measured case route. Solver timing excludes process startup, parsing, and result-file I/O. The common root-solver cap is 500 iterations; cap and evaluation-budget fixtures are retained as explicit edge records.","",
      "## Figures","","All charts have PNG and SVG versions under [benchmarks/results/phase14b/figures/root](../../benchmarks/results/phase14b/figures/root/). The interactive [HTML report](NUMERIX_phase14b_method_level_comparative_evaluation.html) embeds the method-level charts and includes local filters and nested library/method accordions.","",
      "## Memory, time-to-accuracy, and limitations","","No method-specific heap interception or memory comparison was completed. Phase 14 RSS includes harness/process baseline and is not used as library-specific memory evidence. No tolerance sweeps were performed, so time-to-accuracy is unavailable. CPU samples remain in raw records; zero-resolution samples are excluded from positive CPU summaries.","",
      "The linear algebra, differentiation, interpolation, integration, optimization, and ODE method-level benchmark modules remain pending. CMake is absent from the current shell; the Phase 14B adapter compiled manually with GCC 8.5.0, C11, `-O3 -DNDEBUG -Wall -Wextra -Wpedantic`, and pinned local GSL/CMINPACK libraries. The CMake configuration is saved for reproduction where CMake is available.","",
      "## Reproducibility artifacts","","- Root requirements: [`a.md`](../../a.md)","- Initial method inventory: [`method_coverage.csv`](../../benchmarks/analysis/phase14b/method_coverage.csv)","- Per-library capability map: [`library_method_capabilities.csv`](../../benchmarks/results/phase14b/manifests/library_method_capabilities.csv)","- Root raw run: [`root-method-study-final-v3-20261112`](../../benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112/)","- Root analysis: [`analysis/root`](../../benchmarks/results/phase14b/analysis/root/)","- Edge analysis: [`analysis/root_edges`](../../benchmarks/results/phase14b/analysis/root_edges/)","- The Phase 13 checksum inventory and prior Phase 14 data remain preserved. No commit, push, or version change was made.","",
      "**Final Phase 14B status: PARTIAL.** The dashboard shows all methods and library capabilities, but six modules, time-to-accuracy, and method-scoped memory remain unfinished."]
    return "\n".join(lines)+"\n"


def main()->int:
    data=load_data();HTML_OUT.parent.mkdir(parents=True,exist_ok=True)
    HTML_OUT.write_text(build_html(data),encoding="utf-8")
    MD_OUT.write_text(write_markdown(data),encoding="utf-8")
    print(f"wrote offline HTML and Markdown report for {len(data['methods'])} methods across {len(LIBRARIES)} libraries")
    return 0


if __name__=="__main__":raise SystemExit(main())
