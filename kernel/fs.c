#include "fs.h"

static vfs_node_t files[MAX_FILES];
static int file_count = 0;

void vfs_init(void) {
    memset(files, 0, sizeof(files));
    file_count = 0;

    vfs_create("/docs/readme.txt",
        "=========================================\n"
        "           LOHITHOS - BARE METAL         \n"
        "=========================================\n"
        "Welcome to LohithOS!\n"
        "A genuine lightweight 32-bit x86 custom\n"
        "operating system engineered from scratch.\n"
        "- Kernel: Freestanding Monolithic (C & ASM)\n"
        "- Graphics: VBE Linear Framebuffer 1024x768\n"
        "- Network: Intel PRO/1000 MT (e1000) NIC\n"
        "- Protocols: Ethernet, ARP, IPv4, ICMP, UDP\n"
        "- Games: Snake, Snakes & Ladders, Speed Car\n"
        "- Academic Project by Lohith / Rishika V.\n");

    vfs_create("/docs/features.txt",
        "- Double-buffered 32-bit composited GUI\n"
        "- Interactive Desktop with Windows & Dock\n"
        "- Full PS/2 Mouse & Keyboard Drivers\n"
        "- Intel e1000 Network Card & Ping Diagnostics\n"
        "- In-memory Hierarchical Virtual Filesystem\n"
        "- Boundary-tag Dynamic Kernel Heap Manager\n"
        "- Built-in Retro Arcade Games & Utilities\n");

    vfs_create("/system/os_info.txt",
        "OS_NAME=LohithOS\n"
        "VERSION=1.0-RELEASE\n"
        "ARCH=i686-elf-freestanding\n"
        "TARGET=Oracle VirtualBox / Bare-Metal PC\n"
        "COMPILER=LLVM Clang 23.1.5 Freestanding\n");

    vfs_create("/system/network.cfg",
        "INTERFACE=eth0 (Intel 82540EM PRO/1000)\n"
        "IP=10.0.2.15\n"
        "GATEWAY=10.0.2.2\n"
        "SUBNET=255.255.255.0\n"
        "DNS=10.0.2.3\n"
        "WIFI_MODE=Host Wi-Fi Bridged NAT\n");

    vfs_create("/games/rules.txt",
        "1. Snake Food Bite: Arrow keys to steer, eat apples!\n"
        "2. Snakes & Ladders: Roll dice, climb ladders, avoid snakes!\n"
        "3. Speed Car: Arrow keys Left/Right to dodge traffic!\n");
}

vfs_node_t *vfs_find(const char *name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return NULL;
}

bool vfs_create(const char *name, const char *content) {
    if (file_count >= MAX_FILES) return false;

    vfs_node_t *node = &files[file_count++];
    strncpy(node->name, name, MAX_FILENAME - 1);
    node->size = strlen(content);
    if (node->size > MAX_FILE_SIZE - 1) node->size = MAX_FILE_SIZE - 1;
    memcpy(node->data, content, node->size);
    node->data[node->size] = '\0';
    node->is_directory = false;
    return true;
}

bool vfs_write(const char *name, const void *data, uint32_t size) {
    vfs_node_t *node = vfs_find(name);
    if (!node) {
        if (file_count >= MAX_FILES) return false;
        node = &files[file_count++];
        strncpy(node->name, name, MAX_FILENAME - 1);
        node->is_directory = false;
    }
    if (size > MAX_FILE_SIZE) size = MAX_FILE_SIZE;
    memcpy(node->data, data, size);
    node->size = size;
    return true;
}

int vfs_list(char names[MAX_FILES][MAX_FILENAME]) {
    for (int i = 0; i < file_count; i++) {
        strcpy(names[i], files[i].name);
    }
    return file_count;
}
