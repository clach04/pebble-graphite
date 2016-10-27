// Copyright 2016 Stefan Heule
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "redshift.h"


////////////////////////////////////////////
//// Default values for the configuration
////////////////////////////////////////////

// defaults are also in src/redshift.c, src/js/pebble-js-app.js and config/js/preview.js
uint8_t config_color_outer_background = COLOR_FALLBACK(GColorDarkGrayARGB8, GColorBlackARGB8);
uint8_t config_color_inner_background = COLOR_FALLBACK(GColorWhiteARGB8, GColorWhiteARGB8);
uint8_t config_color_minute_hand = COLOR_FALLBACK(GColorBlackARGB8, GColorBlackARGB8);
uint8_t config_color_inner_minute_hand = COLOR_FALLBACK(GColorLightGrayARGB8, GColorBlackARGB8);
uint8_t config_color_hour_hand = COLOR_FALLBACK(GColorJaegerGreenARGB8, GColorBlackARGB8);
uint8_t config_color_inner_hour_hand = COLOR_FALLBACK(GColorLightGrayARGB8, GColorBlackARGB8);
uint8_t config_color_circle = COLOR_FALLBACK(GColorBlackARGB8, GColorBlackARGB8);
uint8_t config_color_ticks = COLOR_FALLBACK(GColorBlackARGB8, GColorBlackARGB8);
uint8_t config_color_day_of_week = COLOR_FALLBACK(GColorJaegerGreenARGB8, GColorBlackARGB8);
uint8_t config_color_date = COLOR_FALLBACK(GColorBlackARGB8, GColorBlackARGB8);
uint8_t config_battery_logo = 1;
uint8_t config_color_battery_logo = COLOR_FALLBACK(PBL_IF_ROUND_ELSE(GColorDarkGrayARGB8, GColorBlackARGB8),
                                                   GColorWhiteARGB8);
uint8_t config_color_battery_30 = COLOR_FALLBACK(PBL_IF_ROUND_ELSE(GColorYellowARGB8, GColorBlackARGB8),
                                                 GColorWhiteARGB8);
uint8_t config_color_battery_20 = COLOR_FALLBACK(PBL_IF_ROUND_ELSE(GColorOrangeARGB8, GColorBlackARGB8),
                                                 GColorWhiteARGB8);
uint8_t config_color_battery_10 = COLOR_FALLBACK(PBL_IF_ROUND_ELSE(GColorRedARGB8, GColorBlackARGB8),
                                                 GColorWhiteARGB8);
uint8_t config_color_battery_bg_30 = COLOR_FALLBACK(PBL_IF_ROUND_ELSE(GColorWhiteARGB8, GColorYellowARGB8),
                                                    GColorBlackARGB8);
uint8_t config_color_battery_bg_20 = COLOR_FALLBACK(PBL_IF_ROUND_ELSE(GColorWhiteARGB8, GColorOrangeARGB8),
                                                    GColorBlackARGB8);
uint8_t config_color_battery_bg_10 = COLOR_FALLBACK(PBL_IF_ROUND_ELSE(GColorWhiteARGB8, GColorRedARGB8),
                                                    GColorBlackARGB8);
uint8_t config_color_bluetooth_logo = COLOR_FALLBACK(GColorWhiteARGB8, GColorBlackARGB8);
uint8_t config_color_bluetooth_logo_2 = COLOR_FALLBACK(GColorBlackARGB8, GColorWhiteARGB8);
uint8_t config_bluetooth_logo = true;
uint8_t config_vibrate_disconnect = true;
uint8_t config_vibrate_reconnect = true;
uint8_t config_message_disconnect = true;
uint8_t config_message_reconnect = true;
uint8_t config_minute_ticks = 1;
uint8_t config_hour_ticks = 1;
uint8_t config_color_weather = COLOR_FALLBACK(GColorBlackARGB8, GColorBlackARGB8);
uint16_t config_weather_refresh = 30;
uint16_t config_weather_expiration = 3*60;
uint8_t config_square = false;
uint8_t config_seconds = 0;
uint8_t config_color_seconds = COLOR_FALLBACK(GColorJaegerGreenARGB8, GColorBlackARGB8);
uint8_t config_date_format = 0;


