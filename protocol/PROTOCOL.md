# EIG protocol v1

Transport between phone and gateway is authenticated local HTTP over a configured private link. Gateway to Flipper is UART0 at 115200 baud, 8 data bits, no parity, 1 stop bit.

Frames are UTF-8 JSON Lines. Every request has `version`, `id`, `op`, `timestamp`, and `nonce`; every response has `version`, `id`, `ok`, and either `result` or `error`. Authentication uses an HMAC-SHA256 signature over the canonical JSON object without the `auth` member. Implementations must reject stale timestamps, duplicate nonces, unknown versions, malformed JSON, and invalid signatures.

Reserved operations in v1 are `ping`, `device_info`, `status`, and `stop`. RF, credential capture, persistence, and destructive actions are not part of this release protocol.

