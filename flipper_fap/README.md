# Evil Inspector 918 Flipper FAP

External FAP that displays 918 Inspector monochrome artwork plus live telemetry from S3 UART0. RX-only. See root README for safe 2-wire signal+ground connection (Flipper TX physically disconnected). Build using the official matching Flipper `ufbt` SDK: `cd inspector_gadget && ufbt`. Do not install the FAP on the S3; it belongs on the Flipper microSD `/ext/apps/GPIO/`. The FAP does not implement remote radio transmission or scanner control.
