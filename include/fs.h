#ifndef _FS_H_
#define _FS_H_

#include "types.h"

#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 4096

typedef struct {
    char name[MAX_FILENAME];
    uint32_t size;
    uint8_t data[MAX_FILE_SIZE];
    bool is_directory;
} vfs_node_t;

void vfs_init(void);
vfs_node_t *vfs_find(const char *name);
bool vfs_create(const char *name, const char *content);
bool vfs_write(const char *name, const void *data, uint32_t size);
int  vfs_list(char names[MAX_FILES][MAX_FILENAME]);

#endif
