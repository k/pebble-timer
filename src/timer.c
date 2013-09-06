#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xD7, 0xD7, 0x11, 0x2C, 0x9D, 0xB8, 0x44, 0x63, 0xAF, 0x98, 0xB6, 0x18, 0xB2, 0x20, 0x18, 0x09 }
PBL_APP_INFO(MY_UUID,
             "Timer", "koding",
             1, 0, /* App version */
             0,
             APP_INFO_STANDARD_APP);

#define YES 1
#define NO 0

Window window;

TextLayer textLayer;
Layer layer;
InverterLayer inverter_layer;

AppContextRef context;
AppTimerHandle timer_handle;
AppTimerHandle timer_tick_handle;

#define COOKIE_MAIN_TIMER 1
#define COOKIE_END_TIMER 2

#define TENS_MINUTES_LOC GRect(22, 6, 23, 28)
#define ONES_MINUTES_LOC GRect(45, 6, 23, 28)
#define TENS_SECONDS_LOC GRect(74, 6, 23, 28)
#define ONES_SECONDS_LOC GRect(97, 6, 23, 28)

const char zeros[] = "00:00";
char text_buffer[5];
enum {
    TENS_MINUTES = 0,
    ONES_MINUTES = 1,
    TENS_SECONDS = 3,
    ONES_SECONDS = 4
};
const char *maxValues = "99059";
short current = 0;
short timerStarted = 0;

void highlight_digit(int digit) {
    switch (digit) {
        case 0:
            layer_set_frame(&inverter_layer.layer, TENS_MINUTES_LOC);
            break;
        case 1:
            layer_set_frame(&inverter_layer.layer, ONES_MINUTES_LOC);
            break;
        case 3:
            layer_set_frame(&inverter_layer.layer, TENS_SECONDS_LOC);
            break;
        case 4:
            layer_set_frame(&inverter_layer.layer, ONES_SECONDS_LOC);
            break;
    }
}

void decrement_timer(char *time) {
    if (time[ONES_SECONDS] != '0') {
        time[ONES_SECONDS]--;
    } else if (time[TENS_SECONDS] != '0') {
        time[TENS_SECONDS]--;
        time[ONES_SECONDS] = maxValues[ONES_SECONDS];
    } else if (time[ONES_MINUTES] != '0') {
        time[ONES_MINUTES]--;
        time[TENS_SECONDS] = maxValues[TENS_SECONDS];
        time[ONES_SECONDS] = maxValues[ONES_SECONDS];
    } else if (time[TENS_MINUTES] != '0') {
        time[TENS_MINUTES]--;
        time[ONES_MINUTES] = maxValues[ONES_MINUTES];
        time[TENS_SECONDS] = maxValues[TENS_SECONDS];
        time[ONES_SECONDS] = maxValues[ONES_SECONDS];
    }

}

uint32_t convertTime(char *time) {
    uint32_t ms = 0;
    ms += 60 * 1000 * 10 * (text_buffer[TENS_MINUTES] - '0');
    ms += 60 * 1000 * (text_buffer[ONES_MINUTES] - '0');
    ms += 1000 * 10 * (text_buffer[TENS_SECONDS] - '0');
    ms += 1000 * (text_buffer[ONES_SECONDS] - '0') ;
    return ms;
}

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    if (text_buffer[current] < maxValues[current]) {
        text_buffer[current]++;
    } else {
        text_buffer[current] = '0';
    }
    text_layer_set_text(&textLayer, text_buffer);
}


void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    if (text_buffer[current] != '0') {
        text_buffer[current]--;
    } else {
        text_buffer[current] = maxValues[current];
    }
    text_layer_set_text(&textLayer, text_buffer);
}


void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    // TODO let them pause for god's sake
    if (current < 4) {
        current++;
        if (current == 2) current = 3;
        highlight_digit(current);
    } else if (!timerStarted) {
        layer_set_hidden(&inverter_layer.layer, true);
        timer_handle = app_timer_send_event(context, convertTime(text_buffer), COOKIE_MAIN_TIMER);
        timer_tick_handle = app_timer_send_event(context, 1000, 0);
        timerStarted = YES;
    } else if (timerStarted) {
        layer_set_hidden(&inverter_layer.layer, false);
        app_timer_cancel_event(context, timer_handle);
        strcpy(text_buffer, zeros);
        text_layer_set_text(&textLayer, text_buffer);
        timerStarted = NO;
        current = 0;
        highlight_digit(current);
    }
}


void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
    // reset on long click
    if (timerStarted) {
        strcpy(text_buffer, "00:00");
        text_layer_set_text(&textLayer, text_buffer);
        app_timer_cancel_event(context, timer_handle);
        app_timer_cancel_event(context, timer_tick_handle);
    }
}

void update_timer_background(Layer *me, GContext* ctx) {
// Redraw the background to match the percentage of original time left
}

void click_config_provider(ClickConfig **config, Window *window) {

  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;

  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}

void handle_init(AppContextRef ctx) {

  context = ctx;
  window_init(&window, "Timer");
  window_stack_push(&window, true /* Animated */);

  layer_init(&layer, window.layer.frame);
  layer.update_proc = &update_timer_background;
  layer_add_child(&window.layer, &layer);

  strcpy(text_buffer, zeros);
  text_layer_init(&textLayer, window.layer.frame);
  GRect mainFrame = layer_get_frame(&layer);
  layer_set_frame(&textLayer.layer, GRect(0, mainFrame.size.h/2 - 50, mainFrame.size.w, 50));
  text_layer_set_text(&textLayer, text_buffer);
  text_layer_set_font(&textLayer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(&textLayer, GTextAlignmentCenter);
  layer_add_child(&window.layer, &textLayer.layer);

  inverter_layer_init(&inverter_layer, TENS_MINUTES_LOC);
  layer_add_child(&textLayer.layer, &inverter_layer.layer);

  window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);

}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {

    if (cookie == COOKIE_MAIN_TIMER) {
        app_timer_cancel_event(ctx, timer_tick_handle);
        text_layer_set_text(&textLayer, "Ding!");
        // This where we vibrate/make a sound that the timer is complete
        vibes_double_pulse();

        timer_handle = app_timer_send_event(ctx, 2000, COOKIE_END_TIMER);
        timerStarted = NO;

    } else if (cookie == COOKIE_END_TIMER) {
        strcpy(text_buffer, zeros);
        text_layer_set_text(&textLayer, text_buffer);
        current = 0;
        layer_set_hidden(&inverter_layer.layer, false);
        highlight_digit(current);
    } else {
        decrement_timer(text_buffer);
        text_layer_set_text(&textLayer, text_buffer);
        timer_tick_handle = app_timer_send_event(ctx, 1000, 0);
    }
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .timer_handler = &handle_timer
  };
  app_event_loop(params, &handlers);
}
