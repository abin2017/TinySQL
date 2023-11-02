


#ifndef _TINY_DB_TABLE
#define _TINY_DB_TABLE


typedef struct{
    td_uchar    idx;
    td_uchar    attr_mask;
    td_uint16   rev;
    td_char *   p_item;
}tbl_head_t;

typedef struct{
    tbl_head_t      head;
    mod_node_t      node;
    list_head_t     list;
}tbl_desc_t;

typedef struct{
    mod_node_t  *   p_table;
    list_head_t     list_head;
}tbl_manage_t;

/*
    TABLE_START_TAG + sizeof(tbl_node_t) + [ (sizeof(tbl_item_t) + tbl_item_t.item_title_len) + ... (tbl_node_t.item_count) ]
    */

td_int32    tiny_db_table_init(td_int32 fd, tbl_manage_t *p_this);


#endif
