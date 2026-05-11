#include "kernel.h"

#define MAX_PROCESSES 512
#define MAX_THREADS_PER_PROCESS 16
#define PROCESS_STATE_RUNNING 1
#define PROCESS_STATE_READY 2
#define PROCESS_STATE_BLOCKED 3
#define PROCESS_STATE_ZOMBIE 4
#define PROCESS_STATE_STOPPED 5
#define PROCESS_STATE_CREATED 6

typedef struct {
    uint32_t thread_id;
    uint32_t eip;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi;
    uint32_t state;
    uint32_t priority;
    uint32_t ticks_used;
    uint32_t time_quantum;
    uint32_t kernel_stack;
    uint32_t user_stack;
} thread_t;

typedef struct {
    uint32_t pid;
    uint32_t ppid;
    uint32_t uid;
    uint32_t gid;
    uint8_t state;
    uint32_t priority;
    uint32_t nice_value;
    uint32_t exit_code;
    uint32_t creation_time;
    uint32_t start_time;
    uint32_t total_cpu_time;
    uint32_t total_memory;
    uint32_t resident_memory;
    uint32_t virtual_memory;
    
    thread_t threads[MAX_THREADS_PER_PROCESS];
    uint32_t thread_count;
    
    char name[256];
    char working_dir[256];
    
    uint32_t *page_directory;
    uint32_t io_ports[16];
    
    uint32_t open_files[256];
    uint32_t open_file_count;
    
    void *signal_handlers[32];
    uint32_t signal_mask;
    
    uint32_t context_switches;
    uint32_t page_faults;
    uint32_t blocked_io_count;
} process_t;

typedef struct {
    process_t process_table[MAX_PROCESSES];
    uint32_t process_count;
    uint32_t next_pid;
    
    uint32_t *ready_queue;
    uint32_t ready_queue_head;
    uint32_t ready_queue_tail;
    
    uint32_t current_pid;
    process_t *current_process;
    thread_t *current_thread;
    
    uint64_t total_context_switches;
    uint64_t total_process_creations;
    uint64_t total_process_terminations;
    uint64_t total_thread_creations;
    
    uint32_t run_queue_length;
    uint32_t load_average_1;
    uint32_t load_average_5;
    uint32_t load_average_15;
    
    uint32_t idle_ticks;
    uint32_t total_ticks;
} process_manager_t;

static process_manager_t process_manager = {0};

void process_manager_init(void) {
    process_manager.process_count = 0;
    process_manager.next_pid = 1;
    process_manager.current_pid = 0;
    process_manager.total_context_switches = 0;
    process_manager.total_process_creations = 0;
    process_manager.total_process_terminations = 0;
    process_manager.total_thread_creations = 0;
    process_manager.run_queue_length = 0;
}

uint32_t process_create(const char *name, uint32_t priority, uint32_t uid) {
    if (process_manager.process_count >= MAX_PROCESSES) {
        return 0;
    }
    
    process_t *new_process = &process_manager.process_table[process_manager.process_count];
    new_process->pid = process_manager.next_pid++;
    new_process->ppid = process_manager.current_pid;
    new_process->uid = uid;
    new_process->gid = 0;
    new_process->state = PROCESS_STATE_CREATED;
    new_process->priority = priority;
    new_process->nice_value = 0;
    new_process->creation_time = 0;
    new_process->total_cpu_time = 0;
    new_process->total_memory = 0;
    new_process->resident_memory = 0;
    new_process->virtual_memory = 0;
    new_process->thread_count = 0;
    new_process->open_file_count = 0;
    new_process->signal_mask = 0;
    new_process->context_switches = 0;
    new_process->page_faults = 0;
    new_process->blocked_io_count = 0;
    
    for (uint32_t i = 0; i < 256; i++) {
        new_process->name[i] = name[i];
        if (name[i] == 0) break;
    }
    
    process_manager.process_count++;
    process_manager.total_process_creations++;
    
    return new_process->pid;
}

uint32_t process_terminate(uint32_t pid, uint32_t exit_code) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            process_manager.process_table[i].state = PROCESS_STATE_ZOMBIE;
            process_manager.process_table[i].exit_code = exit_code;
            process_manager.total_process_terminations++;
            return 1;
        }
    }
    return 0;
}

