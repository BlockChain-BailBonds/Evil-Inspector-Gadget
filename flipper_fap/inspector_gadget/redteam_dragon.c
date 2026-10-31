// 918 Technologies / Evil Inspector Gadget - Flipper UART0 receive-only launcher
// This FAP intentionally NEVER transmits UART data or sends radio frames itself.
#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <expansion/expansion.h>
#include <loader/loader.h>
#include <stdlib.h>
#include <string.h>
#include "dragon_core.h"
#include "inspector_assets.h"
#include "inspector_link.h"
#include <stdio.h>

#define DRAGON_BAUD 115200u
#define DRAGON_STREAM_SIZE 1024u
#define DRAGON_INPUT_QUEUE_SIZE 12u
#define DRAGON_VIEW_ROWS 3u
#define DRAGON_RX_LINE_CAP 128u

typedef enum {
    DragonScreenMenu,
    DragonScreenConfirm,
    DragonScreenMonitor,
} DragonScreen;

typedef struct {
    Gui* gui;
    ViewPort* view;
    FuriMessageQueue* input_queue;
    FuriStreamBuffer* serial_stream;
    Expansion* expansion;
    FuriHalSerialHandle* serial;
    bool uart_ready;
    bool expansion_owned;
    bool running;
    DragonScreen screen;
    DragonRadio radio;
    DragonAction action;
    char lines[DRAGON_VIEW_ROWS][DRAGON_LINE_CAP];
    char building[DRAGON_RX_LINE_CAP];
    size_t building_len;
    uint32_t received_lines;
    uint32_t received_bytes;
    InspectorLinkFrame link;
    uint32_t last_link_tick;
} DragonApp;

static const char* dragon_action_label(DragonRadio radio, DragonAction action) {
    if(radio == DragonRadioBle) {
        return action == DragonActionRead ? "S3 BLE monitor" : "Bluetooth settings";
    }
    return action == DragonActionRead ? "READ / INSPECT" : "OPEN NATIVE APP";
}

static void dragon_draw(Canvas* canvas, void* context) {
    DragonApp* app = context;
    canvas_clear(canvas);
    canvas_draw_xbm(canvas, 0, 0, 128, 64, inspector_gfx_frame);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 0, 128, 14);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 1, 10, "918 EVIL INSPECTOR");
    canvas_draw_line(canvas, 0, 13, 127, 13);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 15, 100, 49);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);
    if(app->screen == DragonScreenMenu) {
        const DragonRadioInfo* radio = dragon_radio_get(app->radio);
        canvas_draw_str(canvas, 1, 24, radio ? radio->label : "Unknown radio");
        canvas_draw_str(canvas, 1, 35, dragon_action_label(app->radio, app->action));
        canvas_draw_str(canvas, 1, 45, "UP/DN:radio L/R:mode");
        const bool live = app->uart_ready && app->link.valid &&
            ((uint32_t)(furi_get_tick() - app->last_link_tick) < furi_ms_to_ticks(7500U));
        if(live) {
            char status_line[40];
            snprintf(status_line, sizeof(status_line), "S3 LIVE W%u B%u",
                (unsigned)app->link.wifi_saved, (unsigned)app->link.ble_saved);
            canvas_draw_str(canvas, 1, 54, status_line);
        } else {
            canvas_draw_str(canvas, 1, 54, "S3 NO HEARTBEAT / RX");
        }
        canvas_draw_str(canvas, 1, 62, "OK app / HOLD OK serial");
    } else if(app->screen == DragonScreenConfirm) {
        const DragonRadioInfo* radio = dragon_radio_get(app->radio);
        canvas_draw_str(canvas, 1, 25, radio ? radio->label : "Unknown radio");
        canvas_draw_str(canvas, 1, 37, dragon_action_label(app->radio, app->action));
        canvas_draw_str(canvas, 1, 48, "Open native Flipper app?");
        canvas_draw_str(canvas, 1, 61, "OK:confirm   BACK:cancel");
    } else {
        canvas_draw_str(canvas, 1, 22, "S3 UART0 RX / 115200");
        for(size_t i = 0; i < DRAGON_VIEW_ROWS; i++) {
            canvas_draw_str(canvas, 1, (int)(33 + i * 9), app->lines[i]);
        }
        canvas_draw_str(canvas, 1, 62, "BACK:menu");
    }
}

