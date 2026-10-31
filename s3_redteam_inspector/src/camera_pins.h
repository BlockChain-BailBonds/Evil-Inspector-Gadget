#pragma once
// CAMERA_PROFILE=1: ESP32-S3-EYE style OV2640 parallel DVP bus.
// Verified with Espressif ESP32-S3-EYE BSP; may also appear on generic N16R8 CAM modules.
// N16R8 alone DOES NOT establish camera routing. If uncertain use CAMERA_PROFILE=0.
#ifndef CAMERA_PROFILE
#define CAMERA_PROFILE 0
#endif
#if CAMERA_PROFILE != 0 && CAMERA_PROFILE != 1
#error "Unsupported CAMERA_PROFILE. Use 0 (off) or 1 (S3-EYE wiring)."
#endif

namespace mermaid_cam {
constexpr int pwdn = -1;
constexpr int reset = -1;
constexpr int xclk = 15;
constexpr int sda = 4;
constexpr int scl = 5;
constexpr int d0 = 11;
constexpr int d1 = 9;
constexpr int d2 = 8;
constexpr int d3 = 10;
constexpr int d4 = 12;
constexpr int d5 = 18;
constexpr int d6 = 17;
constexpr int d7 = 16;
constexpr int vsync = 6;
constexpr int href = 7;
constexpr int pclk = 13;
}
