#include "kernel.h"

#define MAX_OPEN_FILES 1024
#define MAX_INODES 4096
#define MAX_BLOCKS 65536
#define BLOCK_SIZE 4096
#define INODE_SIZE 256

#define FILE_TYPE_REGULAR 0x01
#define FILE_TYPE_DIRECTORY 0x02
#define FILE_TYPE_SYMLINK 0x04
#define FILE_TYPE_BLOCK_DEVICE 0x08
#define FILE_TYPE_CHAR_DEVICE 0x10

#define INODE_PERMISSION_OWNER_READ 0x100
#define INODE_PERMISSION_OWNER_WRITE 0x080
#define INODE_PERMISSION_OWNER_EXEC 0x040
#define INODE_PERMISSION_GROUP_READ 0x020
#define INODE_PERMISSION_GROUP_WRITE 0x010
#define INODE_PERMISSION_GROUP_EXEC 0x008
#define INODE_PERMISSION_OTHER_READ 0x004
#define INODE_PERMISSION_OTHER_WRITE 0x002
#define INODE_PERMISSION_OTHER_EXEC 0x001

typedef struct {
    uint32_t inode_number;
    uint8_t file_type;
    uint16_t permissions;
    uint32_t size;
    uint32_t owner_uid;
    uint32_t owner_gid;
    uint32_t created_time;
    uint32_t modified_time;
    uint32_t accessed_time;
    uint32_t hard_links;
    uint32_t block_ptrs[12];
    uint32_t indirect_block_ptr;
    uint32_t double_indirect_ptr;
    uint32_t triple_indirect_ptr;
} inode_t;

typedef struct {
    uint32_t inode_number;
    char filename[256];
    uint8_t file_type;
} directory_entry_t;

typedef struct {
    uint32_t block_number;
    uint8_t data[BLOCK_SIZE];
    uint8_t dirty;
    uint32_t ref_count;
} block_t;

typedef struct {
    uint32_t fd;
    uint32_t inode_number;
    uint32_t file_position;
    uint8_t file_type;
    uint8_t access_mode;
    uint8_t is_open;
} file_descriptor_t;

typedef struct {
    char superblock[512];
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t creation_time;
    uint32_t mount_time;
    uint32_t last_write_time;
    uint32_t mount_count;
} superblock_t;

typedef struct {
    superblock_t superblock;
    inode_t inode_table[MAX_INODES];
    block_t block_cache[256];
    file_descriptor_t file_descriptors[MAX_OPEN_FILES];
    uint32_t next_inode_number;
    uint32_t next_block_number;
    uint32_t next_fd;
    uint32_t cache_hits;
    uint32_t cache_misses;
} filesystem_t;

static filesystem_t filesystem = {0};

void filesystem_init(void) {
    filesystem.superblock.total_blocks = MAX_BLOCKS;
    filesystem.superblock.total_inodes = MAX_INODES;
    filesystem.superblock.free_blocks = MAX_BLOCKS - 100;
    filesystem.superblock.free_inodes = MAX_INODES - 10;
    filesystem.superblock.block_size = BLOCK_SIZE;
    filesystem.superblock.inode_size = INODE_SIZE;
    filesystem.superblock.creation_time = 0;
    filesystem.superblock.mount_time = 0;
    filesystem.superblock.mount_count = 1;
    
    filesystem.next_inode_number = 1;
    filesystem.next_block_number = 100;
    filesystem.next_fd = 3;
    filesystem.cache_hits = 0;
    filesystem.cache_misses = 0;
    
    inode_t root_inode;
    root_inode.inode_number = 0;
    root_inode.file_type = FILE_TYPE_DIRECTORY;
    root_inode.permissions = 0755;
    root_inode.size = 0;
    root_inode.owner_uid = 0;
    root_inode.owner_gid = 0;
    filesystem.inode_table[0] = root_inode;
}

uint32_t fs_create_file(const char *filename, uint8_t file_type, uint16_t permissions) {
    if (filesystem.superblock.free_inodes == 0) return 0;
    
    inode_t new_inode;
    new_inode.inode_number = filesystem.next_inode_number++;
    new_inode.file_type = file_type;
    new_inode.permissions = permissions;
    new_inode.size = 0;
    new_inode.owner_uid = 0;
    new_inode.owner_gid = 0;
    new_inode.created_time = 0;
    new_inode.hard_links = 1;
    
    filesystem.inode_table[new_inode.inode_number] = new_inode;
    filesystem.superblock.free_inodes--;
    
    return new_inode.inode_number;
}

uint32_t fs_delete_file(uint32_t inode_number) {
    if (inode_number >= MAX_INODES) return 0;
    
    inode_t *inode = &filesystem.inode_table[inode_number];
    if (inode->hard_links > 0) {
        inode->hard_links--;
    }
    
    if (inode->hard_links == 0) {
        for (int i = 0; i < 12; i++) {
            if (inode->block_ptrs[i] != 0) {
                filesystem.superblock.free_blocks++;
            }
        }
        filesystem.superblock.free_inodes++;
    }
    
    return 1;
}

