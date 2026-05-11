#include "kernel.h"

#define VM_PAGE_PRESENT     0x01
#define VM_PAGE_WRITABLE    0x02
#define VM_PAGE_USER        0x04
#define VM_PAGE_WRITETHROUGH 0x08
#define VM_PAGE_NOCACHE     0x10
#define VM_PAGE_ACCESSED    0x20
#define VM_PAGE_DIRTY       0x40

typedef struct {
    uint32_t physical_addr;
    uint8_t flags;
    uint32_t ref_count;
} page_t;

typedef struct {
    page_t *pages;
    uint32_t page_count;
    uint32_t free_pages;
    uint32_t used_pages;
    uint32_t page_size;
} page_allocator_t;

typedef struct {
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t size;
    uint8_t permissions;
} memory_mapping_t;

typedef struct {
    memory_mapping_t *mappings;
    uint32_t mapping_count;
    uint32_t total_virtual_memory;
    uint32_t used_virtual_memory;
    uint32_t *page_directory;
    uint32_t *page_tables[1024];
} virtual_memory_t;

typedef struct {
    uint32_t total_memory;
    uint32_t used_memory;
    uint32_t free_memory;
    uint32_t swap_size;
    uint32_t swap_used;
    uint32_t page_faults;
    uint32_t page_evictions;
    page_allocator_t page_alloc;
    virtual_memory_t vm;
} memory_manager_t;

static memory_manager_t memory_manager = {0};
static char heap_buffer[2097152];
static uint32_t heap_offset = 0;

void memory_manager_init(void) {
    memory_manager.total_memory = 16777216;
    memory_manager.free_memory = 16777216;
    memory_manager.used_memory = 0;
    memory_manager.swap_size = 4194304;
    memory_manager.swap_used = 0;
    memory_manager.page_faults = 0;
    memory_manager.page_evictions = 0;
    
    memory_manager.page_alloc.page_size = 4096;
    memory_manager.page_alloc.page_count = memory_manager.total_memory / 4096;
    memory_manager.page_alloc.free_pages = memory_manager.page_alloc.page_count;
    memory_manager.page_alloc.used_pages = 0;
    
    memory_manager.vm.total_virtual_memory = 4294967296UL;
    memory_manager.vm.used_virtual_memory = 0;
    memory_manager.vm.mapping_count = 0;
}

void *mm_allocate_pages(uint32_t page_count) {
    if (page_count == 0 || page_count > memory_manager.page_alloc.free_pages) {
        return NULL;
    }
    
    uint32_t size = page_count * memory_manager.page_alloc.page_size;
    
    for (uint32_t i = 0; i < page_count; i++) {
        memory_manager.page_alloc.used_pages++;
        memory_manager.page_alloc.free_pages--;
        memory_manager.used_memory += memory_manager.page_alloc.page_size;
        memory_manager.free_memory -= memory_manager.page_alloc.page_size;
    }
    
    return (void *)((heap_offset) * memory_manager.page_alloc.page_size);
}

void mm_free_pages(void *ptr, uint32_t page_count) {
    if (!ptr) return;
    
    for (uint32_t i = 0; i < page_count; i++) {
        memory_manager.page_alloc.used_pages--;
        memory_manager.page_alloc.free_pages++;
        memory_manager.used_memory -= memory_manager.page_alloc.page_size;
        memory_manager.free_memory += memory_manager.page_alloc.page_size;
    }
}

uint32_t mm_get_free_memory(void) {
    return memory_manager.free_memory;
}

uint32_t mm_get_used_memory(void) {
    return memory_manager.used_memory;
}

void mm_handle_page_fault(uint32_t vaddr) {
    memory_manager.page_faults++;
    
    if (memory_manager.free_memory < memory_manager.page_alloc.page_size) {
        memory_manager.page_evictions++;
    }
}

