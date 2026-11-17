# EIG protocol v1

Transport between phone and gateway is authenticated local HTTP over a configured private link. Gateway to Flipper is UART1 at 115200 baud, 8 data bits, no parity, 1 stop bit: ESP32-S3 GPIO3 receives Flipper TX and GPIO7 transmits to Flipper RX. UART0 remains the USB/programming console.

Frames are UTF-8 JSON Lines. Every request has `version`, `id`, `op`, `timestamp`, and `nonce`; every response has `version`, `id`, `ok`, and either `result` or `error`. Authentication uses an HMAC-SHA256 signature over the canonical JSON object without the `auth` member. Implementations must reject stale timestamps, duplicate nonces, unknown versions, malformed JSON, and invalid signatures.

Reserved operations in v1 are `ping`, `device_info`, `status`, and `stop`. RF, credential capture, persistence, and destructive actions are not part of this release protocol.

