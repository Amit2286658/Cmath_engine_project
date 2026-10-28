#!/usr/bin/env python3
"""Scans Main/patches/*.math and generates Main/patches/manifest.json."""
import hashlib
import json
from pathlib import Path

PATCHES_DIR = Path(__file__).resolve().parent.parent / "patches"


def sha256_hex(path: Path) -> str:
    h = hashlib.sha256()
    h.update(path.read_bytes())
    return h.hexdigest()


def main() -> None:
    PATCHES_DIR.mkdir(parents=True, exist_ok=True)
    entries = []
    for math_file in sorted(PATCHES_DIR.glob("*.math")):
        entries.append(
            {
                "file": math_file.name,
                "sha256": sha256_hex(math_file),
                "size": math_file.stat().st_size,
            }
        )
    manifest = {"version": 1, "patches": entries}
    out_path = PATCHES_DIR / "manifest.json"
    out_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"[patches] Wrote {len(entries)} patch(es) to {out_path}")


if __name__ == "__main__":
    main()
