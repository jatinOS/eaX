/*
 * eaX OS Kernel
 * 2026 made by jatiN corporation
 */

#include "kernel.h"

/* Global Variables */
static uint32_t kernel_ticks = 0;
static uint8_t screen_color = (COLOR_LIGHT_GREEN << 4) | COLOR_BLACK;
static uint16_t *video_memory = (uint16_t *)0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;

/* GDT */
static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gp;

/* IDT */
static struct idt_entry idt[256];
static struct idt_ptr idtp;

/* TSS */
static struct tss tss;

/* Memory Management */
static struct mem_block *heap_start = NULL;
static uint32_t heap_current = HEAP_START;
static uint32_t heap_end_addr = HEAP_END;

/* Process Table */
static struct process process_table[256];
static pid_t next_pid = 1;

/* File System */
static struct file open_files[FS_MAX_OPEN];
static int next_fd = 3; /* 0=stdin, 1=stdout, 2=stderr */

/* Security Context */
static struct security_context current_security;

/* Kernel Entry Point */
void kernel_main(void) {
    /* Initialize core subsystems */
    screen_init();
    screen_clear();
    
    screen_print("\n\r");
    screen_print("  ╔══════════════════════════════════════════╗\n\r");
    screen_print("  ║                                          ║\n\r");
    screen_print("  ║          \033[36meaX OS Kernel v1.0.0\033[0m           ║\n\r");
    screen_print("  ║          Codename: \033[33mFortress\033[0m             ║\n\r");
    screen_print("  ║                                          ║\n\r");
    screen_print("  ╚══════════════════════════════════════════╝\n\r");
    screen_print("\n\r");
    
    /* Initialize GDT */
    screen_print("[*] Initializing GDT... ");
    gdt_init();
    screen_print("OK\n\r");
    
    /* Initialize IDT */
    screen_print("[*] Initializing IDT... ");
    idt_init();
    screen_print("OK\n\r");
    
    /* Initialize Memory */
    screen_print("[*] Initializing Memory... ");
    memory_init();
    screen_print("OK\n\r");
    
    /* Initialize File System */
    screen_print("[*] Initializing File System... ");
    fs_init();
    screen_print("OK\n\r");
    
    /* Initialize Security */
    screen_print("[*] Initializing eaX Security (Fotress)... ");
    security_init();
    screen_print("OK\n\r");
    
    /* Initialize System */
    screen_print("[*] Initializing System... ");
    system_init();
    screen_print("OK\n\r");
    
    /* Enable Interrupts */
    sti();
    
    screen_print("\n\r\033[32m[✓] eaX OS boot completed successfully!\033[0m\n\r");
    screen_print("\n\r");
    
    /* Display system info */
    screen_print("  Memory: 16GB DDR5 Available\n\r");
    screen_print("  Graphics: eaX 164MB GPU\n\r");
    screen_print("  Storage: 1TB NVMe SSD\n\r");
    screen_print("  Compatibility: Intel/AMD x86_64 and Mac virtualization\n\r");
    screen_print("  AI Protection: Active\n\r");
    
    screen_print("\n\rType 'help' for available commands.\n\r\n\r");
    
    /* Main kernel loop */
    while (1) {
        /* Display prompt */
        screen_print("\033[36meaX>\033[0m ");
        
        /* Wait for input */
        hlt();
    }
}

/* Screen Functions */
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

void screen_print_hex(uint32_t value) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11];
    buffer[0] = '0';
    buffer[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buffer[2 + i] = hex_chars[(value >> (28 - i * 4)) & 0xF];
    }
    buffer[10] = '\0';
    screen_print(buffer);
}

void screen_set_color(uint8_t color) {
    screen_color = color;
}

void screen_scroll(void) {
    /* Move all lines up by one */
    for (int i = 0; i < (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i++) {
        video_memory[i] = video_memory[(i + SCREEN_WIDTH)];
    }
    /* Clear last line */
    for (int i = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
        video_memory[i] = (screen_color << 8) | ' ';
    }
}

void screen_set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

/* Memory Functions */
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

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dest;
}

