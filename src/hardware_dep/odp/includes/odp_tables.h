#ifndef DPDK_TABLES_H
#define DPDK_TABLES_H

#include <table.h>

typedef struct extended_table_s {
    void*     odp_table;
    uint32_t   size;
    uint8_t** content;
} extended_table_t;

//=============================================================================
// Table size limits

/* 32-bit has less address-space for hugepage memory, limit to 1M entries */
#define LPM_MAX_RULES         1024
#define LPM6_NUMBER_TBL8S (1 << 16)

#define TABLE_MAX 2

#define TABLE_SIZE 2000 //Number of elements table may store
//define different key and value size for different tables.
#define TABLE_KEY_SIZE 4 //key_size    fixed size of the 'key' in bytes.
#define TABLE_VALUE_SIZE 4 //value_size  fixed size of the 'value' in bytes.

int odpc_lookup_tbls_init();
int odpc_lookup_tbls_des();

#endif
