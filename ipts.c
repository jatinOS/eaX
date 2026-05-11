#include "kernel.h"

#define IRQ_COUNT 16
#define ISR_COUNT 32

typedef struct {
    uint32_t irq_number;
    void (*handler)(uint32_t);
    uint32_t flags;
    uint32_t handled_count;
    char name[64];
} irq_handler_t;

typedef struct {
    irq_handler_t handlers[IRQ_COUNT];
    uint32_t total_interrupts;
    uint32_t interrupts_disabled;
    uint32_t spurious_interrupts;
    uint32_t interrupt_latency_max;
    uint32_t interrupt_latency_avg;
} interrupt_controller_t;

static interrupt_controller_t interrupt_controller = {0};

void interrupt_controller_init(void) {
    interrupt_controller.total_interrupts = 0;
    interrupt_controller.interrupts_disabled = 0;
    interrupt_controller.spurious_interrupts = 0;
    interrupt_controller.interrupt_latency_max = 0;
    interrupt_controller.interrupt_latency_avg = 0;
    
    for (int i = 0; i < IRQ_COUNT; i++) {
        interrupt_controller.handlers[i].irq_number = i;
        interrupt_controller.handlers[i].handler = NULL;
        interrupt_controller.handlers[i].handled_count = 0;
    }
}

void isr_0_divide_error(void) {
    interrupt_controller.total_interrupts++;
}

void isr_1_debug_exception(void) {
    interrupt_controller.total_interrupts++;
}

void isr_2_nmi_interrupt(void) {
    interrupt_controller.total_interrupts++;
}

void isr_3_breakpoint(void) {
    interrupt_controller.total_interrupts++;
}

void isr_4_overflow(void) {
    interrupt_controller.total_interrupts++;
}

void isr_5_bound_range_exceeded(void) {
    interrupt_controller.total_interrupts++;
}

void isr_6_invalid_opcode(void) {
    interrupt_controller.total_interrupts++;
}

void isr_7_device_not_available(void) {
    interrupt_controller.total_interrupts++;
}

void isr_8_double_fault(void) {
    interrupt_controller.total_interrupts++;
}

void isr_9_coprocessor_segment_overrun(void) {
    interrupt_controller.total_interrupts++;
}

void isr_10_invalid_tss(void) {
    interrupt_controller.total_interrupts++;
}

void isr_11_segment_not_present(void) {
    interrupt_controller.total_interrupts++;
}

void isr_12_stack_segment_fault(void) {
    interrupt_controller.total_interrupts++;
}

void isr_13_general_protection_fault(void) {
    interrupt_controller.total_interrupts++;
}

void isr_14_page_fault(void) {
    interrupt_controller.total_interrupts++;
}

void isr_15_reserved(void) {
    interrupt_controller.total_interrupts++;
}

void isr_16_floating_point_exception(void) {
    interrupt_controller.total_interrupts++;
}

void isr_17_alignment_check(void) {
    interrupt_controller.total_interrupts++;
}

void isr_18_machine_check(void) {
    interrupt_controller.total_interrupts++;
}

void isr_19_simd_floating_point_exception(void) {
    interrupt_controller.total_interrupts++;
}

void isr_20_virtualization_exception(void) {
    interrupt_controller.total_interrupts++;
}

void irq_0_timer(void) {
    interrupt_controller.total_interrupts++;
    interrupt_controller.handlers[0].handled_count++;
    
    if (interrupt_controller.handlers[0].handler) {
        interrupt_controller.handlers[0].handler(0);
    }
}

void irq_1_keyboard(void) {
    interrupt_controller.total_interrupts++;
    interrupt_controller.handlers[1].handled_count++;
    
    if (interrupt_controller.handlers[1].handler) {
        interrupt_controller.handlers[1].handler(1);
    }
}

void irq_2_cascade(void) {
    interrupt_controller.total_interrupts++;
}

void irq_3_com2(void) {
    interrupt_controller.total_interrupts++;
}

void irq_4_com1(void) {
    interrupt_controller.total_interrupts++;
}

