#ifndef _DRIVERS_H
#define _DRIVERS_H

#include <stdint.h>
#include <stddef.h>

void storage_init(void);
void network_init(void);
void graphics_init(void);
void input_init(void);

uint32_t storage_read_sector(uint32_t drive, uint64_t sector, uint8_t *buffer);
uint32_t storage_write_sector(uint32_t drive, uint64_t sector, uint8_t *buffer);
uint32_t storage_get_drive_info(uint32_t drive, void *info);
void storage_enable_drive(uint32_t drive);
void storage_disable_drive(uint32_t drive);

uint32_t network_send_packet(uint8_t *buffer, uint32_t length);
uint32_t network_receive_packet(uint8_t *buffer, uint32_t max_length);
uint32_t network_get_interface_info(uint32_t interface, void *info);
void network_set_interface_ip(uint32_t interface, uint32_t ipv4);
void network_enable_interface(uint32_t interface);
void network_disable_interface(uint32_t interface);
uint32_t network_get_stats(void *stats_buffer);

uint32_t graphics_set_mode(uint32_t width, uint32_t height, uint32_t bpp);
uint32_t graphics_get_mode(void *mode_info);
void graphics_clear_screen(uint32_t color);
uint32_t graphics_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t graphics_draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
uint32_t graphics_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void graphics_enable_vsync(void);
void graphics_disable_vsync(void);
uint32_t graphics_get_vram_info(void *info);
uint32_t graphics_load_texture(uint32_t texture_id, uint8_t *data, uint32_t size);
void graphics_unload_texture(uint32_t texture_id);

void keyboard_set_key(uint8_t keycode, uint8_t is_pressed);
uint8_t keyboard_get_key(uint8_t keycode);
void mouse_set_position(uint32_t x, uint32_t y);
void mouse_get_position(uint32_t *x, uint32_t *y);
void mouse_set_button(uint8_t button, uint8_t is_pressed);
uint8_t mouse_get_button(uint8_t button);
void mouse_set_wheel(int32_t delta);
void mouse_set_acceleration(uint8_t level);
uint32_t input_get_event(void *event_buffer);
void input_clear_queue(void);
uint32_t input_get_stats(void *stats_buffer);

#endif