static void dragon_input(InputEvent* event, void* context) {
    DragonApp* app = context;
    if(!app || !app->input_queue || !event) return;
    // The GUI callback is not the application worker; enqueue without blocking.
    furi_message_queue_put(app->input_queue, event, 0);
}

static void dragon_uart_irq(
    FuriHalSerialHandle* handle,
    FuriHalSerialRxEvent event,
    void* context) {
    DragonApp* app = context;
    if(!app || !app->serial_stream) return;
    if(event & FuriHalSerialRxEventData) {
        // Drain all available bytes on one RX event, avoiding truncated telemetry
        // when multiple UART characters were already buffered by the ISR.
        while(furi_hal_serial_async_rx_available(handle)) {
            const uint8_t data = furi_hal_serial_async_rx(handle);
            (void)furi_stream_buffer_send(app->serial_stream, &data, 1, 0);
        }
    }
}

static void dragon_push_line(DragonApp* app) {
    if(app->building_len == 0) return;
    app->building[app->building_len] = '\0';
    InspectorLinkFrame parsed = {0};
    if(inspector_link_parse(app->building, &parsed)) {
        app->link = parsed;
        app->last_link_tick = furi_get_tick();
    }
    for(size_t i = 1; i < DRAGON_VIEW_ROWS; i++) {
        memcpy(app->lines[i - 1], app->lines[i], DRAGON_LINE_CAP);
    }
    dragon_filter_line(app->lines[DRAGON_VIEW_ROWS - 1], DRAGON_LINE_CAP, app->building);
    app->building_len = 0;
    app->received_lines++;
}

static void dragon_process_uart(DragonApp* app) {
    if(!app->serial_stream) return;
    uint8_t data[64];
    // Bound work per frame, even if an external device streams continuously.
    for(size_t batch = 0; batch < 16; batch++) {
        const size_t n = furi_stream_buffer_receive(app->serial_stream, data, sizeof(data), 0);
        if(n == 0) break;
        app->received_bytes += (uint32_t)n;
        for(size_t i = 0; i < n; i++) {
            const unsigned char c = data[i];
            if(c == '\r' || c == '\n') {
                dragon_push_line(app);
                continue;
            }
            if(app->building_len + 1 >= sizeof(app->building)) dragon_push_line(app);
            if(app->building_len + 1 < sizeof(app->building)) {
                app->building[app->building_len++] = (c >= 32 && c <= 126) ? (char)c : ' ';
            }
        }
    }
}

static void dragon_open_uart(DragonApp* app) {
    // Flipper expansion services normally own the GPIO USART. Disable them
    // before acquiring this serial peripheral, and restore on exit.
    app->expansion = furi_record_open(RECORD_EXPANSION);
    if(!app->expansion) return;
    expansion_disable(app->expansion);
    app->expansion_owned = true;
    app->serial = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    if(!app->serial) return;
    furi_hal_serial_init(app->serial, DRAGON_BAUD);
    // The S3 UART0 RX line is also driven by the CH340. Never drive it from
    // the Flipper. This FAP is a one-way RX observer by design.
    furi_hal_serial_disable_direction(app->serial, FuriHalSerialDirectionTx);
    furi_hal_serial_async_rx_start(app->serial, dragon_uart_irq, app, false);
    app->uart_ready = true;
}

static void dragon_close_uart(DragonApp* app) {
    if(app->serial) {
        if(app->uart_ready) furi_hal_serial_async_rx_stop(app->serial);
        furi_hal_serial_deinit(app->serial);
        furi_hal_serial_control_release(app->serial);
        app->serial = NULL;
    }
    app->uart_ready = false;
    if(app->expansion) {
        if(app->expansion_owned) expansion_enable(app->expansion);
        furi_record_close(RECORD_EXPANSION);
        app->expansion = NULL;
        app->expansion_owned = false;
    }
}

