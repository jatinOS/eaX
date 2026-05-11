#include "kernel.h"

#define MAX_DEVICES 256
#define DEVICE_TYPE_CHARACTER 0x01
#define DEVICE_TYPE_BLOCK 0x02
#define DEVICE_TYPE_NETWORK 0x04
#define DEVICE_TYPE_MISC 0x08

typedef struct {
    uint32_t device_id;
    uint8_t device_type;
    uint32_t major_number;
    uint32_t minor_number;
    char device_name[128];
    char device_path[256];
    uint8_t status;
    uint8_t initialized;
    void *device_data;
    uint32_t (*device_open)(uint32_t);
    uint32_t (*device_close)(uint32_t);
    uint32_t (*device_read)(uint32_t, uint8_t *, uint32_t);
    uint32_t (*device_write)(uint32_t, const uint8_t *, uint32_t);
    uint32_t (*device_ioctl)(uint32_t, uint32_t, void *);
} device_t;

typedef struct {
    device_t devices[MAX_DEVICES];
    uint32_t device_count;
    uint32_t next_device_id;
} device_manager_t;

static device_manager_t device_manager = {0};

void device_manager_init(void) {
    device_manager.device_count = 0;
    device_manager.next_device_id = 1;
}

uint32_t device_register(const char *name, uint8_t device_type, 
                        uint32_t major, uint32_t minor) {
    if (device_manager.device_count >= MAX_DEVICES) {
        return 0;
    }
    
    device_t *device = &device_manager.devices[device_manager.device_count];
    device->device_id = device_manager.next_device_id++;
    device->device_type = device_type;
    device->major_number = major;
    device->minor_number = minor;
    device->status = 0;
    device->initialized = 0;
    device->device_data = NULL;
    
    for (uint32_t i = 0; i < 128; i++) {
        if (name && name[i]) {
            device->device_name[i] = name[i];
        } else {
            device->device_name[i] = 0;
            break;
        }
    }
    
    device_manager.device_count++;
    return device->device_id;
}

uint32_t device_unregister(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            for (uint32_t j = i; j < device_manager.device_count - 1; j++) {
                device_manager.devices[j] = device_manager.devices[j + 1];
            }
            device_manager.device_count--;
            return 1;
        }
    }
    return 0;
}

uint32_t device_initialize(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            device_manager.devices[i].initialized = 1;
            device_manager.devices[i].status = 1;
            return 1;
        }
    }
    return 0;
}

uint32_t device_open(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            if (device_manager.devices[i].device_open) {
                return device_manager.devices[i].device_open(device_id);
            }
            return 1;
        }
    }
    return 0;
}

uint32_t device_close(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            if (device_manager.devices[i].device_close) {
                return device_manager.devices[i].device_close(device_id);
            }
            return 1;
        }
    }
    return 0;
}

uint32_t device_read(uint32_t device_id, uint8_t *buffer, uint32_t size) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            if (device_manager.devices[i].device_read) {
                return device_manager.devices[i].device_read(device_id, buffer, size);
            }
            return 0;
        }
    }
    return 0;
}

uint32_t device_write(uint32_t device_id, const uint8_t *buffer, uint32_t size) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            if (device_manager.devices[i].device_write) {
                return device_manager.devices[i].device_write(device_id, buffer, size);
            }
            return size;
        }
    }
    return 0;
}

uint32_t device_ioctl(uint32_t device_id, uint32_t cmd, void *arg) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            if (device_manager.devices[i].device_ioctl) {
                return device_manager.devices[i].device_ioctl(device_id, cmd, arg);
            }
            return 0;
        }
    }
    return 0;
}

uint32_t device_get_info(uint32_t device_id, void *info) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            device_t *device_info = (device_t *)info;
            device_info[0] = device_manager.devices[i];
            return sizeof(device_t);
        }
    }
    return 0;
}

uint32_t device_list(void *device_list, uint32_t max_count) {
    uint32_t count = 0;
    device_t *list = (device_t *)device_list;
    
    for (uint32_t i = 0; i < device_manager.device_count && i < max_count; i++) {
        list[i] = device_manager.devices[i];
        count++;
    }
    
    return count;
}

uint32_t device_set_handler(uint32_t device_id,
                           uint32_t (*open_handler)(uint32_t),
                           uint32_t (*close_handler)(uint32_t),
                           uint32_t (*read_handler)(uint32_t, uint8_t *, uint32_t),
                           uint32_t (*write_handler)(uint32_t, const uint8_t *, uint32_t)) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            device_manager.devices[i].device_open = open_handler;
            device_manager.devices[i].device_close = close_handler;
            device_manager.devices[i].device_read = read_handler;
            device_manager.devices[i].device_write = write_handler;
            return 1;
        }
    }
    return 0;
}

uint32_t device_enable(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            device_manager.devices[i].status = 1;
            return 1;
        }
    }
    return 0;
}

uint32_t device_disable(uint32_t device_id) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_id == device_id) {
            device_manager.devices[i].status = 0;
            return 1;
        }
    }
    return 0;
}

uint32_t get_device_count(void) {
    return device_manager.device_count;
}

uint32_t find_device_by_type(uint8_t device_type) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].device_type == device_type) {
            return device_manager.devices[i].device_id;
        }
    }
    return 0;
}

uint32_t find_device_by_major(uint32_t major) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        if (device_manager.devices[i].major_number == major) {
            return device_manager.devices[i].device_id;
        }
    }
    return 0;
}

uint32_t find_device_by_name(const char *name) {
    for (uint32_t i = 0; i < device_manager.device_count; i++) {
        uint32_t match = 1;
        for (uint32_t j = 0; j < 128; j++) {
            if (device_manager.devices[i].device_name[j] != name[j]) {
                match = 0;
                break;
            }
            if (name[j] == 0) break;
        }
        if (match) {
            return device_manager.devices[i].device_id;
        }
    }
    return 0;
}
