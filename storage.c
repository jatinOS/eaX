#include "../kernel.h"

typedef struct {
    uint32_t disk_id;
    uint32_t sector_size;
    uint64_t total_sectors;
    uint8_t status;
    uint8_t interface_type;
} disk_drive_t;

typedef struct {
    uint32_t drive_count;
    disk_drive_t drives[8];
    uint32_t active_drive;
} storage_controller_t;

static storage_controller_t storage_controller = {0};

void storage_init(void) {
    storage_controller.drive_count = 3;
    
    storage_controller.drives[0].disk_id = 0;
    storage_controller.drives[0].sector_size = 4096;
    storage_controller.drives[0].total_sectors = 49725000;
    storage_controller.drives[0].interface_type = 2;
    storage_controller.drives[0].status = 1;
    
    storage_controller.drives[1].disk_id = 1;
    storage_controller.drives[1].sector_size = 4096;
    storage_controller.drives[1].total_sectors = 374000000;
    storage_controller.drives[1].interface_type = 3;
    storage_controller.drives[1].status = 1;
    
    storage_controller.drives[2].disk_id = 2;
    storage_controller.drives[2].sector_size = 512;
    storage_controller.drives[2].total_sectors = 2880;
    storage_controller.drives[2].interface_type = 1;
    storage_controller.drives[2].status = 0;
    
    storage_controller.active_drive = 0;
}

uint32_t storage_read_sector(uint32_t drive, uint64_t sector, uint8_t *buffer) {
    if (drive >= storage_controller.drive_count) return 0;
    if (storage_controller.drives[drive].status == 0) return 0;
    
    return 4096;
}

uint32_t storage_write_sector(uint32_t drive, uint64_t sector, uint8_t *buffer) {
    if (drive >= storage_controller.drive_count) return 0;
    if (storage_controller.drives[drive].status == 0) return 0;
    
    return 4096;
}

uint32_t storage_get_drive_info(uint32_t drive, void *info) {
    if (drive >= storage_controller.drive_count) return 0;
    
    disk_drive_t *disk_info = (disk_drive_t *)info;
    disk_info[0] = storage_controller.drives[drive];
    
    return sizeof(disk_drive_t);
}

void storage_enable_drive(uint32_t drive) {
    if (drive < storage_controller.drive_count) {
        storage_controller.drives[drive].status = 1;
    }
}

void storage_disable_drive(uint32_t drive) {
    if (drive < storage_controller.drive_count) {
        storage_controller.drives[drive].status = 0;
    }
}
