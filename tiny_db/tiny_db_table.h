


#ifndef _TINY_DB_TABLE
#define _TINY_DB_TABLE


typedef struct{
    td_uchar    idx;
    td_uchar    attr_mask;
    td_uint16   fix_length;
    td_char *   title;
}tbl_head_t;

typedef struct{
    td_uchar        head_cnt;
    td_uchar        rev;
    td_uint16       node_id;
    tbl_head_t  *   p_head;
    td_char     *   title;
    mod_node_t      node;
    list_head_t     list;
}tbl_desc_t;

typedef struct{
    mod_node_t  *   p_table;
    td_char     *   buffer;
    list_head_t     list_head;
}tbl_manage_t;

/*
    sizeof(tbl_node_t) + [ (sizeof(tbl_item_t) + tbl_item_t.item_title_len) + ... (tbl_node_t.item_count) ]
    */

td_int32    tiny_db_table_init(td_int32 fd, tbl_manage_t *p_this);

td_int32    tiny_db_table_deinit(td_int32 fd, tbl_manage_t *p_this);

td_int32    tiny_db_table_create(td_int32 fd, tbl_manage_t *p_this, char *title, td_elem_list_t *p_column);

td_int32    tiny_db_table_destroy(td_int32 fd, tbl_manage_t *p_this, char *title);

td_int32    tiny_db_table_insert(td_int32 fd, tbl_manage_t *p_this, char *title, td_elem_t *p_elements, int count);

#endif
