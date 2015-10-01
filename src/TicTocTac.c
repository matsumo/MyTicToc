#include <pebble.h>

#define COLORS       true
#define ANTIALIASING true

#define HAND_MARGIN  12
#define FINAL_RADIUS 70
#define STROKE_WIDTH 8
#define STROKE_WIDTH_SLIM 2
  
#define ANIMATION_DURATION 500
#define ANIMATION_DELAY    600

typedef struct {
    int hours;
    int minutes;
} Time;

static Window *s_main_window;
static GRect window_bounds;
static Layer *s_canvas_layer, *s_date_layer;
static TextLayer *s_num_label;

static GPoint s_center, s_center_top;
static Time s_last_time, s_anim_time;
static bool s_animating = false;
static int s_radius = 0, s_color_channels[3];

static bool s_show_hour_markers = false;

/*************************** AnimationImplementation **************************/

static void animation_started(Animation *anim, void *context) {
  s_animating = true;
}

static void animation_stopped(Animation *anim, bool stopped, void *context) {
  s_animating = false;
}

static void animate(int duration, int delay, AnimationImplementation *implementation, bool handlers) {
    Animation *anim = animation_create();
    animation_set_duration(anim, duration);
    animation_set_delay(anim, delay);
    animation_set_curve(anim, AnimationCurveEaseInOut);
    animation_set_implementation(anim, implementation);
    
    if(handlers) {
        animation_set_handlers(anim, (AnimationHandlers) {
            .started = animation_started,
            .stopped = animation_stopped
        }, NULL);
    }
    
    animation_schedule(anim);
}

/************************************ UI **************************************/

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
    // Store time
    s_last_time.hours = tick_time->tm_hour;
    s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
    s_last_time.minutes = tick_time->tm_min;

    for(int i = 0; i < 3; i++) {
        s_color_channels[i] = rand() % 256;
    }

    // Redraw
    if(s_canvas_layer) {
        layer_mark_dirty(s_canvas_layer);
    }
}

static int hours_to_minutes(int hours_out_of_12) {
    return (int)(float)(((float)hours_out_of_12 / 12.0F) * 60.0F);
}

static void update_proc(Layer *layer, GContext *ctx) {
    // Color background?
    //if(COLORS) {
    //    graphics_context_set_fill_color(ctx, GColorFromRGB(s_color_channels[0], s_color_channels[1], s_color_channels[2]));
    //    graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);
    //}

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, STROKE_WIDTH);
    graphics_context_set_antialiased(ctx, ANTIALIASING);

    // White clockface
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);
    //graphics_fill_circle(ctx, s_center, s_radius);

    // Draw outline
    //graphics_draw_circle(ctx, s_center, s_radius);

    // Don't use current time while animating
    Time mode_time = (s_animating) ? s_anim_time : s_last_time;

    // Adjust for minutes through the hour
    float minute_angle = TRIG_MAX_ANGLE * mode_time.minutes / 60;
    float hour_angle;
    
    if(s_animating) {
        // Hours out of 60 for smoothness
        hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 60;
    } else {
        hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 12;
    }
    
    hour_angle += (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);

    // Plot hands
    GPoint minute_hand = (GPoint) {
        .x = (int16_t)( sin_lookup(TRIG_MAX_ANGLE * mode_time.minutes / 60) * (int32_t)(s_radius - HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.x,
        .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * mode_time.minutes / 60) * (int32_t)(s_radius - HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.y,
    };
    GPoint hour_hand = (GPoint) {
        .x = (int16_t)( sin_lookup(hour_angle) * (int32_t)(s_radius - ((2 * HAND_MARGIN) + 6)) / TRIG_MAX_RATIO) + s_center.x,
        .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(s_radius - ((2 * HAND_MARGIN) + 6)) / TRIG_MAX_RATIO) + s_center.y,
    };
    
    // Draw hands with positive length only
    if(s_radius > HAND_MARGIN) {
        graphics_draw_line(ctx, s_center, minute_hand);
    }

    //Top hour marker
    graphics_draw_line(ctx, s_center_top, s_center_top);
    
    //Hour hand
    graphics_context_set_stroke_color(ctx, GColorFromRGB(2, 0xa7, 0x80)/*GColorRed*/);
    if(s_radius > 2 * HAND_MARGIN) {
        graphics_draw_line(ctx, s_center, hour_hand);
    }
    
    //Hour markers 
    if(s_show_hour_markers) {
		graphics_context_set_stroke_color(ctx, GColorWhite);
		graphics_context_set_stroke_width(ctx, STROKE_WIDTH_SLIM);
		
		for(int h=0; h<12; h++) {
			if(h != 0 && h != 6) {
				float hour_marker_angle = TRIG_MAX_ANGLE * h / 12;
				
				//APP_LOG(APP_LOG_LEVEL_DEBUG, "Width: %d", (window_bounds.size.w / 2) + 40);
				
				GPoint start_hour_marker = (GPoint) {
					.x = (int16_t)( sin_lookup(hour_marker_angle) * (int32_t)(s_radius - (window_bounds.size.w) + 2) / TRIG_MAX_RATIO) + s_center.x,
					.y = (int16_t)(-cos_lookup(hour_marker_angle) * (int32_t)(s_radius - (window_bounds.size.w) + 2) / TRIG_MAX_RATIO) + s_center.y,
				}; 
				
				GPoint end_hour_marker = (GPoint) {
					.x = (int16_t)( sin_lookup(hour_marker_angle) * (int32_t)(s_radius - (window_bounds.size.w) + 7) / TRIG_MAX_RATIO) + s_center.x,
					.y = (int16_t)(-cos_lookup(hour_marker_angle) * (int32_t)(s_radius - (window_bounds.size.w) + 7) / TRIG_MAX_RATIO) + s_center.y,
				}; 
				
				graphics_draw_line(ctx, start_hour_marker, end_hour_marker);
			}
		}
	}
}