uint32_t fs_open_file(uint32_t inode_number, uint8_t access_mode) {
    if (filesystem.next_fd >= MAX_OPEN_FILES) return 0;
    if (inode_number >= MAX_INODES) return 0;
    
    file_descriptor_t *fd = &filesystem.file_descriptors[filesystem.next_fd];
    fd->fd = filesystem.next_fd;
    fd->inode_number = inode_number;
    fd->file_position = 0;
    fd->file_type = filesystem.inode_table[inode_number].file_type;
    fd->access_mode = access_mode;
    fd->is_open = 1;
    
    return filesystem.next_fd++;
}

uint32_t fs_close_file(uint32_t fd) {
    if (fd >= MAX_OPEN_FILES) return 0;
    
    filesystem.file_descriptors[fd].is_open = 0;
    return 1;
}

uint32_t fs_read_file(uint32_t fd, uint8_t *buffer, uint32_t size) {
    if (fd >= MAX_OPEN_FILES) return 0;
    if (!filesystem.file_descriptors[fd].is_open) return 0;
    
    file_descriptor_t *file = &filesystem.file_descriptors[fd];
    inode_t *inode = &filesystem.inode_table[file->inode_number];
    
    uint32_t bytes_to_read = (file->file_position + size > inode->size) ? 
                             (inode->size - file->file_position) : size;
    
    file->file_position += bytes_to_read;
    return bytes_to_read;
}

uint32_t fs_write_file(uint32_t fd, const uint8_t *buffer, uint32_t size) {
    if (fd >= MAX_OPEN_FILES) return 0;
    if (!filesystem.file_descriptors[fd].is_open) return 0;
    
    file_descriptor_t *file = &filesystem.file_descriptors[fd];
    inode_t *inode = &filesystem.inode_table[file->inode_number];
    
    if (inode->size + size > MAX_BLOCKS * BLOCK_SIZE) {
        return 0;
    }
    
    uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (filesystem.superblock.free_blocks < blocks_needed) {
        return 0;
    }
    
    inode->size += size;
    file->file_position += size;
    filesystem.superblock.free_blocks -= blocks_needed;
    
    return size;
}

uint32_t fs_seek_file(uint32_t fd, uint32_t position) {
    if (fd >= MAX_OPEN_FILES) return 0;
    if (!filesystem.file_descriptors[fd].is_open) return 0;
    
    file_descriptor_t *file = &filesystem.file_descriptors[fd];
    inode_t *inode = &filesystem.inode_table[file->inode_number];
    
    if (position > inode->size) {
        return 0;
    }
    
    file->file_position = position;
    return position;
}

uint32_t fs_get_file_size(uint32_t inode_number) {
    if (inode_number >= MAX_INODES) return 0;
    return filesystem.inode_table[inode_number].size;
}

uint32_t fs_allocate_block(void) {
    if (filesystem.superblock.free_blocks == 0) return 0;
    
    uint32_t block_number = filesystem.next_block_number++;
    filesystem.superblock.free_blocks--;
    
    return block_number;
}

void fs_free_block(uint32_t block_number) {
    filesystem.superblock.free_blocks++;
}

uint32_t fs_get_filesystem_stats(void *stats_buffer) {
    uint32_t *stats = (uint32_t *)stats_buffer;
    stats[0] = filesystem.superblock.total_blocks;
    stats[1] = filesystem.superblock.total_inodes;
    stats[2] = filesystem.superblock.free_blocks;
    stats[3] = filesystem.superblock.free_inodes;
    return 16;
}

uint32_t fs_get_inode_info(uint32_t inode_number, void *info) {
    if (inode_number >= MAX_INODES) return 0;
    
    inode_t *inode_info = (inode_t *)info;
    inode_info[0] = filesystem.inode_table[inode_number];
    
    return sizeof(inode_t);
}

uint32_t fs_change_permissions(uint32_t inode_number, uint16_t permissions) {
    if (inode_number >= MAX_INODES) return 0;
    
    filesystem.inode_table[inode_number].permissions = permissions;
    return 1;
}

uint32_t fs_get_block_data(uint32_t block_number, uint8_t *buffer) {
    for (int i = 0; i < 256; i++) {
        if (filesystem.block_cache[i].block_number == block_number) {
            filesystem.cache_hits++;
            for (int j = 0; j < BLOCK_SIZE; j++) {
                buffer[j] = filesystem.block_cache[i].data[j];
            }
            return BLOCK_SIZE;
        }
    }
    filesystem.cache_misses++;
    return 0;
}

void fs_set_block_data(uint32_t block_number, const uint8_t *buffer) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < 256; j++) {
            if (filesystem.block_cache[j].block_number == 0) {
                filesystem.block_cache[j].block_number = block_number;
                filesystem.block_cache[j].dirty = 1;
                filesystem.block_cache[j].data[i] = buffer[i];
                break;
            }
        }
    }
}

uint32_t fs_format_filesystem(void) {
    filesystem.superblock.mount_count = 0;
    filesystem.superblock.free_blocks = MAX_BLOCKS - 100;
    filesystem.superblock.free_inodes = MAX_INODES - 10;
    filesystem.next_inode_number = 1;
    filesystem.next_block_number = 100;
    filesystem.next_fd = 3;
    
    return 1;
}
