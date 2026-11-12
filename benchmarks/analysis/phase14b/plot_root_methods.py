#!/usr/bin/env python3
"""Create publication-style root method plots from Phase 14B raw data."""
from __future__ import annotations

import csv
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT=Path(__file__).resolve().parents[3]
RAW=ROOT/"benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112"
ANALYSIS=ROOT/"benchmarks/results/phase14b/analysis/root"
EDGE=ROOT/"benchmarks/results/phase14b/edge_cases/root"
OUT=ROOT/"benchmarks/results/phase14b/figures/root"
COLORS={"NUMERIX":"#275DAD","GSL":"#E0764F","CMINPACK":"#735A8C"}
METHODS=["bisection","regula_falsi","secant","newton","brent"]


def rows(path:Path)->list[dict[str,str]]:
    with path.open(newline="",encoding="utf-8") as f:return list(csv.DictReader(f))


def save(fig:plt.Figure,name:str)->None:
    fig.savefig(OUT/(name+".png"),dpi=180,bbox_inches="tight",facecolor="white")
    fig.savefig(OUT/(name+".svg"),bbox_inches="tight",facecolor="white")
    plt.close(fig)


def main()->int:
    OUT.mkdir(parents=True,exist_ok=True)
    accuracy=rows(RAW/"accuracy.csv");timing=rows(RAW/"raw_timing.csv")
    comparisons=rows(ANALYSIS/"paired_comparisons.csv")
    by_acc={(r["library"],r["requested_method"],r["problem_id"]):r for r in accuracy}
    times=defaultdict(list)
    for r in timing:
        key=(r["library"],r["requested_method"],r["problem_id"])
        times[key].append(float(r["wall_seconds"])/float(r["batch_size"]))
    paired={r["method"]:r for r in comparisons}

    # Per-method paired eligible samples: both libraries succeeded and met tolerance.
    medians={"NUMERIX":[],"GSL":[]};labels=[];sample_counts=[]
    for method in METHODS:
        ids=[]
        for (lib,m,pid),r in by_acc.items():
            if m!=method or lib!="NUMERIX":continue
            g=by_acc.get(("GSL",method,pid))
            if g and r["applicable"]==g["applicable"]=="1" and r["accuracy_pass"]==g["accuracy_pass"]=="1" and r["status"]==g["status"]=="0":
                if times[("NUMERIX",method,pid)] and times[("GSL",method,pid)]:ids.append(pid)
        if not ids:continue
        labels.append(method.replace("_"," ").title());sample_counts.append(len(ids))
        for lib in ("NUMERIX","GSL"):
            vals=[statistics.median(times[(lib,method,pid)]) for pid in ids]
            medians[lib].append(statistics.median(vals)*1e6)
    x=np.arange(len(labels));width=.35
    fig,ax=plt.subplots(figsize=(10,5.6))
    ax.bar(x-width/2,medians["NUMERIX"],width,label="NUMERIX",color=COLORS["NUMERIX"])
    ax.bar(x+width/2,medians["GSL"],width,label="GSL",color=COLORS["GSL"])
    ax.set_yscale("log");ax.set_ylabel("Median wall time per call (µs, log scale)")
    ax.set_xticks(x,labels);ax.set_title("Root solver runtime on paired accurate cases")
    ax.grid(axis="y",alpha=.25,which="both");ax.legend(frameon=False)
    for i,n in enumerate(sample_counts):ax.text(i,max(medians["NUMERIX"][i],medians["GSL"][i])*1.12,f"n={n}",ha="center",fontsize=8)
    save(fig,"root_direct_runtime_paired")

    ratios=[float(paired[m]["median_paired_speed_ratio"]) for m in METHODS]
    lows=[float(paired[m]["bootstrap_95ci_low"]) for m in METHODS]
    highs=[float(paired[m]["bootstrap_95ci_high"]) for m in METHODS]
    ns=[int(paired[m]["eligible_paired_problems"]) for m in METHODS]
    y=np.arange(len(METHODS))
    fig,ax=plt.subplots(figsize=(9,5.5))
    ax.errorbar(ratios,y,xerr=[np.array(ratios)-np.array(lows),np.array(highs)-np.array(ratios)],
        fmt="o",color=COLORS["NUMERIX"],ecolor="#65758B",capsize=4,markersize=6)
    ax.axvline(1,color="#303846",linewidth=1,linestyle="--")
    ax.set_xscale("log",base=2);ax.set_yticks(y,[m.replace("_"," ").title() for m in METHODS])
    ax.set_xticks([0.5,0.75,1.0,1.25,1.5,2.0,3.0],["0.5","0.75","1.0","1.25","1.5","2.0","3.0"])
    ax.invert_yaxis();ax.set_xlabel("GSL / NUMERIX median time (ratio > 1 favors NUMERIX)")
    ax.set_title("Paired root-method speed ratios with problem bootstrap 95% CI")
    ax.grid(axis="x",alpha=.25,which="both")
    for i,(v,n) in enumerate(zip(ratios,ns)):ax.text(highs[i]*1.04,i,f"n={n}",va="center",fontsize=8)
    save(fig,"root_direct_speed_ratio_forest")

    accuracy_by=defaultdict(list)
    for r in accuracy:accuracy_by[(r["library"],r["requested_method"])].append(r)
    methods=["bisection","regula_falsi","secant","newton","safeguarded_newton","brent","auto_selection","portfolio","multiple_root_discovery"]
    labels=[m.replace("_"," ").title() for m in methods];x=np.arange(len(methods));width=.37
    fig,ax=plt.subplots(figsize=(12,5.8))
    for off,lib in ((-width/2,"NUMERIX"),(width/2,"GSL")):
        rates=[];ns=[]
        for method in methods:
            rs=accuracy_by.get((lib,method),[]);rates.append(100*sum(r["accuracy_pass"]=="1" for r in rs)/len(rs) if rs else 0);ns.append(len(rs))
        bars=ax.bar(x+off,rates,width,label=lib,color=COLORS[lib])
        for b,n in zip(bars,ns):
            if n:ax.text(b.get_x()+b.get_width()/2,b.get_height()+1,f"n={n}",ha="center",fontsize=7,rotation=90)
    ax.set_ylim(0,112);ax.set_ylabel("Accuracy-pass rate across all attempted cases (%)")
    ax.set_xticks(x,labels,rotation=25,ha="right");ax.set_title("Root method accuracy-pass rate; failures remain in denominator")
    ax.grid(axis="y",alpha=.25);ax.legend(frameon=False)
    save(fig,"root_accuracy_pass_rates")

    fig,ax=plt.subplots(figsize=(8.5,5.2))
    discovery=[r for r in accuracy if r["requested_method"]=="multiple_root_discovery"]
    libs=["NUMERIX","GSL"]
    expected=[sum(int(r["expected_roots"]) for r in discovery if r["library"]==lib) for lib in libs]
    found=[sum(int(r["discovered_roots"]) for r in discovery if r["library"]==lib) for lib in libs]
    x=np.arange(2);ax.bar(x-.18,expected,.36,label="Reference roots",color="#929DAA")
    ax.bar(x+.18,found,.36,label="Detected roots",color=[COLORS[x] for x in libs])
    ax.set_xticks(x,libs);ax.set_ylabel("Root count across 100 problems")
    ax.set_title("Multiple-root discovery on the frozen root workloads")
    ax.legend(frameon=False);ax.grid(axis="y",alpha=.25)
    save(fig,"root_discovery_counts")

    edge=rows(EDGE/"accuracy.csv");groups=defaultdict(lambda:[0,0,0])
    for r in edge:
        k=(r["library"],r["requested_method"])
        groups[k][0]+=1;groups[k][1]+=r["status"]=="0" and r["converged"]=="1";groups[k][2]+=r["accuracy_pass"]=="1"
    names=[f"{lib} · {m.replace('_',' ')}" for lib,m in sorted(groups)]
    y=np.arange(len(names));attempt=[groups[k][0] for k in sorted(groups)];good=[groups[k][2] for k in sorted(groups)]
    fig,ax=plt.subplots(figsize=(10,8))
    ax.barh(y,attempt,label="Attempted",color="#DDE3EB");ax.barh(y,good,label="Accurate",color="#42A57A")
    ax.set_yticks(y,names,fontsize=7);ax.invert_yaxis();ax.set_xlabel("Edge cases")
    ax.set_title("Edge-case coverage and accuracy passes")
    ax.grid(axis="x",alpha=.25);ax.legend(frameon=False)
    save(fig,"root_edge_accuracy_counts")

    # Individual method charts are embedded inside their nested method cards.
    root_methods=["bisection","regula_falsi","secant","newton","safeguarded_newton",
                  "brent","auto_selection","portfolio","multiple_root_discovery"]
    for method in root_methods:
        libs=[lib for lib in ("NUMERIX","GSL","CMINPACK") if (lib,method) in accuracy_by]
        med=[];labels=[];passes=[];counts=[]
        for lib in libs:
            rs=accuracy_by[(lib,method)];problem_medians=[]
            for r in rs:
                samples=times.get((lib,method,r["problem_id"]),[])
                if samples:problem_medians.append(statistics.median(samples))
            if not problem_medians:continue
            labels.append(lib);med.append(statistics.median(problem_medians)*1e6)
            passes.append(100*sum(r["accuracy_pass"]=="1" for r in rs)/len(rs));counts.append(len(rs))
        if not labels:continue
        fig,ax=plt.subplots(figsize=(7.5,3.8))
        bars=ax.bar(labels,med,color=[COLORS[l] for l in labels])
        ax.set_yscale("log");ax.set_ylabel("Median wall time per call (µs, log scale)")
        ax.set_title(method.replace("_"," ").title()+" · all attempted cases")
        ax.grid(axis="y",alpha=.25,which="both")
        for bar,n,rate in zip(bars,counts,passes):
            ax.text(bar.get_x()+bar.get_width()/2,bar.get_height()*1.12,
                    f"n={n}; pass={rate:.0f}%",ha="center",fontsize=8)
        save(fig,"root_method_"+method+"_runtime")
    print(f"created method/global PNG and SVG root figures in {OUT.relative_to(ROOT)}")
    return 0


if __name__=="__main__":raise SystemExit(main())
