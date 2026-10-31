#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

typedef struct { FuriMutex* lock; bool running; uint32_t frames; } Gadget;

static void draw(Canvas* canvas, void* context) {
    Gadget* g = context;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 0, 12, "Go Go Gadget");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 0, 28, "918 Technologies");
    char line[32]; snprintf(line, sizeof(line), "UART 115200  RX %lu", (unsigned long)g->frames);
    canvas_draw_str(canvas, 0, 43, line);
    canvas_draw_str(canvas, 0, 62, "OK: Back exits");
}

static void input(InputEvent* event, void* context) {
    Gadget* g = context;
    if(event->type == InputTypePress && event->key == InputKeyBack) g->running = false;
}

int32_t go_gadget_app(void* p) {
    UNUSED(p);
    Gadget g = {.lock = furi_mutex_alloc(FuriMutexTypeNormal), .running = true, .frames = 0};
    Gui* gui = furi_record_open(RECORD_GUI);
    ViewPort* view = view_port_alloc();
    view_port_draw_callback_set(view, draw, &g);
    view_port_input_callback_set(view, input, &g);
    gui_add_view_port(gui, view, GuiLayerFullscreen);
    while(g.running) { furi_delay_ms(100); }
    gui_remove_view_port(gui, view);
    view_port_free(view);
    furi_record_close(RECORD_GUI);
    furi_mutex_free(g.lock);
    return 0;
}
