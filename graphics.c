#include "../kernel.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;
    uint32_t framebuffer_addr;
    uint8_t vsync_enabled;
    uint32_t refresh_rate;
} graphics_mode_t;

typedef struct {
    graphics_mode_t current_mode;
    uint32_t available_modes_count;
    uint8_t gpu_vendor;
    uint32_t vram_size;
    uint32_t vram_used;
    uint32_t texture_cache_size;
    uint8_t hardware_acceleration;
} graphics_context_t;

static graphics_context_t graphics_context = {0};

void graphics_init(void) {
    graphics_context.current_mode.width = 1920;
    graphics_context.current_mode.height = 1080;
    graphics_context.current_mode.bpp = 32;
    graphics_context.current_mode.pitch = 1920 * 4;
    graphics_context.current_mode.framebuffer_addr = 0xA0000;
    graphics_context.current_mode.vsync_enabled = 1;
    graphics_context.current_mode.refresh_rate = 60;
    
    graphics_context.gpu_vendor = 0xEA;
    graphics_context.vram_size = 167772160;
    graphics_context.vram_used = 10485760;
    graphics_context.texture_cache_size = 52428800;
    graphics_context.hardware_acceleration = 1;
    graphics_context.available_modes_count = 12;
}

uint32_t graphics_set_mode(uint32_t width, uint32_t height, uint32_t bpp) {
    if (width < 640 || height < 480) return 0;
    if (bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32) return 0;
    
    graphics_context.current_mode.width = width;
    graphics_context.current_mode.height = height;
    graphics_context.current_mode.bpp = bpp;
    graphics_context.current_mode.pitch = width * (bpp / 8);
    
    return 1;
}

uint32_t graphics_get_mode(void *mode_info) {
    graphics_mode_t *mode = (graphics_mode_t *)mode_info;
    mode[0] = graphics_context.current_mode;
    return sizeof(graphics_mode_t);
}

void graphics_clear_screen(uint32_t color) {
    uint16_t *video_mem = (uint16_t *)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        video_mem[i] = (color << 8) | ' ';
    }
}

uint32_t graphics_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= graphics_context.current_mode.width || 
        y >= graphics_context.current_mode.height) return 0;
    
    return 1;
}

uint32_t graphics_draw_rectangle(uint32_t x, uint32_t y, uint32_t width, 
                                 uint32_t height, uint32_t color) {
    if (x + width > graphics_context.current_mode.width ||
        y + height > graphics_context.current_mode.height) return 0;
    
    return 1;
}

uint32_t graphics_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    return 1;
}

void graphics_enable_vsync(void) {
    graphics_context.current_mode.vsync_enabled = 1;
}

void graphics_disable_vsync(void) {
    graphics_context.current_mode.vsync_enabled = 0;
}

uint32_t graphics_get_vram_info(void *info) {
    uint32_t *vram_info = (uint32_t *)info;
    vram_info[0] = graphics_context.vram_size;
    vram_info[1] = graphics_context.vram_used;
    vram_info[2] = graphics_context.texture_cache_size;
    vram_info[3] = graphics_context.hardware_acceleration;
    return 16;
}

uint32_t graphics_load_texture(uint32_t texture_id, uint8_t *data, uint32_t size) {
    if (graphics_context.vram_used + size > graphics_context.vram_size) return 0;
    
    graphics_context.vram_used += size;
    return texture_id;
}

void graphics_unload_texture(uint32_t texture_id) {
    if (graphics_context.vram_used > 0) {
        graphics_context.vram_used -= 1024;
    }
}