static void battery_handler(BatteryChargeState new_state) {
    // Write to buffer and display
    static char s_battery_buffer[6];
    snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", new_state.charge_percent);
    text_layer_set_text(s_num_label, s_battery_buffer);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    window_bounds = layer_get_bounds(window_layer);

    s_center = grect_center_point(&window_bounds);
    s_center_top = (GPoint) {
        .x = s_center.x,
        .y = s_center.y - (FINAL_RADIUS + 5),
    };
        
    s_canvas_layer = layer_create(window_bounds);
    layer_set_update_proc(s_canvas_layer, update_proc);
    layer_add_child(window_layer, s_canvas_layer);
    
    //Date layer
    s_date_layer = layer_create(window_bounds);
    layer_add_child(window_layer, s_date_layer);
    
    int date_text_height = 20;
    
    s_num_label = text_layer_create(GRect(0, window_bounds.size.h - date_text_height, window_bounds.size.w, date_text_height));
    text_layer_set_text_alignment(s_num_label, GTextAlignmentCenter);
    text_layer_set_background_color(s_num_label, GColorClear);
    text_layer_set_text_color(s_num_label, GColorWhite);
    text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    
    layer_add_child(window_layer, text_layer_get_layer(s_num_label));

    // Get the current battery level
    battery_handler(battery_state_service_peek());
}

static void window_unload(Window *window) {
    layer_destroy(s_canvas_layer);
}

/*********************************** App **************************************/

static int anim_percentage(AnimationProgress dist_normalized, int max) {
    return (int)(float)(((float)dist_normalized / (float)ANIMATION_NORMALIZED_MAX) * (float)max);
}

static void radius_update(Animation *anim, AnimationProgress dist_normalized) {
    s_radius = anim_percentage(dist_normalized, FINAL_RADIUS);

    layer_mark_dirty(s_canvas_layer);
}

static void hands_update(Animation *anim, AnimationProgress dist_normalized) {
    s_anim_time.hours = anim_percentage(dist_normalized, hours_to_minutes(s_last_time.hours));
    s_anim_time.minutes = anim_percentage(dist_normalized, s_last_time.minutes);

    layer_mark_dirty(s_canvas_layer);
}

/*********************************** Config **************************************/

static void init() {
    srand(time(NULL));

    time_t t = time(NULL);
    struct tm *time_now = localtime(&t);
    tick_handler(time_now, MINUTE_UNIT);

    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    // Prepare animations
    AnimationImplementation radius_impl = {
        .update = radius_update
    };
    animate(ANIMATION_DURATION, ANIMATION_DELAY, &radius_impl, false);

    AnimationImplementation hands_impl = {
        .update = hands_update
    };
    animate(2 * ANIMATION_DURATION, ANIMATION_DELAY, &hands_impl, true);

    // Subscribe to the Battery State Service
    battery_state_service_subscribe(battery_handler);
}

static void deinit() {
    window_destroy(s_main_window);
}

int main() {
    init();
    app_event_loop();
    deinit();
}
