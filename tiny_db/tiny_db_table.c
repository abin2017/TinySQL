/*
    sizeof(tbl_node_t) + [ (sizeof(tbl_item_t) + tbl_item_t.item_title_len) + ... (tbl_node_t.item_count) ] + 表名 + [项名，项名 ...]
    */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "list.h"
#include "tiny_db_platform.h"
#include "tiny_db_priv.h"
#include "tiny_db_pager.h"
#include "tiny_db_module.h"
#include "tiny_db_node.h"
#include "tiny_db_table.h"


#define TABLE_ALIAGN        16
#define TABLE_FIRST_ADDR    16
#define TABLE_ITEM_INVALID   0xFF

#define TABLE_ATTR_MASK_IS_INT      (1 << 0)  //1 is int; 0 is string
#define TABLE_ATTR_MASK_LN_NODEID   (1 << 1)  //1 take node id; 0 do nothing

#define TABLE_MANAGE_NODE_LEN 0x80

typedef struct{
    td_uchar idx;
    td_uchar attr_mask;
    td_uint16 title_len : 6;
    td_uint16 title_off : 10;
}tbl_item_t;

typedef struct{
    td_uint16 module_id;
    td_uint16 node_len;
    td_uint16 first_page_id;
    td_uint16 last_node_id;
    td_uint32 item_count : 8;
    td_uint32 title_len : 6;
    td_uint32 title_off : 10;
    td_uint32 rev : 8;
}tbl_node_t;


static td_int32 _td_table_manage_parse(td_int32 fd, mod_node_t *p_table_mode, used_node_t *p_used, list_head_t *p_list_head){
    char table_buffer[512] = {0};
    tbl_node_t *p_tblm_serialize = NULL;
    tbl_item_t *p_tbit_serialize = NULL;

    tbl_desc_t      *p_des  = NULL;
    mod_node_t      *p_mnod = NULL; 
    td_mod_info_t   *p_mod  = NULL;
    td_int32 offset = 0, index = 0;

    tiny_db_assert(tiny_db_node_get_by_pos(fd, p_table_mode, p_used, table_buffer, 512) == TR_SUCCESS);

    p_tblm_serialize = (tbl_node_t *)table_buffer;
    p_tbit_serialize = (tbl_item_t *)((char *)table_buffer + sizeof(tbl_node_t));

    if(p_tblm_serialize->item_count == 0 || p_tblm_serialize->title_len == 0){
        TINY_DB_ERR("table header err\n");
        return TR_FAIL;
    }

    p_des = tiny_db_malloc(sizeof(tbl_desc_t));
    memset(p_des, 0, sizeof(tbl_desc_t));

    list_add_tail(&p_des->list, p_list_head);

    //解析表头信息
    offset = p_tblm_serialize->title_len;
    p_des->title = tiny_db_strdup_fix(&table_buffer[offset], p_tblm_serialize->title_len);
    tiny_db_assert(p_des->title != NULL);

    p_des->head_cnt = p_tblm_serialize->item_count;
    p_des->p_head = tiny_db_malloc(sizeof(tbl_head_t) * p_des->head_cnt);
    memset(p_des->p_head, 0, sizeof(tbl_head_t) * p_des->head_cnt);

    p_mnod = &p_des->node;
    p_mnod->node_length = p_tblm_serialize->node_len;
    p_mnod->last_node_id = p_tblm_serialize->last_node_id;

    p_mod = &p_mnod->module;
    p_mod->module_id = p_tblm_serialize->module_id;
    p_mod->first_page_id = p_tblm_serialize->first_page_id;

    tiny_db_assert(tiny_db_node_init(fd, p_mnod) == TR_SUCCESS);

    //解析表项信息
    for(index = 0; index < p_des->head_cnt; index++){
        tbl_head_t *p_header = &p_des->p_head[index];

        p_header->idx = p_tbit_serialize->idx;
        p_header->attr_mask = p_tbit_serialize->attr_mask;

        tiny_db_assert(p_tbit_serialize->title_len == 0);
        offset = p_tbit_serialize->title_off;
        p_header->title = tiny_db_strdup_fix(&table_buffer[offset], p_tbit_serialize->title_len);
        tiny_db_assert(p_header->title != NULL);
    }

    return TR_SUCCESS;
}

#if 0
static td_int32 _td_table_manage_serialize(, ){

}
#endif

td_int32 tiny_db_table_init(td_int32 fd, tbl_manage_t *p_this){
    td_int32  ret = TR_FAIL;

    mod_node_t      *   p_node      = NULL;
    td_mod_info_t   *   p_mod       = NULL;
    td_uchar            s_buf[TINY_REV_BYTES_LEN] = {0};

    memset(p_this, 0, sizeof(tbl_manage_t));
    list_init(&p_this->list_head);

    /*                          TABLE管理页面开始初始化                          */
    ret = tiny_db_pager_init(fd);
    tiny_db_assert(TR_SUCCESS == ret);

    p_node = tiny_db_malloc(sizeof(mod_node_t));
    memset(p_node, 0, sizeof(mod_node_t));

    //init table first page
    p_mod = &p_node->module;
    p_mod->module_id = TABLE_INDEX_MODULE_ID;
    p_node->node_length = TABLE_MANAGE_NODE_LEN;

    tiny_db_pager_get_rev(fd, TD_PAGER_REV_TBL_LAST_NODE, s_buf);
    p_node->last_node_id = TD_MAKE_DWORD(s_buf);

    ret = tiny_db_node_init(fd, p_node);
    tiny_db_assert(TR_SUCCESS == ret);

    p_this->p_table = p_node;
    /*                          TABLE管理页面初始化完成                          */
    /*                           TABLE用户表开始初始化                           */
    if(!list_empty(&p_node->list_head)){
        list_head_t *p_head = NULL;
        list_head_t *p_tmp  = NULL;
        used_node_t *p_rec = NULL;
        list_for_each_safe(p_head, p_tmp, &p_node->list_head){
            p_rec = list_entry(p_head, used_node_t, list);

            if(NULL != p_rec){
                tiny_db_assert(_td_table_manage_parse(fd, p_node, p_rec, &p_this->list_head) == TR_SUCCESS);
            }
        }
    }
    /*                           TABLE用户表完成初始化                           */

    return ret;
}