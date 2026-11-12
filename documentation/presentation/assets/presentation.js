(() => {
  "use strict";

  const DATA = window.NUMERIX_BENCHMARK_DATA;
  const slides = Array.from(document.querySelectorAll("#deck > .slide"));
  const progress = document.getElementById("progress");
  const position = document.getElementById("section-position");
  const dialog = document.getElementById("chart-dialog");
  const dialogContent = document.getElementById("dialog-content");
  const methodOrder = [
    "bisection", "regula_falsi", "secant", "newton", "safeguarded_newton",
    "brent", "auto_selection", "portfolio", "multiple_root_discovery"
  ];
  const methodNames = {
    bisection: "Bisection",
    regula_falsi: "Regula Falsi",
    secant: "Secant",
    newton: "Newton-Raphson",
    safeguarded_newton: "Safeguarded Newton",
    brent: "Brent",
    auto_selection: "Automatic method selection",
    portfolio: "Solver portfolio",
    multiple_root_discovery: "Multiple-root discovery"
  };
  const libraryColors = { NUMERIX: "num", GSL: "gsl", CMINPACK: "cmin" };
  const numberFormat = new Intl.NumberFormat("en", { maximumFractionDigits: 3 });

  const escapeHtml = (value) => String(value ?? "").replace(/[&<>"']/g, (char) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  }[char]));
  const humanize = (key) => methodNames[key] || String(key).replaceAll("_", " ").replace(/\b\w/g, (m) => m.toUpperCase());
  const fmt = (value, digits = 4) => Number.isFinite(value) ? Number(value).toPrecision(digits) : "not available";
  const fmtUs = (value) => Number.isFinite(value) ? `${numberFormat.format(value)} µs` : "not available";

  function setActiveSlide(index) {
    const bounded = Math.max(0, Math.min(slides.length - 1, index));
    progress.style.width = `${slides.length > 1 ? bounded / (slides.length - 1) * 100 : 100}%`;
    position.textContent = `${String(bounded + 1).padStart(2, "0")} / ${String(slides.length).padStart(2, "0")}`;
  }

  if ("IntersectionObserver" in window) {
    const visible = new Map();
    const observer = new IntersectionObserver((entries) => {
      entries.forEach((entry) => {
        if (entry.isIntersecting) visible.set(entry.target, entry.intersectionRatio);
        else visible.delete(entry.target);
      });
      const current = Array.from(visible.entries()).sort((a, b) => b[1] - a[1])[0];
      if (current) setActiveSlide(slides.indexOf(current[0]));
    }, { threshold: [0.18, 0.35, 0.55, 0.72] });
    slides.forEach((slide) => observer.observe(slide));
  } else {
    window.addEventListener("scroll", () => {
      let nearest = 0;
      let distance = Infinity;
      slides.forEach((slide, index) => {
        const value = Math.abs(slide.getBoundingClientRect().top);
        if (value < distance) { nearest = index; distance = value; }
      });
      setActiveSlide(nearest);
    }, { passive: true });
  }
  setActiveSlide(0);

  function goToSlide(index) {
    const slide = slides[Math.max(0, Math.min(slides.length - 1, index))];
    if (slide) slide.scrollIntoView({ behavior: matchMedia("(prefers-reduced-motion: reduce)").matches ? "auto" : "smooth", block: "start" });
  }
  function currentSlideIndex() {
    let bestIndex = 0;
    let bestDistance = Infinity;
    slides.forEach((slide, index) => {
      const rect = slide.getBoundingClientRect();
      const distance = Math.abs(rect.top) + (rect.bottom < 0 || rect.top > innerHeight ? innerHeight : 0);
      if (distance < bestDistance) { bestDistance = distance; bestIndex = index; }
    });
    return bestIndex;
  }

  const presentButton = document.getElementById("present");
  async function toggleFullscreen() {
    try {
      if (document.fullscreenElement) await document.exitFullscreen();
      else await document.documentElement.requestFullscreen();
    } catch (error) {
      presentButton.title = `Fullscreen was blocked by the browser: ${error.message}`;
    }
  }
  presentButton.addEventListener("click", toggleFullscreen);
  document.addEventListener("fullscreenchange", () => {
    const active = Boolean(document.fullscreenElement);
    presentButton.setAttribute("aria-pressed", String(active));
    presentButton.innerHTML = active ? "⛶ <span>Exit presentation</span>" : "⛶ <span>Present</span>";
  });
  document.addEventListener("keydown", (event) => {
    if (dialog?.open) return;
    if (event.key === "Escape" && document.fullscreenElement) {
      event.preventDefault();
      document.exitFullscreen().catch(() => {});
      return;
    }
    const target = event.target;
    if (target && (target.isContentEditable || /INPUT|TEXTAREA|SELECT|BUTTON/.test(target.tagName))) return;
    const key = event.key.toLowerCase();
    if (["arrowright", "arrowdown", "pagedown", " "].includes(key)) {
      event.preventDefault(); goToSlide(currentSlideIndex() + 1);
    } else if (["arrowleft", "arrowup", "pageup"].includes(key)) {
      event.preventDefault(); goToSlide(currentSlideIndex() - 1);
    } else if (key === "f") {
      event.preventDefault(); toggleFullscreen();
    }
  });

  function chartCard(title, subtitle, svg, note, full = false) {
    return `<article class="chart-card${full ? " full" : ""}">
      <div class="chart-title"><b>${escapeHtml(title)}</b><button class="chart-expand" type="button" aria-label="Enlarge ${escapeHtml(title)} graph">Enlarge ↗</button></div>
      <small class="chart-subtitle">${escapeHtml(subtitle)}</small>${svg}
      <p class="chart-note">${escapeHtml(note)}</p></article>`;
  }

  function runtimeSvg(routes, methodLabel) {
    const measured = routes.filter((route) => Number.isFinite(route.median_wall_microseconds_per_call));
    if (!measured.length) return "";
    const width = 700, row = 31, top = 17, bottom = 42, left = 125, right = 70;
    const height = top + measured.length * row + bottom;
    const plot = width - left - right;
    const max = Math.max(...measured.map((route) => route.median_wall_microseconds_per_call)) * 1.16;
    const ticks = [0, 0.25, 0.5, 0.75, 1];
    const grid = ticks.map((t) => {
      const x = left + t * plot;
      return `<line class="grid" x1="${x}" y1="${top - 3}" x2="${x}" y2="${height - bottom + 2}"/><text x="${x}" y="${height - 20}" text-anchor="middle">${numberFormat.format(t * max)}</text>`;
    }).join("");
    const bars = measured.map((route, i) => {
      const y = top + i * row + 4;
      const value = route.median_wall_microseconds_per_call;
      const barWidth = Math.max(1, value / max * plot);
      const css = libraryColors[route.library] || "num";
      const tip = `${route.library}; all-attempt median ${fmt(value, 5)} µs/call; ${route.status_successes}/${route.attempted_problems} successful; ${route.accuracy_passes}/${route.attempted_problems} accuracy passes; ${route.timing_batch_sample_count} timing batches`;
      return `<text x="${left - 9}" y="${y + 12}" text-anchor="end">${escapeHtml(route.library)}</text><rect class="bar-${css}" x="${left}" y="${y}" width="${barWidth}" height="16" rx="5" tabindex="0"><title>${escapeHtml(tip)}</title></rect><text x="${Math.min(left + barWidth + 6, width - right + 3)}" y="${y + 12}">${fmt(value, 4)}</text>`;
    }).join("");
    return `<svg class="chart-svg" viewBox="0 0 ${width} ${height}" role="img" aria-label="${escapeHtml(methodLabel)} all-attempt median runtime in microseconds per call by library">${grid}${bars}<line class="axis" x1="${left}" y1="${height - bottom + 2}" x2="${width - right}" y2="${height - bottom + 2}"/><text x="${left + plot / 2}" y="${height - 3}" text-anchor="middle">Median wall time (µs / call)</text></svg>`;
  }

  function pairedSvg(pair, methodLabel) {
    if (!pair) return "";
    const [low, high] = pair.bootstrap_95ci;
    const ratio = pair.median_competitor_over_numerix;
    const min = Math.min(0.94, Math.floor(low * 100) / 100);
    const max = Math.max(1.12, Math.ceil(high * 100) / 100);
    const width = 700, height = 148, left = 52, right = 32, top = 22, bottom = 43;
    const plot = width - left - right;
    const x = (v) => left + (v - min) / (max - min) * plot;
    const steps = 5;
    const ticks = Array.from({ length: steps + 1 }, (_, i) => min + (max - min) * i / steps);
    const grid = ticks.map((tick) => `<line class="grid" x1="${x(tick)}" y1="${top}" x2="${x(tick)}" y2="${height - bottom + 2}"/><text x="${x(tick)}" y="${height - 19}" text-anchor="middle">${tick.toFixed(2)}</text>`).join("");
    const reference = `<line class="reference" x1="${x(1)}" y1="${top - 4}" x2="${x(1)}" y2="${height - bottom + 2}"/><text x="${x(1)}" y="${top - 8}" text-anchor="middle">equal runtime</text>`;
    const tip = `Median paired GSL / NUMERIX runtime ratio ${fmt(ratio, 5)}; descriptive 95% problem-bootstrap interval ${fmt(low, 5)} to ${fmt(high, 5)}; ${pair.eligible_paired_problems} eligible pairs from 100 frozen problems; ${pair.bootstrap_resamples} resamples; no hypothesis test.`;
    const marker = `<line class="whisker" x1="${x(low)}" y1="73" x2="${x(high)}" y2="73"/><line class="whisker" x1="${x(low)}" y1="65" x2="${x(low)}" y2="81"/><line class="whisker" x1="${x(high)}" y1="65" x2="${x(high)}" y2="81"/><circle class="ratio-point" cx="${x(ratio)}" cy="73" r="7" tabindex="0"><title>${escapeHtml(tip)}</title></circle><text x="${x(ratio)}" y="50" text-anchor="middle">${ratio.toFixed(4)}</text>`;
    return `<svg class="chart-svg" viewBox="0 0 ${width} ${height}" role="img" aria-label="${escapeHtml(methodLabel)} paired GSL divided by NUMERIX median runtime ratio with descriptive 95 percent bootstrap interval">${grid}${reference}${marker}<line class="axis" x1="${left}" y1="${height - bottom + 2}" x2="${width - right}" y2="${height - bottom + 2}"/><text x="${left + plot / 2}" y="${height - 3}" text-anchor="middle">GSL / NUMERIX median paired time · above 1 favors NUMERIX</text></svg>`;
  }

  function scatterSvg(routes, methodLabel) {
    const points = [];
    let unavailableError = 0;
    routes.forEach((route) => (route.problem_points || []).forEach((point) => {
      if (!Number.isFinite(point.median_wall_microseconds)) return;
      if (!Number.isFinite(point.absolute_error)) { unavailableError += 1; return; }
      points.push({ route, point });
    }));
    if (!points.length) return "";
    const width = 700, height = 270, left = 65, right = 22, top = 18, bottom = 55;
    const plotW = width - left - right, plotH = height - top - bottom;
    const times = points.map(({ point }) => point.median_wall_microseconds).filter((v) => v > 0);
    const errors = points.map(({ point }) => point.absolute_error).filter((v) => v > 0);
    const xMin = Math.min(...times), xMax = Math.max(...times);
    const logMin = errors.length ? Math.log10(Math.min(...errors)) : -15;
    const logMax = errors.length ? Math.log10(Math.max(...errors)) : -14;
    const x = (value) => left + (value - xMin) / Math.max(xMax - xMin, Number.EPSILON) * plotW;
    const y = (value) => value === 0 ? top + plotH : top + (1 - (Math.log10(value) - logMin) / Math.max(logMax - logMin, 1)) * (plotH - 8);
    const xTicks = [xMin, (xMin + xMax) / 2, xMax];
    const xGrid = xTicks.map((tick) => `<line class="grid" x1="${x(tick)}" y1="${top}" x2="${x(tick)}" y2="${top + plotH}"/><text x="${x(tick)}" y="${height - 34}" text-anchor="middle">${numberFormat.format(tick)}</text>`).join("");
    const yExponents = [];
    if (errors.length) {
      const lo = Math.floor(logMin), hi = Math.ceil(logMax);
      const stride = Math.max(1, Math.ceil((hi - lo) / 5));
      for (let exp = lo; exp <= hi; exp += stride) yExponents.push(exp);
      if (!yExponents.includes(hi)) yExponents.push(hi);
    }
    const yGrid = yExponents.map((exp) => {
      const value = 10 ** exp, py = y(value);
      return `<line class="grid" x1="${left}" y1="${py}" x2="${left + plotW}" y2="${py}"/><text x="${left - 8}" y="${py + 3}" text-anchor="end">10^${exp}</text>`;
    }).join("");
    const zeroLabel = `<line class="grid" x1="${left}" y1="${top + plotH}" x2="${left + plotW}" y2="${top + plotH}"/><text x="${left - 8}" y="${top + plotH + 3}" text-anchor="end">0</text>`;
    const circles = points.map(({ route, point }) => {
      const cls = libraryColors[route.library] || "num";
      const failed = !point.status_success || !point.accuracy_pass ? " failed" : "";
      const tip = `${route.library} · ${point.problem_id} · ${fmt(point.median_wall_microseconds, 5)} µs/problem median · absolute root-position error ${point.absolute_error === 0 ? "0" : point.absolute_error.toExponential(4)} · status ${point.status_success ? "success" : "failure"} · accuracy ${point.accuracy_pass ? "pass" : "fail"} · ${point.timing_batches} timing batches`;
      return `<circle class="data-point point-${cls}${failed}" cx="${x(point.median_wall_microseconds)}" cy="${y(point.absolute_error)}" r="2.7" tabindex="0"><title>${escapeHtml(tip)}</title></circle>`;
    }).join("");
    const note = unavailableError ? `${unavailableError} attempted route/problem values have no finite reported error; omitted from this scatter.` : "All plotted values are retained, including failures with a finite error.";
    return `<svg class="chart-svg" viewBox="0 0 ${width} ${height}" role="img" aria-label="${escapeHtml(methodLabel)} observed per-problem runtime versus absolute position error at one fixed tolerance">${xGrid}${yGrid}${zeroLabel}${circles}<line class="axis" x1="${left}" y1="${top + plotH}" x2="${left + plotW}" y2="${top + plotH}"/><line class="axis" x1="${left}" y1="${top}" x2="${left}" y2="${top + plotH}"/><text x="${left + plotW / 2}" y="${height - 7}" text-anchor="middle">Per-problem median time (µs)</text><text x="13" y="${top + plotH / 2}" text-anchor="middle" transform="rotate(-90 13 ${top + plotH / 2})">Absolute position error · log; zero on baseline</text></svg><span class="chart-note">${escapeHtml(note)}</span>`;
  }

  function resultRows(routes) {
    return `<div class="route-results" role="table" aria-label="Per-library completion and accuracy counts">${routes.map((route) => `<div class="route-result" role="row"><b role="cell"><i class="legend-dot ${libraryColors[route.library] || "num"}" aria-hidden="true"></i>${escapeHtml(route.library)}</b><span role="cell">Status success <strong>${route.status_successes}/${route.attempted_problems}</strong></span><span role="cell">Accuracy pass <strong>${route.accuracy_passes}/${route.attempted_problems}</strong></span><span role="cell">Both <strong>${route.successful_and_accurate}/${route.attempted_problems}</strong></span></div>`).join("")}</div>`;
  }

  function routeComparisonLabel(routes) {
    const hasDirect = routes.some((route) => route.paired_comparison);
    const hasBest = routes.some((route) => route.comparison_class === "best_available");
    const libraries = new Set(routes.map((route) => route.library));
    if (hasDirect && hasBest) return "Direct GSL comparison + best-available route";
    if (hasDirect) return "Direct algorithm-family comparison";
    if (libraries.size > 1) return "Best-available comparison · methods differ";
    return "NUMERIX-only operation · no competitor result";
  }

  function observationFor(pair) {
    if (!pair) return "No direct paired ratio is available for this route. Interpret all-attempt medians with the algorithm classification and accuracy counts above.";
    const [low, high] = pair.bootstrap_95ci;
    const ratio = pair.median_competitor_over_numerix;
    if (low <= 1 && high >= 1) return `Median ratio ${ratio.toFixed(4)} is above 1, but its descriptive interval includes 1; treat the speed difference as inconclusive.`;
    if (low > 1) return `NUMERIX had lower median wall time on ${pair.eligible_paired_problems} paired problems that both implementations solved accurately. This is descriptive evidence; the study made no significance claim.`;
    return `GSL had lower median wall time on ${pair.eligible_paired_problems} eligible paired problems. This is descriptive evidence; the study made no significance claim.`;
  }

  function renderRootComparisons() {
    const target = document.getElementById("benchmark-content");
    if (!target) return;
    if (!DATA || !Array.isArray(DATA.methods) || !Array.isArray(DATA.coverage)) {
      target.innerHTML = '<p class="no-data">Local benchmark payload is unavailable. Regenerate it using the instructions in the presentation README.</p>';
      return;
    }
    const rootAudit = DATA.coverage.find((entry) => entry.module === "root");
    const rootOps = rootAudit?.operations || [];
    const ratioByMethod = new Map((DATA.paired_comparisons || []).map((pair) => [pair.method, pair]));
    const sections = [];
    const rootMethods = methodOrder.map((method) => ({
      key: method,
      routes: DATA.methods.filter((route) => route.method === method),
      operation: rootOps.find((op) => op.public_symbol && DATA.methods.some((route) => route.method === method && route.library === "NUMERIX" && route.operation === op.operation))
        || rootOps.find((op) => op.public_symbol.toLowerCase().includes(method.split("_")[0]))
    })).filter((entry) => entry.routes.length);

    const rootMarkup = rootMethods.map(({ key, routes, operation }) => {
      const label = humanize(key);
      const pair = ratioByMethod.get(key) || null;
      const symbol = operation?.public_symbol || ({
        bisection: "numerix_root_bisection", regula_falsi: "numerix_root_regula_falsi",
        secant: "numerix_root_secant", newton: "numerix_root_newton",
        safeguarded_newton: "numerix_root_safeguarded_newton", brent: "numerix_root_brent",
        auto_selection: "numerix_root_solve_auto", portfolio: "numerix_root_solve_portfolio",
        multiple_root_discovery: "numerix_root_find_all"
      }[key]);
      const details = [...new Set(routes.map((route) => route.comparison_detail).filter(Boolean))];
      const routeInfo = details.map((detail) => `<span>${escapeHtml(detail)}</span>`).join("");
      const runtime = runtimeSvg(routes, label);
      const ratio = pair ? pairedSvg(pair, label) : "";
      const scatter = scatterSvg(routes, label);
      const pairedCard = pair ? chartCard("Paired speed ratio", `${pair.eligible_paired_problems} eligible paired problems · 5,000 descriptive bootstrap resamples`, ratio, observationFor(pair)) : `<div class="chart-card"><b>Paired comparison</b><p class="no-data">${routes.length === 1 ? "NUMERIX-only route; no competitor ratio exists." : "No direct paired ratio was reported for this best-available comparison."}</p></div>`;
      const trendCard = scatter ? chartCard("Observed runtime vs error", "Distinct frozen problems · one fixed tolerance · dots are individual problems", scatter, "This is a cross-problem outcome scatter, not a tolerance sweep or time-to-accuracy curve. Hover points to inspect exact values.", true) : "";
      return `<article class="method-block" id="method-${escapeHtml(key.replaceAll("_", "-"))}">
        <div class="method-heading"><h4>${escapeHtml(label)}</h4><span class="badge ${pair ? "measured" : "pending"}">${escapeHtml(routeComparisonLabel(routes))}</span></div>
        <code class="method-symbol">${escapeHtml(symbol || "Public API method")}</code>
        <p class="method-summary">${routeInfo} ${pair ? escapeHtml(observationFor(pair)) : ""}</p>
        <div class="chart-grid">${runtime ? chartCard("Execution time", "Median wall time per call across all attempted workloads", runtime, "Units are microseconds per call. Route medians retain each route's own attempted problems; paired ratios use a separate eligible set.") : ""}${pairedCard}${resultRows(routes)}${trendCard}</div>
      </article>`;
    }).join("");
    sections.push(`<section class="benchmark-module root-comparison-module"><h3>Root finding <span class="badge measured">9 measured routes</span></h3><p>Each route shows all-attempt runtime and status/accuracy counts. Direct paired ratios are restricted to five like-for-like GSL comparisons; remaining methods are labeled best-available or NUMERIX-only.</p>${rootMarkup}</section>`);

    (DATA.coverage || []).filter((module) => module.module !== "root").forEach((module) => {
      const operations = module.operations || [];
      const pending = operations.map((op) => `<article class="pending-method"><div><b>${escapeHtml(op.operation)}</b><code>${escapeHtml(op.public_symbol)}</code><small>${escapeHtml(op.implementation_source)}</small></div><div><span class="badge pending">Method study pending</span><small>Candidate: ${escapeHtml(op.competitor_equivalent || "No equivalent verified")}</small><small>Class: ${escapeHtml(op.comparison_class || "Not classified")}</small></div></article>`).join("");
      sections.push(`<section class="benchmark-module pending-module"><h3>${escapeHtml(module.label)} <span class="badge pending">Method-level study pending</span></h3><p>${escapeHtml(module.note)} The capability inventory below is not benchmark evidence.</p><div class="pending-methods">${pending || '<p class="no-data">No audited operations listed.</p>'}</div></section>`);
    });
    target.innerHTML = sections.join("");
    bindChartEnlargement();
  }

  function bindChartEnlargement() {
    document.querySelectorAll(".chart-expand").forEach((button) => button.addEventListener("click", () => {
      if (!dialog || !dialogContent) return;
      const card = button.closest(".chart-card");
      const clone = card.cloneNode(true);
      clone.querySelector(".chart-expand")?.remove();
      dialogContent.replaceChildren(clone);
      if (typeof dialog.showModal === "function") dialog.showModal();
      else dialog.setAttribute("open", "");
      document.getElementById("close-dialog")?.focus();
    }));
  }
  document.getElementById("close-dialog")?.addEventListener("click", () => dialog?.close());
  dialog?.addEventListener("click", (event) => { if (event.target === dialog) dialog.close(); });
  dialog?.addEventListener("close", () => dialogContent?.replaceChildren());

  renderRootComparisons();
  const footer = document.querySelector(".site-footer span:nth-child(2)");
  if (footer) footer.textContent = "Keyboard: ↑ / ↓ or ← / → sections · F full screen · Esc exits";
})();
