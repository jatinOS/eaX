#include "../kernel.h"

typedef struct {
    uint8_t is_connected;
    uint8_t key_state[256];
    uint8_t key_pressed;
    uint8_t key_released;
} keyboard_device_t;

typedef struct {
    uint8_t is_connected;
    uint32_t x_position;
    uint32_t y_position;
    uint8_t button_state;
    int32_t wheel_delta;
    uint8_t acceleration;
} mouse_device_t;

typedef struct {
    keyboard_device_t keyboard;
    mouse_device_t mouse;
    uint32_t input_events_queued;
    uint32_t total_events_processed;
} input_manager_t;

static input_manager_t input_manager = {0};

void input_init(void) {
    input_manager.keyboard.is_connected = 1;
    input_manager.keyboard.key_pressed = 0;
    input_manager.keyboard.key_released = 0;
    
    input_manager.mouse.is_connected = 1;
    input_manager.mouse.x_position = 960;
    input_manager.mouse.y_position = 540;
    input_manager.mouse.button_state = 0;
    input_manager.mouse.acceleration = 1;
    input_manager.mouse.wheel_delta = 0;
    
    input_manager.input_events_queued = 0;
    input_manager.total_events_processed = 0;
}

void keyboard_set_key(uint8_t keycode, uint8_t is_pressed) {
    if (is_pressed) {
        input_manager.keyboard.key_state[keycode] = 1;
        input_manager.keyboard.key_pressed = keycode;
    } else {
        input_manager.keyboard.key_state[keycode] = 0;
        input_manager.keyboard.key_released = keycode;
    }
    input_manager.input_events_queued++;
    input_manager.total_events_processed++;
}

uint8_t keyboard_get_key(uint8_t keycode) {
    return input_manager.keyboard.key_state[keycode];
}

void mouse_set_position(uint32_t x, uint32_t y) {
    input_manager.mouse.x_position = x;
    input_manager.mouse.y_position = y;
    input_manager.input_events_queued++;
}

void mouse_get_position(uint32_t *x, uint32_t *y) {
    *x = input_manager.mouse.x_position;
    *y = input_manager.mouse.y_position;
}

void mouse_set_button(uint8_t button, uint8_t is_pressed) {
    if (is_pressed) {
        input_manager.mouse.button_state |= (1 << button);
    } else {
        input_manager.mouse.button_state &= ~(1 << button);
    }
    input_manager.input_events_queued++;
    input_manager.total_events_processed++;
}

uint8_t mouse_get_button(uint8_t button) {
    return (input_manager.mouse.button_state >> button) & 1;
}

void mouse_set_wheel(int32_t delta) {
    input_manager.mouse.wheel_delta = delta;
    input_manager.input_events_queued++;
}

void mouse_set_acceleration(uint8_t level) {
    if (level > 0 && level <= 10) {
        input_manager.mouse.acceleration = level;
    }
}

uint32_t input_get_event(void *event_buffer) {
    if (input_manager.input_events_queued == 0) return 0;
    
    input_manager.input_events_queued--;
    return 1;
}

void input_clear_queue(void) {
    input_manager.input_events_queued = 0;
}

uint32_t input_get_stats(void *stats_buffer) {
    uint32_t *stats = (uint32_t *)stats_buffer;
    stats[0] = input_manager.total_events_processed;
    stats[1] = input_manager.keyboard.is_connected;
    stats[2] = input_manager.mouse.is_connected;
    return 12;
}