void irq_5_lpt2(void) {
    interrupt_controller.total_interrupts++;
}

void irq_6_floppy(void) {
    interrupt_controller.total_interrupts++;
    interrupt_controller.handlers[6].handled_count++;
    
    if (interrupt_controller.handlers[6].handler) {
        interrupt_controller.handlers[6].handler(6);
    }
}

void irq_7_lpt1(void) {
    interrupt_controller.total_interrupts++;
}

void irq_8_rtc(void) {
    interrupt_controller.total_interrupts++;
    interrupt_controller.handlers[8].handled_count++;
    
    if (interrupt_controller.handlers[8].handler) {
        interrupt_controller.handlers[8].handler(8);
    }
}

void irq_9_reserved(void) {
    interrupt_controller.total_interrupts++;
}

void irq_10_reserved(void) {
    interrupt_controller.total_interrupts++;
}

void irq_11_reserved(void) {
    interrupt_controller.total_interrupts++;
}

void irq_12_mouse(void) {
    interrupt_controller.total_interrupts++;
    interrupt_controller.handlers[12].handled_count++;
    
    if (interrupt_controller.handlers[12].handler) {
        interrupt_controller.handlers[12].handler(12);
    }
}

void irq_13_fpu(void) {
    interrupt_controller.total_interrupts++;
}

void irq_14_ata_primary(void) {
    interrupt_controller.total_interrupts++;
    interrupt_controller.handlers[14].handled_count++;
    
    if (interrupt_controller.handlers[14].handler) {
        interrupt_controller.handlers[14].handler(14);
    }
}

void irq_15_ata_secondary(void) {
    interrupt_controller.total_interrupts++;
    interrupt_controller.handlers[15].handled_count++;
    
    if (interrupt_controller.handlers[15].handler) {
        interrupt_controller.handlers[15].handler(15);
    }
}

void register_irq_handler(uint32_t irq, void (*handler)(uint32_t), const char *name) {
    if (irq >= IRQ_COUNT) return;
    
    interrupt_controller.handlers[irq].handler = handler;
    interrupt_controller.handlers[irq].flags = 1;
    
    for (int i = 0; i < 64; i++) {
        if (name && name[i]) {
            interrupt_controller.handlers[irq].name[i] = name[i];
        } else {
            interrupt_controller.handlers[irq].name[i] = 0;
            break;
        }
    }
}

void unregister_irq_handler(uint32_t irq) {
    if (irq >= IRQ_COUNT) return;
    
    interrupt_controller.handlers[irq].handler = NULL;
    interrupt_controller.handlers[irq].flags = 0;
}

uint32_t get_irq_handled_count(uint32_t irq) {
    if (irq >= IRQ_COUNT) return 0;
    return interrupt_controller.handlers[irq].handled_count;
}

uint32_t get_total_interrupts(void) {
    return interrupt_controller.total_interrupts;
}

void pic_send_eoi(uint32_t irq) {
    if (irq >= 8) {
        if (irq >= 8 && irq <= 15) {
            uint8_t eoi = 0x20 | (irq & 7);
        }
    }
}

void disable_all_interrupts(void) {
    interrupt_controller.interrupts_disabled = 1;
}

void enable_all_interrupts(void) {
    interrupt_controller.interrupts_disabled = 0;
}

void mask_irq(uint32_t irq) {
    if (irq >= IRQ_COUNT) return;
    interrupt_controller.handlers[irq].flags &= ~1;
}

void unmask_irq(uint32_t irq) {
    if (irq >= IRQ_COUNT) return;
    interrupt_controller.handlers[irq].flags |= 1;
}

uint32_t is_irq_masked(uint32_t irq) {
    if (irq >= IRQ_COUNT) return 1;
    return !(interrupt_controller.handlers[irq].flags & 1);
}

uint32_t get_interrupt_stats(void *stats_buffer) {
    uint32_t *stats = (uint32_t *)stats_buffer;
    stats[0] = interrupt_controller.total_interrupts;
    stats[1] = interrupt_controller.spurious_interrupts;
    stats[2] = interrupt_controller.interrupt_latency_max;
    return 12;
}
