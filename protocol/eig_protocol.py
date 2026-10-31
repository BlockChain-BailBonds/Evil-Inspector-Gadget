"""Small dependency-free EIG v1 framing and authentication implementation."""
from __future__ import annotations
import hashlib, hmac, json, time
from typing import Any

VERSION = 1

def canonical(obj: dict[str, Any]) -> bytes:
    return json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()

def sign(request: dict[str, Any], secret: bytes) -> str:
    unsigned = {k: v for k, v in request.items() if k != "auth"}
    return hmac.new(secret, canonical(unsigned), hashlib.sha256).hexdigest()

def make_request(op: str, request_id: str, secret: bytes, *, nonce: str, timestamp: int | None = None, args: dict[str, Any] | None = None) -> dict[str, Any]:
    request = {"version": VERSION, "id": request_id, "op": op, "timestamp": int(time.time()) if timestamp is None else timestamp, "nonce": nonce}
    if args:
        request["args"] = args
    request["auth"] = sign(request, secret)
    return request

def verify(request: dict[str, Any], secret: bytes, *, now: int | None = None, max_age: int = 60) -> None:
    if request.get("version") != VERSION or not isinstance(request.get("auth"), str):
        raise ValueError("unsupported version or missing auth")
    timestamp = request.get("timestamp")
    if not isinstance(timestamp, int) or abs((int(time.time()) if now is None else now) - timestamp) > max_age:
        raise ValueError("stale timestamp")
    expected = sign(request, secret)
    if not hmac.compare_digest(expected, request["auth"]):
        raise ValueError("invalid authentication")

def encode_frame(obj: dict[str, Any]) -> bytes:
    return canonical(obj) + b"\n"

