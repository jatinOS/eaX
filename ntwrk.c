#include "../kernel.h"

typedef struct {
    uint32_t ipv4;
    uint8_t ipv6[16];
    uint8_t mac_address[6];
    uint32_t mtu;
    uint8_t status;
    uint8_t duplex;
    uint32_t speed;
} network_interface_t;

typedef struct {
    uint32_t interface_count;
    network_interface_t interfaces[4];
    uint32_t active_interface;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t packets_dropped;
} network_stack_t;

static network_stack_t network_stack = {0};

void network_init(void) {
    network_stack.interface_count = 2;
    
    network_stack.interfaces[0].ipv4 = 0xC0A80001;
    network_stack.interfaces[0].mac_address[0] = 0x52;
    network_stack.interfaces[0].mac_address[1] = 0x54;
    network_stack.interfaces[0].mac_address[2] = 0x00;
    network_stack.interfaces[0].mac_address[3] = 0x12;
    network_stack.interfaces[0].mac_address[4] = 0x34;
    network_stack.interfaces[0].mac_address[5] = 0x56;
    network_stack.interfaces[0].mtu = 1500;
    network_stack.interfaces[0].status = 1;
    network_stack.interfaces[0].duplex = 1;
    network_stack.interfaces[0].speed = 1000;
    
    network_stack.interfaces[1].mtu = 1500;
    network_stack.interfaces[1].status = 0;
    network_stack.interfaces[1].duplex = 1;
    network_stack.interfaces[1].speed = 100;
    
    network_stack.active_interface = 0;
    network_stack.packets_sent = 0;
    network_stack.packets_received = 0;
    network_stack.packets_dropped = 0;
}

uint32_t network_send_packet(uint8_t *buffer, uint32_t length) {
    if (network_stack.interfaces[network_stack.active_interface].status == 0) return 0;
    
    network_stack.packets_sent++;
    return length;
}

uint32_t network_receive_packet(uint8_t *buffer, uint32_t max_length) {
    if (network_stack.interfaces[network_stack.active_interface].status == 0) return 0;
    
    network_stack.packets_received++;
    return 0;
}

uint32_t network_get_interface_info(uint32_t interface, void *info) {
    if (interface >= network_stack.interface_count) return 0;
    
    network_interface_t *iface_info = (network_interface_t *)info;
    iface_info[0] = network_stack.interfaces[interface];
    
    return sizeof(network_interface_t);
}

void network_set_interface_ip(uint32_t interface, uint32_t ipv4) {
    if (interface < network_stack.interface_count) {
        network_stack.interfaces[interface].ipv4 = ipv4;
    }
}

void network_enable_interface(uint32_t interface) {
    if (interface < network_stack.interface_count) {
        network_stack.interfaces[interface].status = 1;
    }
}

void network_disable_interface(uint32_t interface) {
    if (interface < network_stack.interface_count) {
        network_stack.interfaces[interface].status = 0;
    }
}

uint32_t network_get_stats(void *stats_buffer) {
    uint32_t *stats = (uint32_t *)stats_buffer;
    stats[0] = network_stack.packets_sent;
    stats[1] = network_stack.packets_received;
    stats[2] = network_stack.packets_dropped;
    return 12;
}
