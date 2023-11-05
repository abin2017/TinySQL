/*
    sizeof(tbl_node_t) + [ (sizeof(tbl_item_t) + tbl_item_t.item_title_len) + ... (tbl_node_t.item_count) ] + 表名 + [项名，项名 ...]
    */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "list.h"
#include "tiny_db_platform.h"
#include "tiny_db_api.h"
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
#define TABLE_ATTR_MASK_FIX_LEN     (1 << 2)  //1 string fix length; 0 dynamic

#define TABLE_MANAGE_NODE_LEN 0x80

#define MAX_BUFFER_LEN 1884

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

    TD_TRUE_RETVAL(p_tblm_serialize->item_count == 0 || p_tblm_serialize->title_len == 0, TR_FAIL, "table header err\n");

    p_des = tiny_db_malloc(sizeof(tbl_desc_t));
    memset(p_des, 0, sizeof(tbl_desc_t));

    list_add_tail(&p_des->list, p_list_head);

    p_des->node_id = p_used->node_id;

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

static tbl_desc_t * _td_table_get_desinfo(td_int32 fd, tbl_manage_t *p_this, char *title){
    
    if(!list_empty(&p_this->list_head)){
        list_head_t *   p_head = NULL;
        list_head_t *   p_tmp  = NULL;
        tbl_desc_t  *   p_rec = NULL;

        list_for_each_safe(p_head, p_tmp, &p_this->list_head){
            p_rec = list_entry(p_head, tbl_desc_t, list);

            if(p_rec == NULL || strcmp(p_rec->title, title) != 0){
                continue;
            }

            return p_rec;
        }
    }

    return NULL;
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
    TD_TRUE_RETVAL(TR_SUCCESS != ret, TR_FAIL, "database error\n");

    p_node = tiny_db_malloc(sizeof(mod_node_t));
    memset(p_node, 0, sizeof(mod_node_t));

    p_this->buffer = tiny_db_malloc(MAX_BUFFER_LEN);
    p_this->buffer[0] = 0;

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

td_int32 tiny_db_table_deinit(td_int32 fd, tbl_manage_t *p_this){
    if(NULL == p_this->p_table){
        return TR_FAIL;
    }

    if(!list_empty(&p_this->list_head)){
        list_head_t *   p_head = NULL;
        list_head_t *   p_tmp  = NULL;
        tbl_desc_t  *   p_rec = NULL;
        int             i = 0;

        list_for_each_safe(p_head, p_tmp, &p_this->list_head){
            p_rec = list_entry(p_head, tbl_desc_t, list);

            if(NULL == p_rec){
                continue;
            }

            list_del(&p_rec->list);
            tiny_db_free(p_rec->title);
            for(i = 0; i < p_rec->head_cnt; i ++){
                tiny_db_free(p_rec->p_head[i].title);
            }
            tiny_db_free(p_rec->p_head);
            tiny_db_node_deinit(fd, &p_rec->node);
            tiny_db_free(p_rec);
        }
    }

    tiny_db_node_deinit(fd, p_this->p_table);

    if(p_this->p_table){
        tiny_db_free(p_this->p_table);
    }

    memset(p_this, 0, sizeof(tbl_manage_t));

    return TR_SUCCESS;
}

td_int32    tiny_db_table_create(td_int32 fd, tbl_manage_t *p_this, char *title, td_elem_list_t *p_column){
    td_int32        new_module_id = 0, length = 0, i = 0, val = 0;
    tbl_node_t  *   p_node = NULL;
    tbl_item_t  *   p_item = NULL;

    tbl_desc_t  *   p_des  = NULL;

    td_uint16       esti_len = 0, already_has = 0;
    td_uchar        s_buf[4] = {0};
    st_data_cpy_t   data;

    if(_td_table_get_desinfo(fd, p_this, title) != NULL){
        TINY_DB_ERR("table %s already exsit\n", title);
        return TR_FAIL;
    }

    length = sizeof(tbl_item_t) * p_column->count + sizeof(tbl_node_t);
    p_node = (tbl_node_t *)p_this->buffer;
    p_item = (tbl_item_t *)(p_this->buffer + sizeof(tbl_node_t));

    memset(p_this->buffer, 0, length);
    memset(&data, 0, sizeof(st_data_cpy_t));

    data.buffer = p_this->buffer;
    data.buffer_length = MAX_BUFFER_LEN;
    data.buffer_used = length;

    new_module_id = tiny_db_module_require_id(fd);

    p_node->module_id = new_module_id;
    //p_node->first_page_id
    p_node->last_node_id = 0;
    p_node->item_count = p_column->count;
    p_node->title_len = strlen(title);
    p_node->title_off = length;

    length = strlen(title);
    TD_TRUE_RETVAL(tiny_db_copy_block(&data, title, length) != length, TR_FAIL, "copy length fail %d, %s\n", data.buffer_used, title);

    p_des = tiny_db_malloc(sizeof(tbl_desc_t));
    TD_TRUE_RETVAL(NULL == p_des, TR_FAIL, "no memory\n");
    memset(p_des, 0, sizeof(tbl_desc_t));

    p_des->title = tiny_db_strdup(title);
    TD_TRUE_JUMP(NULL == p_des->title, _err, "no memory\n");

    p_des->p_head = tiny_db_malloc(sizeof(tbl_head_t) * p_column->count);
    TD_TRUE_JUMP(NULL == p_des->p_head, _err, "no memory\n");
    memset(p_des->p_head, 0, sizeof(tbl_head_t) * p_column->count);
    p_des->head_cnt = p_column->count;

    for(i = 0; i < p_column->count; i ++){
        td_elem_t * p_elem = &p_column->p_elem[i];

        p_item[i].idx = i;
        p_item[i].title_off = data.buffer_used;
        p_item[i].title_len = strlen(p_elem->p_tag);

        TD_TRUE_JUMP(tiny_db_copy_block(&data, p_elem->p_tag, p_item[i].title_len) != p_item[i].title_len, _err, "copy length fail %d, %s\n", data.buffer_used, p_elem->p_tag);

        switch(p_elem->type){
            case TD_ELEM_TYPE_STRING:
                esti_len += 20;
                p_item[i].attr_mask = 0b11111000;
                break;

            case TD_ELEM_TYPE_STRING_FIXED:
                esti_len += (1 + 2 + *(int *)p_elem->content);
                p_item[i].attr_mask = 0b11111100;
                break;
            
            case TD_ELEM_TYPE_INTEGER:
                esti_len += 5;
                p_item[i].attr_mask = 0b11111101;
                break;
            
            case TD_ELEM_TYPE_AUTO_INCREASE:
                if(already_has){
                    TINY_DB_ERR("already set auto\n");
                    goto _err;
                }else{
                    already_has = 1;
                    p_item[i].attr_mask = 0b11111111;
                }
                break;

                default:
                    TINY_DB_ERR("unknow type %d\n", p_elem->type);
                    goto _err;
        }

        p_des->p_head[i].attr_mask = p_item[i].attr_mask;
        p_des->p_head[i].idx = p_item[i].idx;
        p_des->p_head[i].title = tiny_db_strdup(p_elem->p_tag);
        TD_TRUE_JUMP(NULL == p_des->p_head[i].title, _err, "strdup %s fail\n", p_elem->p_tag);
    }

    p_node->node_len = esti_len;
    if(esti_len >= 255){
        p_node->node_len = 255;
    }

    p_des->node.last_node_id = p_node->last_node_id;
    p_des->node.node_length = p_node->node_len;
    p_des->node.module.module_id = new_module_id;
    p_des->node.module.first_page_id = TINY_INVALID_PAGE_ID;

    TD_TRUE_JUMP(TR_SUCCESS != tiny_db_node_init(fd, &p_des->node), _err, "node init error\n");

    p_node->first_page_id = p_des->node.module.first_page_id;
    TD_TRUE_JUMP(TR_SUCCESS != tiny_db_node_set(fd, p_this->p_table, p_this->buffer, data.buffer_used), _err, "table create write node fail\n");
    val = p_this->p_table->last_node_id;

    p_des->node_id = val;
    TD_DWORD_SERIALIZE(s_buf, val);
    tiny_db_pager_set_rev(fd, TD_PAGER_REV_TBL_LAST_NODE, s_buf);
    list_add_tail(&p_des->list, &p_this->list_head);

    return TR_SUCCESS;

    _err:
        if(p_des){
            if(p_des->p_head){
                for(i = 0; i < p_column->count; i ++){
                    if(p_des->p_head[i].title){
                        tiny_db_free(p_des->p_head[i].title);
                    }
                }
                tiny_db_free(p_des->p_head);
            }

            if(p_des->title){
                tiny_db_free(p_des->title);
            }

            tiny_db_free(p_des);
        }
        return TR_FAIL;
}

td_int32    tiny_db_table_destroy(td_int32 fd, tbl_manage_t *p_this, char *title){
    td_int32        ret     = TR_FAIL;
    tbl_desc_t  *   p_rec   = NULL;

    p_rec = _td_table_get_desinfo(fd, p_this, title);

    if(p_rec){
        int i = 0;

        list_del(&p_rec->list);
        tiny_db_free(p_rec->title);
        for(i = 0; i < p_rec->head_cnt; i ++){
            tiny_db_free(p_rec->p_head[i].title);
        }
        tiny_db_free(p_rec->p_head);
        tiny_db_node_destroy(fd, &p_rec->node);

        tiny_db_assert(tiny_db_node_del_by_id(fd, p_this->p_table, p_rec->node_id) == TR_SUCCESS);
        tiny_db_free(p_rec);
        
        ret = TR_SUCCESS;
    }

    return ret;
}