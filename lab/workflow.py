#!/usr/bin/env python3
"""Minimal intake/planning/simulation workflow for the supported EIG profile."""

import argparse
import hashlib
import json
import re
import sys
import uuid
from pathlib import Path

ROOT = Path(__file__).parent
PROFILES = ROOT / "test_profiles.json"
FCC_RE = re.compile(r"\bFCC\s*ID\s*[:#]?\s*([A-Z0-9-]+)", re.I)
MODEL_RE = re.compile(r"\b(?:model|mdl|p/n)\s*[:#]?\s*([A-Z0-9][A-Z0-9._/-]{1,31})", re.I)


def extract_identifiers(label_text: str, photo_names: list[str] | None = None) -> dict:
    text = label_text or ""
    return {
        "fcc_ids": sorted(set(FCC_RE.findall(text.upper()))),
        "models": sorted(set(m.upper() for m in MODEL_RE.findall(text))),
        "brand_candidates": sorted(set(re.findall(r"\b(FLIPPER|918\s+TECHNOLOGIES|ESPRESSIF|SAMSUNG)\b", text, re.I))),
        "photo_references": list(photo_names or []),
        "ambiguous": not bool(FCC_RE.search(text) or MODEL_RE.search(text)),
    }


def load_profile(profile_id: str = "eig-readonly-bridge-inspection") -> dict:
    data = json.loads(PROFILES.read_text(encoding="utf-8"))
    for profile in data["profiles"]:
        if profile["id"] == profile_id:
            return {"region": data["region"], "profile": profile}
    raise ValueError(f"unknown profile: {profile_id}")


def analyze_capture(path: Path) -> dict:
    if not path.is_file():
        raise ValueError("capture file does not exist")
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    return {"path": str(path), "bytes": path.stat().st_size, "sha256": digest, "transmitted": False}


def simulation(profile: dict) -> dict:
    return {"run_id": str(uuid.uuid4()), "profile": profile["profile"]["id"], "simulated": True, "hardware_test": "UNVERIFIED", "results": [{"id": t["id"], "status": "SIMULATED", "evidence": t["evidence"]} for t in profile["profile"]["tests"]], "unsupported": profile["profile"]["unsupported"]}


def main() -> int:
    parser = argparse.ArgumentParser(description="EIG minimal lab workflow; no radio/GPIO transmission")
    sub = parser.add_subparsers(dest="command", required=True)
    identify = sub.add_parser("identify")
    identify.add_argument("--label-text", default="")
    identify.add_argument("--photo", action="append", default=[])
    sub.add_parser("plan")
    sub.add_parser("simulate")
    capture = sub.add_parser("analyze-capture")
    capture.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.command == "identify":
        result = extract_identifiers(args.label_text, args.photo)
    elif args.command == "plan":
        result = load_profile()
    elif args.command == "simulate":
        result = simulation(load_profile())
    else:
        result = analyze_capture(args.path)
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
