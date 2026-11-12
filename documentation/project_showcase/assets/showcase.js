(() => {
  "use strict";

  const escapeHtml = (value) =>
    String(value ?? "").replace(/[&<>"']/g, (char) => ({
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      '"': "&quot;",
      "'": "&#39;"
    }[char]));

  function initCopyButtons() {
    document.querySelectorAll(".copy-button").forEach((button) => {
      button.addEventListener("click", () => {
        const block = button.closest(".code-block");
        const code = block ? block.querySelector("code") : null;
        const text = code ? code.textContent : "";
        navigator.clipboard
          .writeText(text)
          .then(() => {
            const original = button.textContent;
            button.textContent = "copied";
            button.classList.add("copied");
            setTimeout(() => {
              button.textContent = original;
              button.classList.remove("copied");
            }, 1400);
          })
          .catch(() => {
            button.textContent = "failed";
            setTimeout(() => {
              button.textContent = "copy";
            }, 1400);
          });
      });
    });
  }

  function initTabs() {
    document.querySelectorAll(".tabs").forEach((tabList) => {
      const tabs = Array.from(tabList.querySelectorAll(".tab"));
      const panels = tabs.map((tab) => {
        const id = tab.getAttribute("data-panel");
        return id ? document.getElementById(id) : null;
      });
      const activate = (index) => {
        tabs.forEach((t, i) => {
          const active = i === index;
          t.setAttribute("aria-selected", String(active));
          t.classList.toggle("active", active);
          if (panels[i]) panels[i].classList.toggle("active", active);
        });
      };
      tabs.forEach((tab, index) => {
        tab.addEventListener("click", () => activate(index));
        tab.addEventListener("keydown", (event) => {
          if (event.key === "ArrowRight") {
            event.preventDefault();
            activate((index + 1) % tabs.length);
          } else if (event.key === "ArrowLeft") {
            event.preventDefault();
            activate((index - 1 + tabs.length) % tabs.length);
          } else if (event.key === "Home") {
            event.preventDefault();
            activate(0);
          } else if (event.key === "End") {
            event.preventDefault();
            activate(tabs.length - 1);
          }
        });
      });
      activate(0);
    });
  }

  function initMobileMenu() {
    const button = document.querySelector(".menu-button");
    const menu = document.querySelector(".mobile-menu");
    const overlay = document.querySelector(".menu-overlay");
    const closeButton = document.querySelector(".mobile-menu .close-button");
    if (!button || !menu || !overlay) return;

    function openMenu() {
      menu.classList.add("open");
      overlay.style.display = "block";
      button.setAttribute("aria-expanded", "true");
      document.body.style.overflow = "hidden";
    }
    function closeMenu() {
      menu.classList.remove("open");
      overlay.style.display = "none";
      button.setAttribute("aria-expanded", "false");
      document.body.style.overflow = "";
    }
    button.addEventListener("click", openMenu);
    closeButton.addEventListener("click", closeMenu);
    overlay.addEventListener("click", closeMenu);
    menu.querySelectorAll("a").forEach((link) => {
      link.addEventListener("click", closeMenu);
    });
    document.addEventListener("keydown", (event) => {
      if (event.key === "Escape" && menu.classList.contains("open")) closeMenu();
    });
  }

  function initProgress() {
    const track = document.querySelector(".progress-track");
    const fill = document.getElementById("progress-fill");
    if (!track || !fill) return;
    const sections = Array.from(document.querySelectorAll("main > section"));
    const update = () => {
      let bestIndex = 0;
      let bestDistance = Infinity;
      const viewHeight = innerHeight;
      sections.forEach((section, index) => {
        const rect = section.getBoundingClientRect();
        const distance =
          Math.abs(rect.top) +
          (rect.bottom < 0 || rect.top > viewHeight ? viewHeight : 0);
        if (distance < bestDistance) {
          bestDistance = distance;
          bestIndex = index;
        }
      });
      const pct =
        sections.length > 1
          ? (bestIndex / (sections.length - 1)) * 100
          : 100;
      fill.style.width = `${pct}%`;
    };
    update();
    window.addEventListener("scroll", update, { passive: true });
  }

  function initAnchorScroll() {
    document.querySelectorAll('a[href^="#"]').forEach((link) => {
      link.addEventListener("click", (event) => {
        const id = link.getAttribute("href").slice(1);
        const target = document.getElementById(id);
        if (target) {
          event.preventDefault();
          target.scrollIntoView({ behavior: "smooth", block: "start" });
          history.pushState(null, "", `#${id}`);
        }
      });
    });
  }

  function initSectionHighlights() {
    const links = Array.from(
      document.querySelectorAll("nav.section-nav a, .mobile-menu a")
    );
    const targets = links
      .map((link) => document.getElementById(link.getAttribute("href").slice(1)))
      .filter(Boolean);
    if (!links.length || !targets.length) return;
    const update = () => {
      const viewHeight = innerHeight;
      let bestIndex = 0;
      let bestDistance = Infinity;
      targets.forEach((target, index) => {
        const rect = target.getBoundingClientRect();
        const distance =
          Math.abs(rect.top) +
          (rect.bottom < 0 || rect.top > viewHeight ? viewHeight : 0);
        if (distance < bestDistance) {
          bestDistance = distance;
          bestIndex = index;
        }
      });
      links.forEach((link, index) => {
        link.setAttribute("aria-current", index === bestIndex ? "page" : null);
      });
    };
    update();
    window.addEventListener("scroll", update, { passive: true });
  }

  document.addEventListener("DOMContentLoaded", () => {
    initCopyButtons();
    initTabs();
    initMobileMenu();
    initProgress();
    initAnchorScroll();
    initSectionHighlights();
  });
})();
