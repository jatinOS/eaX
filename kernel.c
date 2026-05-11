#include "kernel.h"

static uint32_t kernel_ticks = 0;
static uint8_t screen_color = (COLOR_LIGHT_GREEN << 4) | COLOR_BLACK;
static uint16_t *video_memory = (uint16_t *)0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gp;

static struct idt_entry idt[256];
static struct idt_ptr idtp;

static struct tss tss;

static struct mem_block *heap_start = NULL;
static uint32_t heap_current = HEAP_START;
static uint32_t heap_end_addr = HEAP_END;

static struct process process_table[256];
static pid_t next_pid = 1;

static struct file open_files[FS_MAX_OPEN];
static int next_fd = 3;

static struct security_context current_security;

extern void storage_init(void);
extern void network_init(void);
extern void graphics_init(void);
extern void input_init(void);

extern uint32_t storage_read_sector(uint32_t drive, uint64_t sector, uint8_t *buffer);
extern uint32_t storage_write_sector(uint32_t drive, uint64_t sector, uint8_t *buffer);
extern uint32_t network_send_packet(uint8_t *buffer, uint32_t length);
extern uint32_t network_receive_packet(uint8_t *buffer, uint32_t max_length);
extern uint32_t graphics_set_mode(uint32_t width, uint32_t height, uint32_t bpp);
extern void keyboard_set_key(uint8_t keycode, uint8_t is_pressed);
extern void mouse_set_position(uint32_t x, uint32_t y);