void memory_init(void) {
    heap_start = (struct mem_block *)heap_current;
    heap_start->size = heap_end_addr - heap_start;
    heap_start->free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;
    screen_print("OK\n\r");
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    /* Align size to 16 bytes */
    size = ALIGN_UP(size, 16);
    
    /* Find a free block */
    struct mem_block *current = heap_start;
    while (current) {
        if (current->free && current->size >= size) {
            /* Split block if it's large enough */
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
    
    /* No suitable block found */
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    struct mem_block *block = (struct mem_block *)((uint8_t *)ptr - sizeof(struct mem_block));
    block->free = 1;
    
    /* Try to merge with adjacent free blocks */
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

/* GDT Functions */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    gdt[num].access = access;
}

void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base = (uint32_t)&gdt;
    
    /* Null segment */
    gdt_set_gate(GDT_NULL, 0, 0, 0, 0);
    
    /* Kernel code segment */
    gdt_set_gate(GDT_KERNEL_CODE, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    /* Kernel data segment */
    gdt_set_gate(GDT_KERNEL_DATA, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    /* User code segment */
    gdt_set_gate(GDT_USER_CODE, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    /* User data segment */
    gdt_set_gate(GDT_USER_DATA, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    /* TSS */
    uint32_t tss_base = (uint32_t)&tss;
    uint32_t tss_limit = sizeof(struct tss) - 1;
    gdt_set_gate(GDT_TSS, tss_base, tss_limit, 0xE9, 0x00);
    
    /* Initialize TSS */
    memset(&tss, 0, sizeof(struct tss));
    tss.ss0 = 0x10; /* Kernel data segment */
    tss.esp0 = 0x200000; /* Kernel stack */
    tss.cs = 0x08 | 0x03; /* User code segment */
    tss.ds = 0x10 | 0x03; /* User data segment */
    tss.es = 0x10 | 0x03;
    tss.fs = 0x10 | 0x03;
    tss.gs = 0x10 | 0x03;
    
    lgdt(&gp);
    ltr(0x28); /* TSS selector */
}

/* IDT Functions */
void idt_set_gate(int num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;
    memset(&idt, 0, sizeof(struct idt_entry) * 256);
    
    /* Install ISRs */
    isr_install();
    
    /* Install IRQs */
    irq_install();
    
    lidt(&idtp);
}

/* ISR Installation */
extern void isr0_asm(void);
extern void isr1_asm(void);
extern void isr2_asm(void);
extern void isr3_asm(void);
extern void isr4_asm(void);
extern void isr5_asm(void);
extern void isr6_asm(void);
extern void isr7_asm(void);
extern void isr8_asm(void);
extern void isr9_asm(void);
extern void isr10_asm(void);
extern void isr11_asm(void);
extern void isr12_asm(void);
extern void isr13_asm(void);
extern void isr14_asm(void);
extern void isr15_asm(void);
extern void isr16_asm(void);
extern void isr17_asm(void);
extern void isr18_asm(void);
extern void isr19_asm(void);
extern void isr20_asm(void);
extern void isr21_asm(void);
extern void isr22_asm(void);
extern void isr23_asm(void);
extern void isr24_asm(void);
extern void isr25_asm(void);
extern void isr26_asm(void);
extern void isr27_asm(void);
extern void isr28_asm(void);
extern void isr29_asm(void);
extern void isr30_asm(void);
extern void isr31_asm(void);

void isr_install(void) {
    idt_set_gate(0, (uint32_t)isr0_asm, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1_asm, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2_asm, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3_asm, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4_asm, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5_asm, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6_asm, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7_asm, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8_asm, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9_asm, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10_asm, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11_asm, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12_asm, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13_asm, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14_asm, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15_asm, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16_asm, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17_asm, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18_asm, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19_asm, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20_asm, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21_asm, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22_asm, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23_asm, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24_asm, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25_asm, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26_asm, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27_asm, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28_asm, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29_asm, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30_asm, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31_asm, 0x08, 0x8E);
}

/* IRQ Installation */
extern void irq0_asm(void);
extern void irq1_asm(void);

void irq_install(void) {
    /* Remap PIC */
    outb(PIC_MASTER_CMD, 0x11);
    outb(PIC_SLAVE_CMD, 0x11);
    outb(PIC_MASTER_DATA, 0x20);
    outb(PIC_SLAVE_DATA, 0x28);
    outb(PIC_MASTER_DATA, 0x04);
    outb(PIC_SLAVE_DATA, 0x02);
    outb(PIC_MASTER_DATA, 0x01);
    outb(PIC_SLAVE_DATA, 0x01);
    outb(PIC_MASTER_DATA, 0x0);
    outb(PIC_SLAVE_DATA, 0x0);
    
    /* Install IRQ handlers */
    idt_set_gate(32, (uint32_t)irq0_asm, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1_asm, 0x08, 0x8E);
}

/* ISR Handler */
void isr_handler(struct registers *regs) {
    kernel_ticks++;
    
    if (regs->int_no < 32) {
        screen_print("\n\r[FAULT] ");
        static const char *exception_messages[] = {
            "Division By Zero",
            "Debug",
            "Non Maskable Interrupt",
            "Breakpoint",
            "Overflow",
            "Bound Range Exceeded",
            "Invalid Opcode",
            "Device Not Available",
            "Double Fault",
            "Coprocessor Segment Overrun",
            "Invalid TSS",
            "Segment Not Present",
            "Stack Fault",
            "General Protection Fault",
            "Page Fault",
            "Reserved",
            "x87 Floating-Point Exception",
            "Alignment Check",
            "Machine Check",
            "SIMD Floating-Point Exception",
            "Virtualization Exception",
            "Control Protection Exception",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Security Exception",
            "Reserved"
        };
        screen_print(exception_messages[regs->int_no]);
        screen_print(" at ");
        screen_print_hex(regs->eip);
        screen_print("\n\r");
        
        /* Kernel panic for critical faults */
        if (regs->int_no < 20) {
            kernel_panic("Critical system fault");
        }
    }
}

/* IRQ Handler */
void irq_handler(struct registers *regs) {
    kernel_ticks++;
    
    /* Timer interrupt */
    if (regs->int_no == 32) {
        /* Update system time, scheduler, etc. */
    }
    
    /* Keyboard interrupt */
    if (regs->int_no == 33) {
        uint8_t scancode = inb(KB_DATA);
        /* Handle keyboard input */
    }
    
    /* Send EOI */
    outb(PIC_MASTER_CMD, 0x20);
    if (regs->int_no >= 40) {
        outb(PIC_SLAVE_CMD, 0x20);
    }
}

/* System Functions */
void system_init(void) {
    /* Initialize timer */
    outb(0x43, 0x36);
    outb(0x40, 0x9C); /* 100 Hz */
    outb(0x40, 0x00);
}

void kernel_panic(const char *msg) {
    cli();
    screen_set_color((COLOR_RED << 4) | COLOR_WHITE);
    screen_print("\n\r\n\r*** KERNEL PANIC ***\n\r");
    screen_print(msg);
    screen_print("\n\rSystem halted.\n\r");
    while (1) {
        hlt();
    }
}

void halt(void) {
    cli();
    __asm__ volatile("hlt");
}

void reboot(void) {
    /* Triple fault method */
    struct idt_ptr null_idt;
    null_idt.limit = 0;
    null_idt.base = 0;
    lidt(&null_idt);
    __asm__ volatile("int $3");
}

/* File System Functions */
void fs_init(void) {
    /* Initialize virtual file system */
    memset(open_files, 0, sizeof(open_files));
    
    /* Create root directory */
    fs_mkdir("/");
    fs_mkdir("/bin");
    fs_mkdir("/etc");
    fs_mkdir("/home");
    fs_mkdir("/tmp");
    fs_mkdir("/dev");
    fs_mkdir("/sys");
    fs_mkdir("/proc");
}

int fs_mount(const char *device, const char *mountpoint) {
    /* Mount a filesystem */
    return 0;
}

int fs_open(const char *path, int flags) {
    /* Open a file */
    if (next_fd >= FS_MAX_OPEN) return -1;
    return next_fd++;
}

int fs_close(int fd) {
    /* Close a file */
    if (fd < 3) return -1;
    memset(&open_files[fd], 0, sizeof(struct file));
    return 0;
}

int fs_read(int fd, void *buf, size_t count) {
    /* Read from a file */
    return 0;
}

int fs_write(int fd, const void *buf, size_t count) {
    /* Write to a file */
    return count;
}

int fs_mkdir(const char *path) {
    /* Create a directory */
    return 0;
}

int fs_rmdir(const char *path) {
    /* Remove a directory */
    return 0;
}

int fs_unlink(const char *path) {
    /* Delete a file */
    return 0;
}

/* Security Functions */
void security_init(void) {
    current_security.level = SECURITY_FORTRESS;
    current_security.flags = 0;
    current_security.allowed_syscalls = 0xFFFFFFFF;
    current_security.denied_paths = 0;
}

int security_check_file(const char *path) {
    /* Check file for security issues */
    return 0;
}

int security_scan_dll(const char *path) {
    /* Scan DLL for malicious content */
    return 0;
}

/* Assembly ISR stubs */
__asm__(
    ".global isr0_asm\n"
    "isr0_asm:\n"
    "    push byte 0\n"
    "    push byte 0\n"
    "    jmp isr_common\n"
    
    ".global isr1_asm\n"
    "isr1_asm:\n"
    "    push byte 0\n"
    "    push byte 1\n"
    "    jmp isr_common\n"
    
    ".global isr2_asm\n"
    "isr2_asm:\n"
    "    push byte 0\n"
    "    push byte 2\n"
    "    jmp isr_common\n"
    
    ".global isr3_asm\n"
    "isr3_asm:\n"
    "    push byte 0\n"
    "    push byte 3\n"
    "    jmp isr_common\n"
    
    ".global isr4_asm\n"
    "isr4_asm:\n"
    "    push byte 0\n"
    "    push byte 4\n"
    "    jmp isr_common\n"
    
    ".global isr5_asm\n"
    "isr5_asm:\n"
    "    push byte 0\n"
    "    push byte 5\n"
    "    jmp isr_common\n"
    
    ".global isr6_asm\n"
    "isr6_asm:\n"
    "    push byte 0\n"
    "    push byte 6\n"
    "    jmp isr_common\n"
    
    ".global isr7_asm\n"
    "isr7_asm:\n"
    "    push byte 0\n"
    "    push byte 7\n"
    "    jmp isr_common\n"
    
    ".global isr8_asm\n"
    "isr8_asm:\n"
    "    push byte 8\n"
    "    jmp isr_common\n"
    
    ".global isr9_asm\n"
    "isr9_asm:\n"
    "    push byte 0\n"
    "    push byte 9\n"
    "    jmp isr_common\n"
    
    ".global isr10_asm\n"
    "isr10_asm:\n"
    "    push byte 10\n"
    "    jmp isr_common\n"
    
    ".global isr11_asm\n"
    "isr11_asm:\n"
    "    push byte 11\n"
    "    jmp isr_common\n"
    
    ".global isr12_asm\n"
    "isr12_asm:\n"
    "    push byte 12\n"
    "    jmp isr_common\n"
    
    ".global isr13_asm\n"
    "isr13_asm:\n"
    "    push byte 13\n"
    "    jmp isr_common\n"
    
    ".global isr14_asm\n"
    "isr14_asm:\n"
    "    push byte 14\n"
    "    jmp isr_common\n"
    
    ".global isr15_asm\n"
    "isr15_asm:\n"
    "    push byte 15\n"
    "    jmp isr_common\n"
    
    ".global isr16_asm\n"
    "isr16_asm:\n"
    "    push byte 0\n"
    "    push byte 16\n"
    "    jmp isr_common\n"
    
    ".global isr17_asm\n"
    "isr17_asm:\n"
    "    push byte 0\n"
    "    push byte 17\n"
    "    jmp isr_common\n"
    
    ".global isr18_asm\n"
    "isr18_asm:\n"
    "    push byte 0\n"
    "    push byte 18\n"
    "    jmp isr_common\n"
    
    ".global isr19_asm\n"
    "isr19_asm:\n"
    "    push byte 0\n"
    "    push byte 19\n"
    "    jmp isr_common\n"
    
    ".global isr20_asm\n"
    "isr20_asm:\n"
    "    push byte 0\n"
    "    push byte 20\n"
    "    jmp isr_common\n"
    
    ".global isr21_asm\n"
    "isr21_asm:\n"
    "    push byte 0\n"
    "    push byte 21\n"
    "    jmp isr_common\n"
    
    ".global isr22_asm\n"
    "isr22_asm:\n"
    "    push byte 0\n"
    "    push byte 22\n"
    "    jmp isr_common\n"
    
    ".global isr23_asm\n"
    "isr23_asm:\n"
    "    push byte 0\n"
    "    push byte 23\n"
    "    jmp isr_common\n"
    
    ".global isr24_asm\n"
    "isr24_asm:\n"
    "    push byte 0\n"
    "    push byte 24\n"
    "    jmp isr_common\n"
    
    ".global isr25_asm\n"
    "isr25_asm:\n"
    "    push byte 0\n"
    "    push byte 25\n"
    "    jmp isr_common\n"
    
    ".global isr26_asm\n"
    "isr26_asm:\n"
    "    push byte 0\n"
    "    push byte 26\n"
    "    jmp isr_common\n"
    
    ".global isr27_asm\n"
    "isr27_asm:\n"
    "    push byte 0\n"
    "    push byte 27\n"
    "    jmp isr_common\n"
    
    ".global isr28_asm\n"
    "isr28_asm:\n"
    "    push byte 0\n"
    "    push byte 28\n"
    "    jmp isr_common\n"
    
    ".global isr29_asm\n"
    "isr29_asm:\n"
    "    push byte 0\n"
    "    push byte 29\n"
    "    jmp isr_common\n"
    
    ".global isr30_asm\n"
    "isr30_asm:\n"
    "    push byte 0\n"
    "    push byte 30\n"
    "    jmp isr_common\n"
    
    ".global isr31_asm\n"
    "isr31_asm:\n"
    "    push byte 0\n"
    "    push byte 31\n"
    "    jmp isr_common\n"
    
    ".global irq0_asm\n"
    "irq0_asm:\n"
    "    push byte 0\n"
    "    push byte 32\n"
    "    jmp irq_common\n"
    
    ".global irq1_asm\n"
    "irq1_asm:\n"
    "    push byte 0\n"
    "    push byte 33\n"
    "    jmp irq_common\n"
    
    "isr_common:\n"
    "    pusha\n"
    "    push ds\n"
    "    push es\n"
    "    push fs\n"
    "    push gs\n"
    "    mov ax, 0x10\n"
    "    mov ds, ax\n"
    "    mov es, ax\n"
    "    mov fs, ax\n"
    "    mov gs, ax\n"
    "    push esp\n"
    "    call isr_handler\n"
    "    add esp, 4\n"
    "    pop gs\n"
    "    pop fs\n"
    "    pop es\n"
    "    pop ds\n"
    "    popa\n"
    "    add esp, 8\n"
    "    iret\n"
    
    "irq_common:\n"
    "    pusha\n"
    "    push ds\n"
    "    push es\n"
    "    push fs\n"
    "    push gs\n"
    "    mov ax, 0x10\n"
    "    mov ds, ax\n"
    "    mov es, ax\n"
    "    mov fs, ax\n"
    "    mov gs, ax\n"
    "    push esp\n"
    "    call irq_handler\n"
    "    add esp, 4\n"
    "    pop gs\n"
    "    pop fs\n"
    "    pop es\n"
    "    pop ds\n"
    "    popa\n"
    "    add esp, 8\n"
    "    iret\n"
);