static void dragon_launch_native(DragonApp* app) {
    if(dragon_is_monitor_action(app->radio, app->action)) {
        app->screen = DragonScreenMonitor;
        return;
    }
    const char* target = dragon_native_target(app->radio, app->action);
    if(!target) {
        app->screen = DragonScreenMenu;
        return;
    }
    Loader* loader = furi_record_open(RECORD_LOADER);
    if(!loader) {
        app->screen = DragonScreenMenu;
        return;
    }
    // The next app launches only after this FAP exits and releases the UART.
    loader_enqueue_launch(loader, target, NULL, LoaderDeferredLaunchFlagGui);
    furi_record_close(RECORD_LOADER);
    app->running = false;
}

static void dragon_handle_input(DragonApp* app, const InputEvent* event) {
    if(event->key == InputKeyBack && event->type == InputTypeShort) {
        if(app->screen == DragonScreenMenu) app->running = false;
        else app->screen = DragonScreenMenu;
        return;
    }
    if(event->type == InputTypeLong && event->key == InputKeyOk &&
       app->screen == DragonScreenMenu) {
        app->screen = DragonScreenMonitor;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(app->screen == DragonScreenMonitor) return;
    if(app->screen == DragonScreenConfirm) {
        if(event->key == InputKeyOk) dragon_launch_native(app);
        return;
    }
    switch(event->key) {
    case InputKeyUp:
        app->radio = (DragonRadio)((app->radio + DRAGON_RADIO_COUNT - 1) % DRAGON_RADIO_COUNT);
        break;
    case InputKeyDown:
        app->radio = (DragonRadio)((app->radio + 1) % DRAGON_RADIO_COUNT);
        break;
    case InputKeyLeft:
        app->action = DragonActionRead;
        break;
    case InputKeyRight:
        app->action = DragonActionEmulate;
        break;
    case InputKeyOk:
        app->screen = DragonScreenConfirm;
        break;
    default:
        break;
    }
}

int32_t redteam_dragon_main(void* arguments) {
    UNUSED(arguments);
    DragonApp* app = calloc(1, sizeof(DragonApp));
    if(!app) return -1;
    app->running = true;
    app->radio = DragonRadioSubGhz;
    app->action = DragonActionRead;
    app->screen = DragonScreenMenu;
    app->input_queue = furi_message_queue_alloc(DRAGON_INPUT_QUEUE_SIZE, sizeof(InputEvent));
    app->serial_stream = furi_stream_buffer_alloc(DRAGON_STREAM_SIZE, 1);
    if(!app->input_queue || !app->serial_stream) {
        if(app->input_queue) furi_message_queue_free(app->input_queue);
        if(app->serial_stream) furi_stream_buffer_free(app->serial_stream);
        free(app);
        return -1;
    }
    app->gui = furi_record_open(RECORD_GUI);
    app->view = view_port_alloc();
    view_port_draw_callback_set(app->view, dragon_draw, app);
    view_port_input_callback_set(app->view, dragon_input, app);
    gui_add_view_port(app->gui, app->view, GuiLayerFullscreen);
    dragon_open_uart(app);
    view_port_update(app->view);
    while(app->running) {
        InputEvent event;
        if(furi_message_queue_get(app->input_queue, &event, 100) == FuriStatusOk) {
            dragon_handle_input(app, &event);
        }
        dragon_process_uart(app);
        if(app->running) view_port_update(app->view);
    }
    // Never leave a GPIO USART attached when switching to a native radio app.
    dragon_close_uart(app);
    view_port_enabled_set(app->view, false);
    gui_remove_view_port(app->gui, app->view);
    view_port_free(app->view);
    furi_record_close(RECORD_GUI);
    furi_stream_buffer_free(app->serial_stream);
    furi_message_queue_free(app->input_queue);
    free(app);
    return 0;
}