uint32_t thread_create(uint32_t pid, uint32_t priority) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            process_t *process = &process_manager.process_table[i];
            
            if (process->thread_count >= MAX_THREADS_PER_PROCESS) {
                return 0;
            }
            
            thread_t *new_thread = &process->threads[process->thread_count];
            new_thread->thread_id = process->thread_count++;
            new_thread->state = PROCESS_STATE_CREATED;
            new_thread->priority = priority;
            new_thread->ticks_used = 0;
            new_thread->time_quantum = 10;
            
            process_manager.total_thread_creations++;
            return new_thread->thread_id;
        }
    }
    return 0;
}

uint32_t process_get_info(uint32_t pid, void *info) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            process_t *proc_info = (process_t *)info;
            proc_info[0] = process_manager.process_table[i];
            return sizeof(process_t);
        }
    }
    return 0;
}

void schedule_next_process(void) {
    process_manager.total_context_switches++;
    
    uint32_t next_pid = 0;
    uint32_t highest_priority = 0;
    
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].state == PROCESS_STATE_READY ||
            process_manager.process_table[i].state == PROCESS_STATE_CREATED) {
            
            if (process_manager.process_table[i].priority > highest_priority) {
                highest_priority = process_manager.process_table[i].priority;
                next_pid = process_manager.process_table[i].pid;
            }
        }
    }
    
    if (next_pid > 0) {
        process_manager.current_pid = next_pid;
        for (uint32_t i = 0; i < process_manager.process_count; i++) {
            if (process_manager.process_table[i].pid == next_pid) {
                process_manager.current_process = &process_manager.process_table[i];
                process_manager.current_process->state = PROCESS_STATE_RUNNING;
                break;
            }
        }
    }
}

uint32_t process_wait(uint32_t pid, uint32_t *status) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            if (process_manager.process_table[i].state == PROCESS_STATE_ZOMBIE) {
                *status = process_manager.process_table[i].exit_code;
                return pid;
            }
        }
    }
    return 0;
}

void process_set_priority(uint32_t pid, uint32_t priority) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            process_manager.process_table[i].priority = priority;
            break;
        }
    }
}

uint32_t process_get_priority(uint32_t pid) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            return process_manager.process_table[i].priority;
        }
    }
    return 0;
}

void process_add_open_file(uint32_t pid, uint32_t fd) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            if (process_manager.process_table[i].open_file_count < 256) {
                process_manager.process_table[i].open_files[
                    process_manager.process_table[i].open_file_count++] = fd;
            }
            break;
        }
    }
}

void process_remove_open_file(uint32_t pid, uint32_t fd) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            for (uint32_t j = 0; j < process_manager.process_table[i].open_file_count; j++) {
                if (process_manager.process_table[i].open_files[j] == fd) {
                    process_manager.process_table[i].open_files[j] = 0;
                }
            }
            break;
        }
    }
}

uint32_t process_get_stats(void *stats_buffer) {
    uint32_t *stats = (uint32_t *)stats_buffer;
    stats[0] = process_manager.process_count;
    stats[1] = (uint32_t)(process_manager.total_context_switches & 0xFFFFFFFF);
    stats[2] = (uint32_t)(process_manager.total_process_creations & 0xFFFFFFFF);
    stats[3] = (uint32_t)(process_manager.total_process_terminations & 0xFFFFFFFF);
    stats[4] = (uint32_t)(process_manager.total_thread_creations & 0xFFFFFFFF);
    return 20;
}

void process_block_for_io(uint32_t pid) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            process_manager.process_table[i].state = PROCESS_STATE_BLOCKED;
            process_manager.process_table[i].blocked_io_count++;
            break;
        }
    }
}

void process_unblock(uint32_t pid) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            process_manager.process_table[i].state = PROCESS_STATE_READY;
            break;
        }
    }
}

uint32_t process_get_memory_usage(uint32_t pid) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            return process_manager.process_table[i].total_memory;
        }
    }
    return 0;
}

void process_update_cpu_time(uint32_t pid, uint32_t ticks) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            process_manager.process_table[i].total_cpu_time += ticks;
            break;
        }
    }
}

uint32_t process_get_state(uint32_t pid) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            return process_manager.process_table[i].state;
        }
    }
    return 0;
}

void process_set_working_dir(uint32_t pid, const char *dir) {
    for (uint32_t i = 0; i < process_manager.process_count; i++) {
        if (process_manager.process_table[i].pid == pid) {
            for (uint32_t j = 0; j < 256; j++) {
                process_manager.process_table[i].working_dir[j] = dir[j];
                if (dir[j] == 0) break;
            }
            break;
        }
    }
}
