/*
    TABLE_START_TAG + sizeof(tbl_node_t) + [ (sizeof(tbl_item_t) + tbl_item_t.item_title_len) + ... (tbl_node_t.item_count) ]
    */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "list.h"
#include "sql_porting.h"
#include "tiny_db_priv.h"
#include "tiny_db_pager.h"
#include "tiny_db_module.h"
#include "tiny_db_node.h"
#include "tiny_db_table.h"


#define TABLE_ALIAGN        16
#define TABLE_FIRST_ADDR    16
#define TABLE_ITEM_INVALID   0xFF

#define TABLE_START_TAG "^..^"

#define TABLE_ATTR_MASK_IS_INT      (1 << 0)  //1 is int; 0 is string
#define TABLE_ATTR_MASK_LN_NODEID   (1 << 1)  //1 take node id; 0 do nothing

#define TABLE_MANAGE_NODE_LEN 0xFF

typedef struct{
    td_uchar idx;
    td_uchar attr_mask;
    td_uint16 title_len : 6;
    td_uint16 title_off : 10;
}tbl_item_t;

typedef struct{
    td_uint16 module_id;
    td_uint16 node_len;
    td_uint16 start_page_idx;
    td_uint16 node_index;
    td_uint32 item_count : 8;
    td_uint32 title_len : 6;
    td_uint32 title_off : 10;
    td_uint32 rev : 8;
}tbl_node_t;


td_int32 tiny_db_table_init(td_int32 fd, tbl_manage_t *p_this){
    td_int32  ret = TR_FAIL, val = 0;

    tbl_desc_t      *   p_tbl       = NULL;
    mod_node_t      *   p_node      = NULL;
    td_mod_info_t   *   p_mod       = NULL;
    td_uchar            s_buf[TINY_REV_BYTES_LEN] = {0};

    memset(p_this, 0, sizeof(tbl_manage_t));
    list_init(&p_this->list_head);

    /*                          TABLE管理页面开始初始化                          */
    ret = tiny_db_pager_init(fd);
    sql_tiny_db_assert(TR_SUCCESS == ret);

    p_node = sql_tiny_db_malloc(sizeof(mod_node_t));
    memset(p_node, 0, sizeof(mod_node_t));

    //init table first page
    p_mod = &p_node->module;
    p_mod->module_id = TABLE_INDEX_MODULE_ID;
    p_node->node_length = TABLE_MANAGE_NODE_LEN;

    tiny_db_pager_get_rev(fd, TD_PAGER_REV_TBL_LAST_NODE, s_buf);
    p_node->last_node_id = TD_MAKE_DWORD(s_buf);

    ret = tiny_db_node_init(fd, p_node);
    sql_tiny_db_assert(TR_SUCCESS == ret);

    p_this->p_table = p_node;
    /*                          TABLE管理页面初始化完成                          */


    

    return ret;
}