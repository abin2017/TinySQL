


#ifndef _TINY_DB_TABLE
#define _TINY_DB_TABLE

#define TABLE_ALIAGN        16
#define TABLE_FIRST_ADDR    16
#define TABLE_ITEM_INVALID   0xFF

#define TABLE_START_TAG "^..^"

typedef struct{
    td_uint8 item_idx;
    td_uint8 item_type;
    td_uint8 item_title_len;
}tbl_item_t;

typedef struct{
    td_uint16 module_id;
    td_uint16 node_len;
    td_uint16 start_page_idx;
    td_uint16 rev;
    td_uint32 item_count : 8;
    td_uint32 node_index : 24;
}tbl_node_t;


/*
    TABLE_START_TAG + sizeof(tbl_node_t) + [ (sizeof(tbl_item_t) + tbl_item_t.item_title_len) + ... (tbl_node_t.item_count) ]
    */

td_int32    tiny_db_table_init();

#endif