////////////////////////////////////////////
//// Global variables
////////////////////////////////////////////

/** A pointer to our window, for later deallocation. */
Window *window;

/** All layers */
Layer *layer_background;

/** Buffers for strings */
char buffer_1[30];
char buffer_2[30];

/** The height and width of the watch */
fixed_t height;
fixed_t width;

/** Fonts. */
FFont* font_main;
FFont* font_weather;
FFont* font_icon;

/** Is the bluetooth popup current supposed to be shown? */
bool show_bluetooth_popup;

/** The timer for the bluetooth popup */
AppTimer *timer_bluetooth_popup;

/** The current weather information. */
Weather weather;

/** Is the JS runtime ready? */
bool js_ready;

/** A timer used to schedule weather updates. */
AppTimer * weather_request_timer;



////////////////////////////////////////////
//// Implementation
////////////////////////////////////////////

/**
 * Handler for time ticks.
 */
void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(layer_background);
}

void timer_callback_bluetooth_popup(void *data) {
    show_bluetooth_popup = false;
    timer_bluetooth_popup = NULL;
    layer_mark_dirty(layer_background);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bluetooth change callback");
}

void handle_bluetooth(bool connected) {
    // redraw background (to turn on/off the logo)
    layer_mark_dirty(layer_background);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bluetooth change");

    bool show_popup = false;
    bool vibrate = false;
    if ((config_message_reconnect && connected) || (config_message_disconnect && !connected)) {
        show_popup = true;
    }
    if ((config_vibrate_reconnect && connected) || (config_vibrate_disconnect && !connected)) {
        vibrate = true;
    }

    // vibrate
    if (vibrate) {
        vibes_double_pulse();
    }

    // turn light on
    if (show_popup) {
        light_enable_interaction();
    }

    // show popup
    if (show_popup) {
        show_bluetooth_popup = true;
        if (timer_bluetooth_popup) {
            app_timer_reschedule(timer_bluetooth_popup, REDSHIFT_BLUETOOTH_POPUP_MS);
        } else {
            timer_bluetooth_popup = app_timer_register(REDSHIFT_BLUETOOTH_POPUP_MS, timer_callback_bluetooth_popup,
                                                       NULL);
        }
    }
}

/**
 * Window load callback.
 */
void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // create layer
    layer_background = layer_create(bounds);
    layer_set_update_proc(layer_background, background_update_proc);
    layer_add_child(window_layer, layer_background);

    // load fonts
    font_main = ffont_create_from_resource(RESOURCE_ID_MAIN_FFONT);
    font_weather = ffont_create_from_resource(RESOURCE_ID_WEATHER_FFONT);
    font_icon = ffont_create_from_resource(RESOURCE_ID_ICON_FFONT);

    // initialize
    show_bluetooth_popup = false;
}

/**
 * Window unload callback.
 */
void window_unload(Window *window) {
    layer_destroy(layer_background);
    ffont_destroy(font_main);
    ffont_destroy(font_weather);
    ffont_destroy(font_icon);
}

void subscribe_tick(bool also_unsubscribe) {
    if (also_unsubscribe) {
        tick_timer_service_unsubscribe();
    }
    TimeUnits unit = MINUTE_UNIT;
    tick_timer_service_subscribe(unit, handle_second_tick);
}

/**
 * Initialization.
 */
void init() {
    read_config_all();

    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
    });
    window_stack_push(window, true);

    subscribe_tick(false);
    bluetooth_connection_service_subscribe(handle_bluetooth);

    app_message_open(REDSHIFT_INBOX_SIZE, REDSHIFT_OUTBOX_SIZE);
    app_message_register_inbox_received(inbox_received_handler);
}

/**
 * De-initialisation.
 */
void deinit() {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();

    window_destroy(window);

    app_message_deregister_callbacks();
}

/**
 * Main entry point.
 */
int main() {
    init();
    app_event_loop();
    deinit();
}
