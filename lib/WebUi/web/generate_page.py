#!/usr/bin/env python3
"""Embed web/index.html into src/WebPage.h as a PROGMEM string.

The ESP32 serves the page straight from flash, so there is no SPIFFS image to
build or upload. Edit index.html, then re-run this script:

    python3 lib/WebUi/web/generate_page.py
"""

from pathlib import Path

WEB_DIR = Path(__file__).resolve().parent
SOURCE = WEB_DIR / "index.html"
TARGET = WEB_DIR.parent / "src" / "WebPage.h"

HEADER = """\
#pragma once

// AUTO-GENERATED FILE - DO NOT EDIT BY HAND.
// Source:    lib/WebUi/web/index.html
// Regenerate: python3 lib/WebUi/web/generate_page.py

#include <Arduino.h>

// The complete single-page UI, stored in flash rather than RAM.
static const char WEB_PAGE_INDEX[] PROGMEM = R"rawhtml(
"""

FOOTER = """)rawhtml";
"""


def main() -> None:
    html = SOURCE.read_text(encoding="utf-8")

    # A raw string literal ends at the delimiter, so the HTML must never
    # contain it. Fail loudly instead of emitting a file that will not compile.
    if ')rawhtml"' in html:
        raise SystemExit("index.html contains the raw-string delimiter )rawhtml\"")

    TARGET.write_text(HEADER + html + FOOTER, encoding="utf-8")
    print(f"Wrote {TARGET.relative_to(WEB_DIR.parent.parent.parent)} "
          f"({len(html):,} bytes of HTML)")


if __name__ == "__main__":
    main()
