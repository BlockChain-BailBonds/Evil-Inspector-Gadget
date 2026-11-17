#pragma once

// Established Evil Inspector Gadget wiring. Keep this file authoritative for
// the active gateway bridge; UART0 remains the USB/programming console.
#define EIG_FLIPPER_UART_INSTANCE 1
#define EIG_FLIPPER_BAUD 115200
#define EIG_FLIPPER_RX_GPIO 3
#define EIG_FLIPPER_TX_GPIO 7

#define EIG_LCD_SCLK_GPIO 42
#define EIG_LCD_MOSI_GPIO 41
#define EIG_LCD_CS_GPIO 47
#define EIG_LCD_DC_GPIO 21
#define EIG_LCD_RST_GPIO 14
#define EIG_LCD_BACKLIGHT_GPIO 27

#if EIG_FLIPPER_UART_INSTANCE == 0
#error "EIG bridge must use UART1; UART0 is reserved for the host/programming console"
#endif
#if EIG_FLIPPER_RX_GPIO == 43 || EIG_FLIPPER_RX_GPIO == 44 || EIG_FLIPPER_TX_GPIO == 43 || EIG_FLIPPER_TX_GPIO == 44
#error "EIG bridge must not use conflicting GPIO43/GPIO44 assignments"
#endif
#if EIG_FLIPPER_RX_GPIO == EIG_FLIPPER_TX_GPIO
#error "EIG bridge RX and TX GPIOs must differ"
#endif