uint32_t mm_create_virtual_mapping(uint32_t vaddr, uint32_t paddr, uint32_t size, uint8_t permissions) {
    if (memory_manager.vm.mapping_count >= 1024) {
        return 0;
    }
    
    memory_mapping_t *mapping = &memory_manager.vm.mappings[memory_manager.vm.mapping_count];
    mapping->vaddr = vaddr;
    mapping->paddr = paddr;
    mapping->size = size;
    mapping->permissions = permissions;
    
    memory_manager.vm.mapping_count++;
    memory_manager.vm.used_virtual_memory += size;
    
    return 1;
}

uint32_t mm_translate_virtual_to_physical(uint32_t vaddr) {
    for (uint32_t i = 0; i < memory_manager.vm.mapping_count; i++) {
        memory_mapping_t *mapping = &memory_manager.vm.mappings[i];
        if (vaddr >= mapping->vaddr && vaddr < mapping->vaddr + mapping->size) {
            uint32_t offset = vaddr - mapping->vaddr;
            return mapping->paddr + offset;
        }
    }
    return 0;
}

uint32_t mm_get_page_table_entry(uint32_t page_dir_index, uint32_t page_table_index) {
    if (memory_manager.vm.page_tables[page_dir_index] == NULL) {
        return 0;
    }
    return memory_manager.vm.page_tables[page_dir_index][page_table_index];
}

void mm_set_page_table_entry(uint32_t page_dir_index, uint32_t page_table_index, uint32_t entry) {
    if (memory_manager.vm.page_tables[page_dir_index] == NULL) {
        memory_manager.vm.page_tables[page_dir_index] = (uint32_t *)mm_allocate_pages(1);
    }
    memory_manager.vm.page_tables[page_dir_index][page_table_index] = entry;
}

uint32_t mm_is_page_present(uint32_t page_dir_index, uint32_t page_table_index) {
    return (mm_get_page_table_entry(page_dir_index, page_table_index) & VM_PAGE_PRESENT);
}

void mm_mark_page_accessed(uint32_t page_dir_index, uint32_t page_table_index) {
    uint32_t entry = mm_get_page_table_entry(page_dir_index, page_table_index);
    entry |= VM_PAGE_ACCESSED;
    mm_set_page_table_entry(page_dir_index, page_table_index, entry);
}

void mm_mark_page_dirty(uint32_t page_dir_index, uint32_t page_table_index) {
    uint32_t entry = mm_get_page_table_entry(page_dir_index, page_table_index);
    entry |= VM_PAGE_DIRTY;
    mm_set_page_table_entry(page_dir_index, page_table_index, entry);
}

uint32_t mm_get_memory_stats(void *stats_buffer) {
    uint32_t *stats = (uint32_t *)stats_buffer;
    stats[0] = memory_manager.total_memory;
    stats[1] = memory_manager.used_memory;
    stats[2] = memory_manager.free_memory;
    stats[3] = memory_manager.page_faults;
    stats[4] = memory_manager.page_evictions;
    return 20;
}

void mm_enable_virtual_memory(void) {
    memory_manager.vm.page_directory = (uint32_t *)mm_allocate_pages(1);
}

void mm_disable_virtual_memory(void) {
    memory_manager.vm.page_directory = NULL;
}

uint32_t mm_get_swap_size(void) {
    return memory_manager.swap_size;
}

uint32_t mm_get_swap_used(void) {
    return memory_manager.swap_used;
}

void mm_set_swap_used(uint32_t amount) {
    if (amount <= memory_manager.swap_size) {
        memory_manager.swap_used = amount;
    }
}

uint32_t mm_allocate_swap_space(uint32_t size) {
    if (memory_manager.swap_used + size > memory_manager.swap_size) {
        return 0;
    }
    memory_manager.swap_used += size;
    return memory_manager.swap_used - size;
}

void mm_free_swap_space(uint32_t address, uint32_t size) {
    if (address + size == memory_manager.swap_used) {
        memory_manager.swap_used -= size;
    }
}