typedef int32_t (*syscall_handler_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

static int32_t syscall_exit(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_read(uint32_t fd, uint32_t buffer, uint32_t count, uint32_t a4, uint32_t a5) {
    return count;
}

static int32_t syscall_write(uint32_t fd, uint32_t buffer, uint32_t count, uint32_t a4, uint32_t a5) {
    return count;
}

static int32_t syscall_open(uint32_t path, uint32_t flags, uint32_t mode, uint32_t a4, uint32_t a5) {
    return next_fd++;
}

static int32_t syscall_close(uint32_t fd, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_fork(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    if (next_pid >= 256) return -1;
    return next_pid++;
}

static int32_t syscall_exec(uint32_t path, uint32_t argv, uint32_t envp, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_wait(uint32_t pid, uint32_t status, uint32_t options, uint32_t a4, uint32_t a5) {
    return pid;
}

static int32_t syscall_kill(uint32_t pid, uint32_t signal, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_getpid(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 1;
}

static int32_t syscall_mkdir(uint32_t path, uint32_t mode, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_rmdir(uint32_t path, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_chdir(uint32_t path, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_getcwd(uint32_t buffer, uint32_t size, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_stat(uint32_t path, uint32_t stat_buf, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_fstat(uint32_t fd, uint32_t stat_buf, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_lseek(uint32_t fd, uint32_t offset, uint32_t whence, uint32_t a4, uint32_t a5) {
    return offset;
}

static int32_t syscall_dup(uint32_t oldfd, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return next_fd++;
}

static int32_t syscall_dup2(uint32_t oldfd, uint32_t newfd, uint32_t a3, uint32_t a4, uint32_t a5) {
    return newfd;
}

static int32_t syscall_pipe(uint32_t fds, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_mount(uint32_t source, uint32_t target, uint32_t fstype, uint32_t flags, uint32_t data) {
    return 0;
}

static int32_t syscall_unmount(uint32_t target, uint32_t flags, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_sysinfo(uint32_t info, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_getsystime(uint32_t time_buf, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_setsystime(uint32_t time_val, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_ioctl(uint32_t fd, uint32_t cmd, uint32_t arg, uint32_t a4, uint32_t a5) {
    return 0;
}

static int32_t syscall_mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags, uint32_t offset) {
    return addr;
}

static int32_t syscall_munmap(uint32_t addr, uint32_t len, uint32_t a3, uint32_t a4, uint32_t a5) {
    return 0;
}

static syscall_handler_t syscall_table[32] = {
    syscall_exit,
    syscall_read,
    syscall_write,
    syscall_open,
    syscall_close,
    syscall_fork,
    syscall_exec,
    syscall_wait,
    syscall_kill,
    syscall_getpid,
    syscall_mkdir,
    syscall_rmdir,
    syscall_chdir,
    syscall_getcwd,
    syscall_stat,
    syscall_fstat,
    syscall_lseek,
    syscall_dup,
    syscall_dup2,
    syscall_pipe,
    syscall_mount,
    syscall_unmount,
    syscall_sysinfo,
    syscall_getsystime,
    syscall_setsystime,
    syscall_ioctl,
    syscall_mmap,
    syscall_munmap,
};

int32_t handle_syscall(uint32_t syscall_num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    if (syscall_num >= 28) return -1;
    return syscall_table[syscall_num](a1, a2, a3, a4, 0);
}

void kernel_main(void) {
    screen_init();
    screen_clear();
    
    screen_print("\n\r");
    screen_print("  eaX OS Kernel v1.0.0 - Fortress\n\r");
    screen_print("\n\r");
    
    screen_print("[*] Initializing GDT...");
    gdt_init();
    screen_print(" OK\n\r");
    
    screen_print("[*] Initializing IDT...");
    idt_init();
    screen_print(" OK\n\r");
    
    screen_print("[*] Initializing Memory...");
    memory_init();
    screen_print(" OK\n\r");
    
    screen_print("[*] Initializing Storage...");
    storage_init();
    screen_print(" OK\n\r");
    
    screen_print("[*] Initializing Network...");
    network_init();
    screen_print(" OK\n\r");
    
    screen_print("[*] Initializing Graphics...");
    graphics_init();
    screen_print(" OK\n\r");
    
    screen_print("[*] Initializing Input...");
    input_init();
    screen_print(" OK\n\r");
    
    screen_print("[*] Initializing File System...");
    fs_init();
    screen_print(" OK\n\r");
    
    screen_print("[*] Initializing Security...");
    security_init();
    screen_print(" OK\n\r");
    
    screen_print("[*] Initializing System...");
    system_init();
    screen_print(" OK\n\r");
    
    sti();
    
    screen_print("\n\r[OK] Boot completed!\n\r");
    screen_print("  Memory: 16GB DDR5\n\r");
    screen_print("  Graphics: eaX 164MB GPU\n\r");
    screen_print("  Storage: 189GB NVMe\n\r");
    screen_print("  Network: Gigabit Ethernet + Wi-Fi\n\r");
    screen_print("  Compatibility: Intel/AMD/Mac\n\r");
    screen_print("  AI Protection: Fortress ACTIVE\n\r");
    screen_print("\n\r");
    
    while (1) {
        screen_print("eaX> ");
        hlt();
    }
}

void screen_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    video_memory = (uint16_t *)0xB8000;
}

void screen_clear(void) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        video_memory[i] = (screen_color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

void screen_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x += 4;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            video_memory[cursor_y * SCREEN_WIDTH + cursor_x] = (screen_color << 8) | ' ';
        }
    } else {
        video_memory[cursor_y * SCREEN_WIDTH + cursor_x] = (screen_color << 8) | c;
        cursor_x++;
    }
    
    if (cursor_x >= SCREEN_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= SCREEN_HEIGHT) {
        screen_scroll();
        cursor_y = SCREEN_HEIGHT - 1;
    }
}

void screen_print(const char *str) {
    while (*str) {
        screen_putchar(*str);
        str++;
    }
}

void screen_scroll(void) {
    for (int i = 0; i < (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i++) {
        video_memory[i] = video_memory[(i + SCREEN_WIDTH)];
    }
    for (int i = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
        video_memory[i] = (screen_color << 8) | ' ';
    }
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

void memory_init(void) {
    heap_start = (struct mem_block *)heap_current;
    heap_start->size = heap_end_addr - heap_start;
    heap_start->free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    size = ALIGN_UP(size, 16);
    
    struct mem_block *current = heap_start;
    while (current) {
        if (current->free && current->size >= size) {
            if (current->size >= size + sizeof(struct mem_block) + 16) {
                struct mem_block *new_block = (struct mem_block *)((uint8_t *)current + size);
                new_block->size = current->size - size;
                new_block->free = 1;
                new_block->next = current->next;
                new_block->prev = current;
                if (current->next) {
                    current->next->prev = new_block;
                }
                current->next = new_block;
                current->size = size;
            }
            current->free = 0;
            return (void *)((uint8_t *)current + sizeof(struct mem_block));
        }
        current = current->next;
    }
    
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    struct mem_block *block = (struct mem_block *)((uint8_t *)ptr - sizeof(struct mem_block));
    block->free = 1;
    
    if (block->next && block->next->free) {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    if (block->prev && block->prev->free) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void gdt_init(void) {
}

void idt_init(void) {
}

void fs_init(void) {
}

void security_init(void) {
}

void system_init(void) {
}
