#!/usr/bin/env python3
"""Render the Phase 13C/13D Markdown evaluation as a self-contained HTML file."""
import argparse
from html import escape
from pathlib import Path

import markdown


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = ROOT / "documentation/reports/NUMERIX_comparative_evaluation.md"
DEFAULT_OUTPUT = ROOT / "documentation/reports/NUMERIX_comparative_evaluation.html"

STYLE = """
body { max-width: 1120px; margin: 2rem auto; padding: 0 1.25rem;
       color: #20242a; font: 16px/1.58 system-ui, sans-serif; }
h1, h2, h3 { line-height: 1.25; margin-top: 1.8em; }
h1 { margin-top: 0; }
table { border-collapse: collapse; width: 100%; margin: 1rem 0 1.5rem; }
th, td { border: 1px solid #cbd2da; padding: .45rem .6rem; text-align: left; }
th { background: #eef2f6; }
tr:nth-child(even) { background: #f8fafc; }
img { display: block; max-width: 100%; height: auto; margin: 1rem auto 1.8rem; }
code, pre { font-family: ui-monospace, monospace; }
pre { padding: 1rem; overflow-x: auto; background: #f2f4f7; }
a { color: #075ea8; }
"""


def render(input_path, output_path):
    source = input_path.read_text(encoding="utf-8")
    body = markdown.markdown(source, extensions=["tables", "fenced_code", "toc"])
    document = (
        "<!doctype html>\n<html lang=\"en\">\n<head>\n"
        "<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        f"<title>{escape(input_path.stem.replace('_', ' '))}</title>\n"
        f"<style>{STYLE}</style>\n</head>\n<body>\n{body}\n</body>\n</html>\n"
    )
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(document, encoding="utf-8")
    return len(body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    blocks = render(args.input, args.output)
    print(f"Rendered {blocks} characters to {args.output}")


if __name__ == "__main__":
    main()
