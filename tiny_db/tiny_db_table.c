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
#define MAX_STRING_FIX_LEN 64

typedef struct{
    td_uchar idx;
    td_uchar attr_mask;
    td_uint16 title_len : 6;
    td_uint16 title_off : 10;
    td_uint32 fix_length : 16;
    td_uint32 rev : 16;
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

typedef struct select_node{
    td_int32                order;
    td_int16                rev;
    used_node_t         *   p_used_node;
    struct select_node  *   p_next;
}select_node_t;

static td_int32 _td_table_condition_tag2index(tbl_head_t *p_head, int head_cnt, td_condition_t *p_cond){
    int i = 0, j = 0, ret = TR_FAIL;
    td_cond_elem_t  *   p_elem  = NULL;      

    for(; i < p_cond->count; i++){
        p_elem  = &p_cond->p_elements[i];
        if(p_elem->p_next){
            ret = _td_table_condition_tag2index(p_head, head_cnt, p_elem->p_next);
            if(ret != TR_SUCCESS){
                return ret;
            }
        }else{
            for(j = 0; j < head_cnt; j++){
                if(strcmp(p_head[j].title, p_elem->p_tag) == 0){
                    p_elem->tag_idx = j;
                    break;
                }
            }

            if(j >= head_cnt){
                return TR_FAIL;        
            }
        }
    }

    return TR_SUCCESS;
}

static td_int32 _td_table_element_tag2index(tbl_head_t *p_head, int head_cnt, td_elem_list_t *p_element){
    int i = 0, j = 0;
    td_elem_t  *    p_elem  = NULL;      
    int        *    p_content = NULL;
    for(; i < p_element->count; i++){
        p_elem  = &p_element->p_elem[i];

        for(j = 0; j < head_cnt; j++){
            if(strcmp(p_head[j].title, p_elem->p_tag) == 0){
                p_content = (int *)&p_elem->content;
                *p_content = j;
                break;
            }
        }

        if(j >= head_cnt){
            return TR_FAIL;        
        }

    }

    return TR_SUCCESS;
}



static td_int32 _td_table_buffer_preparse(tbl_desc_t *p_desc, tbl_manage_t *p_this, td_int32 buffer_len, td_condition_t *p_cond, int tag_index, td_uint32 *p_val){
    int i = 0, cursor = 0, len = 0;
    td_uchar id = 0, *p_buf = p_this->buffer, *p_pos = NULL;

    for(; i < p_desc->head_cnt; i++){
        p_desc->p_head_proc[i].buf_item.offset = 0;
        p_desc->p_head_proc[i].buf_item.used = 0;
    }

    if(p_val){
        *p_val = 0xFFFFFFFF;
    }

    while(cursor < buffer_len){
        id = p_buf[cursor];

        for(i = 0; i < p_desc->head_cnt; i++){
            if(id == p_desc->p_head_proc[i].idx){
                switch(p_desc->p_head_proc[i].attr_mask){
                    case TD_ELEM_TYPE_INTEGER:
                        p_desc->p_head_proc[i].buf_item.offset = cursor + 1;
                        p_desc->p_head_proc[i].buf_item.length = 4;
                        cursor += 5;

                        if(tag_index >= 0 && p_val && i == tag_index){
                            char *p = &p_buf[cursor - 4];
                            *p_val = TD_MAKE_DWORD(p);
                        }
                        break;

                    case TD_ELEM_TYPE_AUTO_INCREASE:
                        TD_TRUE_RETVAL(i >= p_desc->head_cnt, TR_FAIL, "auto increase error\n");
                        break;
                    
                    case TD_ELEM_TYPE_STRING:
                    case TD_ELEM_TYPE_STRING_FIXED:
                        p_pos = &p_buf[cursor + 1];
                        len = TD_MAKE_WORD(p_pos);
                        if(len > 0){
                            p_desc->p_head_proc[i].buf_item.offset = cursor + 3;
                            p_desc->p_head_proc[i].buf_item.length = len;
                        }
                        
                        if(tag_index >= 0 && p_val && i == tag_index){
                            if(len >= 2){
                                if(p_buf[cursor + 3] <= 0x7E){
                                    *p_val = (td_uchar)p_buf[cursor + 3]; 
                                }else{
                                    *p_val = p_buf[cursor + 3] | (p_buf[cursor + 4] << 8); 
                                }
                            }else if(len == 1){
                                *p_val = (td_uchar)p_buf[cursor + 3];            
                            }else{
                                *p_val = 0;
                            }
                        }

                        cursor += (3 + len);
                        break;

                        default:
                            TINY_DB_WARN("unknow type %d\n", p_desc->p_head_proc[i].attr_mask);
                            return TR_FAIL;
                            break;
                }

                break;
            }
        }

        TD_TRUE_RETVAL(i >= p_desc->head_cnt, TR_FAIL, "id %d error\n", id);
    }

    return TR_SUCCESS;
}

static td_int32 _td_table_condition_arithmetic(td_uint32 *p_src, td_uint32 src_len, td_uint32 *p_base, td_arithmetic_t arithmetic, td_int32 be_string){
    td_int32 ret = TR_FAIL;
    
    if(be_string == 0){
        switch(arithmetic){
            case TD_ARITHMETIC_EQUAL:
                if(*p_src == *p_base){
                    ret = TR_SUCCESS; 
                }
                break;

            case TD_ARITHMETIC_UNEQUAL:
                if(*p_src != *p_base){
                    ret = TR_SUCCESS; 
                }
                break;

            case TD_ARITHMETIC_GREATER:
                if(*p_src > *p_base){
                    ret = TR_SUCCESS; 
                }
                break;

            case TD_ARITHMETIC_GREATER_QUUAL:
                if(*p_src >= *p_base){
                    ret = TR_SUCCESS; 
                }
                break;   

            case TD_ARITHMETIC_LESS:
                if(*p_src < *p_base){
                    ret = TR_SUCCESS; 
                }
                break;
            
            case TD_ARITHMETIC_LESS_QUUAL:
                if(*p_src <= *p_base){
                    ret = TR_SUCCESS; 
                }
                break; 

                default:
                    ;
        }
    }else{
        char *p_str = (char *)p_src;
        char *p_dst = (char *)p_base;
        char bak = p_str[src_len];

        p_str[src_len] = 0;

        switch(arithmetic){
            case TD_ARITHMETIC_EQUAL:
                if(strcmp(p_str, p_dst) == 0){
                    ret = TR_SUCCESS; 
                }
                break;

            case TD_ARITHMETIC_UNEQUAL:
                if(strcmp(p_str, p_dst) != 0){
                    ret = TR_SUCCESS; 
                }
                break;

                default:
                    ;
        }

        p_str[src_len] = bak;
    }

    return ret;
}

static td_int32 _td_table_condition_check(tbl_desc_t *p_desc, tbl_manage_t *p_this, td_uint32 node_id, td_condition_t *p_cond){
    int         i = 0;
    td_int32    ret = TR_FAIL, val = 0;
    td_uchar   *p_cursor = NULL;
    tbl_head_t *p_head = p_desc->p_head_proc;

    while(i < p_cond->count){
        td_cond_elem_t *p_elem = &p_cond->p_elements[i];
        
        ret = TR_FAIL;
        if(p_elem->p_next){
            ret = _td_table_condition_check(p_desc, p_this, node_id, p_elem->p_next);
        }else{
            if(p_head[p_elem->tag_idx].attr_mask == TD_ELEM_TYPE_AUTO_INCREASE){
                if(p_elem->arithmetic < TD_ARITHMETIC_MAX){
                    ret = _td_table_condition_arithmetic((td_uint32 *)&node_id, 4, (td_uint32 *)p_elem->content, p_elem->arithmetic, 0);
                }
            }else
            if(p_head[p_elem->tag_idx].attr_mask == TD_ELEM_TYPE_INTEGER){
                if(p_elem->arithmetic < TD_ARITHMETIC_MAX){
                    p_cursor = &p_this->buffer[p_head[p_elem->tag_idx].buf_item.offset];
                    val = TD_MAKE_DWORD(p_cursor);
                    ret = _td_table_condition_arithmetic((td_uint32 *)&val, 4, (td_uint32 *)p_elem->content, p_elem->arithmetic, 0);
                }
            }else
            if(p_head[p_elem->tag_idx].attr_mask == TD_ELEM_TYPE_STRING || p_head[p_elem->tag_idx].attr_mask == TD_ELEM_TYPE_STRING_FIXED){
                if(p_elem->arithmetic < TD_ARITHMETIC_UNEQUAL && p_elem->content && p_head[p_elem->tag_idx].buf_item.length){
                    p_cursor = &p_this->buffer[p_head[p_elem->tag_idx].buf_item.offset];
                    val = p_head[p_elem->tag_idx].buf_item.length;
                    ret = _td_table_condition_arithmetic((td_uint32 *)p_cursor, val, (td_uint32 *)p_elem->content, p_elem->arithmetic, 0);
                }
            }
        }

        if(p_cond->logic == TD_LOGIC_OR){
            if(ret == TR_SUCCESS){
                break;
            }
        }else if(p_cond->logic == TD_LOGIC_AND){
            if(ret == TR_FAIL){
                break;
            }
        }

        i ++;
    }

    return ret;
}

static td_int32 _td_table_make_update_buffer(tbl_head_t *p_head, tbl_head_t *p_head_proc, int head_cnt, tbl_manage_t *p_this, td_int32 * buffer_length, td_elem_list_t *p_column){
    char buffer[MAX_BUFFER_LEN] = {0};
    char s_buf[4] = {0};
    st_data_cpy_t   data;
    td_int32        i = 0, j = 0;

    data.buffer = buffer;
    data.buffer_length = MAX_BUFFER_LEN;
    data.buffer_used = 0;

    for(i = 0; i < p_column->count; i++){
        td_elem_t *p_elem = &p_column->p_elem[i];

        for(j = 0; j < head_cnt; j ++){
            if(strcmp(p_elem->p_tag, p_head[j].title) == 0){
                if(p_head[j].attr_mask == TD_ELEM_TYPE_AUTO_INCREASE){
                    break;
                }

                p_head_proc[j].buf_item.used = 1;
                TD_TRUE_RETVAL(tiny_db_copy_block(&data, (char *)&p_head[j].idx, 1) != 1, TR_FAIL, "copy length fail %d\n", data.buffer_used);
                
                if(p_head[j].attr_mask == TD_ELEM_TYPE_INTEGER){
                    TD_DWORD_SERIALIZE(s_buf, *p_elem->content);
                    TD_TRUE_RETVAL(tiny_db_copy_block(&data, s_buf, 4) != 4, TR_FAIL, "copy length fail %d\n", data.buffer_used);
                }else
                if((p_head[j].attr_mask == TD_ELEM_TYPE_STRING_FIXED || p_head[j].attr_mask == TD_ELEM_TYPE_STRING) 
                    && p_elem->content){
                    char *p_str = (char *)p_elem->content;
                    int len = strlen(p_str);

                    if(p_head[j].attr_mask == TD_ELEM_TYPE_STRING_FIXED){
                        len = len > p_head[j].fix_length ? p_head[j].fix_length : len;
                    }
                    TD_WORD_SERIALIZE(s_buf, len);
                    TD_TRUE_RETVAL(tiny_db_copy_block(&data, s_buf, 2) != 2, TR_FAIL, "copy length fail %d\n", data.buffer_used);
                    TD_TRUE_RETVAL(tiny_db_copy_block(&data, (char *)p_elem->content, len) != len, TR_FAIL, "copy length fail %d\n", data.buffer_used);
                }
            }
        }
    }

    for(i = 0; i < head_cnt; i++){
        if(p_head_proc[j].buf_item.used == 0 && p_head_proc[j].buf_item.offset){
            if(p_head_proc[i].attr_mask == TD_ELEM_TYPE_INTEGER){
                TD_TRUE_RETVAL(tiny_db_copy_block(&data, &p_this->buffer[p_head_proc[j].buf_item.offset - 1], 5) != 5, TR_FAIL, "copy length fail %d\n", data.buffer_used);
            }else
            if(p_head_proc[i].attr_mask == TD_ELEM_TYPE_STRING || p_head_proc[i].attr_mask == TD_ELEM_TYPE_STRING_FIXED){
                int len = p_head_proc[j].buf_item.length + 3;
                TD_TRUE_RETVAL(tiny_db_copy_block(&data, &p_this->buffer[p_head_proc[j].buf_item.offset - 3], len) != len, TR_FAIL, "copy length fail %d\n", data.buffer_used);
            }
        }
    }

    *buffer_length = data.buffer_used;
    memcpy(p_this->buffer, data.buffer, data.buffer_used);

    return TD_SUCCESS;
}

static int _td_table_notify_data(tbl_desc_t *p_desc, tbl_manage_t *p_this, td_uint32 node_id, td_elem_list_t *p_elements, void *p_data, tiny_db_callback callback){
    int         i = 0;
    
    int         argc = p_elements->count;
    char    *   argv[argc];
    int         argv_len[argc]; 
    char    *   col_names[argc];

    tbl_head_t *p_head = p_desc->p_head_proc;

    for(i = 0; i < p_elements->count; i ++){
        int tag_idx = (int)p_elements->p_elem[i].content;

        if(p_head[tag_idx].attr_mask == TD_ELEM_TYPE_AUTO_INCREASE){
            argv[i] = (char *)node_id;
            argv_len[i] = 4;
        }else if(p_head[tag_idx].attr_mask == TD_ELEM_TYPE_INTEGER){
            if(p_head[tag_idx].buf_item.offset >= 0){
                char *p = &p_this->buffer[p_head[tag_idx].buf_item.offset];
                td_uint32 val = TD_MAKE_DWORD(p);
                argv[i] = (char *)val;
                argv_len[i] = 4;
            }else{
                argv[i] = (char *)NULL;
                argv_len[i] = 0;
            }
        }else{
            if(p_head[tag_idx].buf_item.offset >= 0){
                argv[i] = (char *)&p_this->buffer[p_head[tag_idx].buf_item.offset];
                argv_len[i] = p_head[tag_idx].buf_item.length;
            }else{
                argv[i] = (char *)NULL;
                argv_len[i] = 0;
            }
        }

        col_names[i] = p_elements->p_elem[i].p_tag;
    }


    return callback(p_data, argc, argv, argv_len, col_names);
}

static td_int32 _td_table_normal_serialize(tbl_desc_t *p_rec, tbl_manage_t *p_this, td_elem_t *p_elements, int count){
    td_int32        i = 0, j = 0;
    td_uchar        s_buf[4]    = {0};
    st_data_cpy_t   data;
    tbl_head_t  *   p_head = p_rec->p_head;     
    
    data.buffer = p_this->buffer;
    data.buffer_length = MAX_BUFFER_LEN;
    data.buffer_used = 0;

    for(i = 0; i < count; i++){
        td_elem_t *p_elem = &p_elements[i];

        for(j = 0; j < p_rec->head_cnt; j ++){
            if(strcmp(p_elem->p_tag, p_head[j].title) == 0){
                if(p_head[j].attr_mask == TD_ELEM_TYPE_AUTO_INCREASE){
                    break;
                }

                TD_TRUE_RETVAL(tiny_db_copy_block(&data, (char *)&p_head[j].idx, 1) != 1, TR_FAIL, "copy length fail %d\n", data.buffer_used);
                
                if(p_head[j].attr_mask == TD_ELEM_TYPE_INTEGER){
                    TD_DWORD_SERIALIZE(s_buf, *p_elem->content);
                    TD_TRUE_RETVAL(tiny_db_copy_block(&data, s_buf, 4) != 4, TR_FAIL, "copy length fail %d\n", data.buffer_used);
                }else
                if((p_head[j].attr_mask == TD_ELEM_TYPE_STRING_FIXED || p_head[j].attr_mask == TD_ELEM_TYPE_STRING) 
                    && p_elem->content){
                    char *p_str = (char *)p_elem->content;
                    int len = strlen(p_str);

                    if(p_head[j].attr_mask == TD_ELEM_TYPE_STRING_FIXED){
                        len = len > p_head[j].fix_length ? p_head[j].fix_length : len;
                    }
                    TD_WORD_SERIALIZE(s_buf, len);
                    TD_TRUE_RETVAL(tiny_db_copy_block(&data, s_buf, 2) != 2, TR_FAIL, "copy length fail %d\n", data.buffer_used);
                    TD_TRUE_RETVAL(tiny_db_copy_block(&data, (char *)p_elem->content, len) != len, TR_FAIL, "copy length fail %d\n", data.buffer_used);
                }
            }
        }
    }

    return data.buffer_used;
}

static td_int32 _td_table_manage_parse(td_int32 fd, mod_node_t *p_table_mode, used_node_t *p_used, list_head_t *p_list_head){
    char table_buffer[512] = {0};
    tbl_node_t *p_tblm_serialize = NULL;
    tbl_item_t *p_tbit_serialize = NULL;

    tbl_desc_t      *p_des  = NULL;
    mod_node_t      *p_mnod = NULL; 
    td_mod_info_t   *p_mod  = NULL;
    td_int32 offset = 0, index = 0, buffer_len = 512;

    tiny_db_assert(tiny_db_node_get_by_pos(fd, p_table_mode, p_used, table_buffer, &buffer_len) == TR_SUCCESS);

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
        p_header->fix_length = p_tbit_serialize->fix_length;

        tiny_db_assert(p_tbit_serialize->title_len == 0);
        offset = p_tbit_serialize->title_off;
        p_header->title = tiny_db_strdup_fix(&table_buffer[offset], p_tbit_serialize->title_len);
        tiny_db_assert(p_header->title != NULL);
    }

    p_des->p_head_proc = tiny_db_malloc(sizeof(tbl_head_t) * p_des->head_cnt);
    memcpy(p_des->p_head_proc, p_des->p_head, sizeof(tbl_head_t) * p_des->head_cnt);

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
            tiny_db_free(p_rec->p_head_proc);
            tiny_db_node_deinit(fd, &p_rec->node);
            tiny_db_free(p_rec);
        }
    }

    tiny_db_node_deinit(fd, p_this->p_table);

    if(p_this->p_table){
        tiny_db_free(p_this->p_table);
    }

    if(p_this->buffer){
        tiny_db_free(p_this->buffer);
    }

    memset(p_this, 0, sizeof(tbl_manage_t));

    return TR_SUCCESS;
}

td_int32    tiny_db_table_create(td_int32 fd, tbl_manage_t *p_this, char *title, td_elem_list_t *p_column){
    td_int32        new_module_id = 0, length = 0, i = 0, val = 0, free_node = 0;
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
                //p_item[i].attr_mask = 0b11111000;
                p_item[i].attr_mask = TD_ELEM_TYPE_STRING;
                break;

            case TD_ELEM_TYPE_STRING_FIXED:
                //p_item[i].attr_mask = 0b11111100;
                p_item[i].attr_mask = TD_ELEM_TYPE_STRING_FIXED;
                p_item[i].fix_length = *(int *)p_elem->content;

                if(p_item[i].fix_length >= MAX_STRING_FIX_LEN){
                    p_item[i].fix_length = 64;
                }
                esti_len += (1 + 2 + p_item[i].fix_length);
                break;
            
            case TD_ELEM_TYPE_INTEGER:
                esti_len += 5;
                //p_item[i].attr_mask = 0b11111101;
                p_item[i].attr_mask = TD_ELEM_TYPE_INTEGER;
                break;
            
            case TD_ELEM_TYPE_AUTO_INCREASE:
                if(already_has){
                    TINY_DB_ERR("already set auto\n");
                    goto _err;
                }else{
                    already_has = 1;
                    //p_item[i].attr_mask = 0b11111111;
                    p_item[i].attr_mask = TD_ELEM_TYPE_AUTO_INCREASE;
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

    p_des->p_head_proc = tiny_db_malloc(sizeof(tbl_head_t) * p_column->count);
    TD_TRUE_JUMP(NULL == p_des->p_head_proc, _err, "no memory\n");
    memcpy(p_des->p_head_proc, p_des->p_head, sizeof(tbl_head_t) * p_column->count);

    p_node->node_len = esti_len;
    if(esti_len >= 255){
        p_node->node_len = 255;
    }

    p_des->node.last_node_id = p_node->last_node_id;
    p_des->node.node_length = p_node->node_len;
    p_des->node.module.module_id = new_module_id;
    p_des->node.module.first_page_id = TINY_INVALID_PAGE_ID;

    TD_TRUE_JUMP(TR_SUCCESS != tiny_db_node_init(fd, &p_des->node), _err, "node init error\n");
    free_node = 1;

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
            if(free_node){
                tiny_db_node_deinit(fd, &p_des->node);
            }
            if(p_des->p_head){
                for(i = 0; i < p_column->count; i ++){
                    if(p_des->p_head[i].title){
                        tiny_db_free(p_des->p_head[i].title);
                    }
                }
                tiny_db_free(p_des->p_head);
            }

            if(p_des->p_head_proc){
                tiny_db_free(p_des->p_head_proc);
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
        tiny_db_free(p_rec->p_head_proc);
        tiny_db_node_destroy(fd, &p_rec->node);

        tiny_db_assert(tiny_db_node_del_by_id(fd, p_this->p_table, p_rec->node_id) == TR_SUCCESS);
        tiny_db_free(p_rec);
        
        ret = TR_SUCCESS;
    }

    return ret;
}

td_int32    tiny_db_table_insert(td_int32 fd, tbl_manage_t *p_this, char *title, td_elem_t *p_elements, int count){
    td_int32        ret     = TR_FAIL;
    tbl_desc_t  *   p_rec   = NULL;

    p_rec = _td_table_get_desinfo(fd, p_this, title);
    TD_TRUE_RETVAL(p_rec == NULL, ret, "table %s is not exist\n", title);

    ret = _td_table_normal_serialize(p_rec, p_this, p_elements, count);
    TD_TRUE_RETVAL(ret <= 0, TR_FAIL, "Serialized fail\n");

    ret = tiny_db_node_set(fd, &p_rec->node, p_this->buffer, ret);
    TINY_DB_INFO("insert into %s, ret=%d", title, ret);

    return ret;
}

td_int32    tiny_db_table_update(td_int32 fd, tbl_manage_t *p_this, char *title, td_elem_list_t *p_column, td_condition_t *p_cond){
    tbl_desc_t      * p_des     = NULL; 
    mod_node_t      * p_module  = NULL;

    p_des = _td_table_get_desinfo(fd, p_this, title);
    TD_TRUE_RETVAL(p_des == NULL, TR_FAIL, "table %s is not exist\n", title);

    TD_TRUE_RETVAL(_td_table_condition_tag2index(p_des->p_head, p_des->head_cnt, p_cond) == TR_FAIL, TR_FAIL, "condition tag fail\n", title);

    p_module = &p_des->node;

    list_head_t *p_head = NULL;
    list_head_t *p_tmp  = NULL;
    used_node_t *p_node = NULL;
    int buffer_len = MAX_BUFFER_LEN;

    if(!list_empty(&p_module->list_head)){
        list_for_each_safe(p_head,p_tmp,&p_module->list_head){
            p_node = list_entry(p_head, used_node_t, list);

            if(NULL != p_node){
                TD_TRUE_RETVAL(tiny_db_node_get_by_pos(fd, p_module, p_node, p_this->buffer, &buffer_len) == TR_FAIL, TR_FAIL, "get node fail [%s, %s]", p_node->node_id, p_node->node_pos);
                TD_TRUE_RETVAL(_td_table_buffer_preparse(p_des, p_this, buffer_len, p_cond, 0, NULL) == TR_FAIL, TR_FAIL, "node [%s, %s], parse fail", p_node->node_id, p_node->node_pos);

                if(_td_table_condition_check(p_des, p_this, p_node->node_id, p_cond) == TR_SUCCESS && _td_table_make_update_buffer(p_des->p_head, p_des->p_head_proc, p_des->head_cnt, p_this, &buffer_len, p_column) == TR_SUCCESS){

                    return tiny_db_node_update_by_pos(fd, p_module, p_node, p_this->buffer, buffer_len);
                }
            }
        }
    }
    
    TINY_DB_DBG("don't find node\n");
    return TR_FAIL;
}

td_int32    tiny_db_table_delete(td_int32 fd, tbl_manage_t *p_this, char *title, td_condition_t *p_cond){
    tbl_desc_t      * p_des     = NULL; 
    mod_node_t      * p_module  = NULL;
    td_int32          ret       = TR_FAIL;  

    p_des = _td_table_get_desinfo(fd, p_this, title);
    TD_TRUE_RETVAL(p_des == NULL, TR_FAIL, "table %s is not exist\n", title);

    TD_TRUE_RETVAL(_td_table_condition_tag2index(p_des->p_head, p_des->head_cnt, p_cond) == TR_FAIL, TR_FAIL, "condition tag fail\n", title);

    p_module = &p_des->node;

    list_head_t *p_head = NULL;
    list_head_t *p_tmp  = NULL;
    used_node_t *p_node = NULL;
    int buffer_len = MAX_BUFFER_LEN;

    if(!list_empty(&p_module->list_head)){
        list_for_each_safe(p_head,p_tmp,&p_module->list_head){
            p_node = list_entry(p_head, used_node_t, list);

            if(NULL != p_node){
                TD_TRUE_RETVAL(tiny_db_node_get_by_pos(fd, p_module, p_node, p_this->buffer, &buffer_len) == TR_FAIL, TR_FAIL, "get node fail [%s, %s]", p_node->node_id, p_node->node_pos);
                TD_TRUE_RETVAL(_td_table_buffer_preparse(p_des, p_this, buffer_len, p_cond, 0, NULL) == TR_FAIL, TR_FAIL, "node [%s, %s], parse fail", p_node->node_id, p_node->node_pos);

                if(_td_table_condition_check(p_des, p_this, p_node->node_id, p_cond) == TR_SUCCESS){

                    ret = tiny_db_node_del_by_pos(fd, p_module, p_node);
                }
            }
        }
    }
    
    TINY_DB_DBG("don't find node\n");
    return ret;
}

td_int32    tiny_db_table_select_count(td_int32 fd, tbl_manage_t *p_this, char *title, td_select_t *p_select){
    td_int32            ret = 0;

    tbl_desc_t      *   p_des     = NULL; 
    mod_node_t      *   p_module  = NULL;

    p_des = _td_table_get_desinfo(fd, p_this, title);
    TD_TRUE_RETVAL(p_des == NULL, TR_FAIL, "table %s is not exist\n", title);
    
    p_module = &p_des->node;

    if(p_select && p_select->cond.count){
        TD_TRUE_RETVAL(_td_table_condition_tag2index(p_des->p_head, p_des->head_cnt, &p_select->cond) == TR_FAIL, TR_FAIL, "condition tag fail\n", title);

        list_head_t *p_head = NULL;
        list_head_t *p_tmp  = NULL;
        used_node_t *p_node = NULL;
        int buffer_len = MAX_BUFFER_LEN;

        if(!list_empty(&p_module->list_head)){
            list_for_each_safe(p_head,p_tmp,&p_module->list_head){
                p_node = list_entry(p_head, used_node_t, list);

                if(NULL != p_node){
                    TD_TRUE_RETVAL(tiny_db_node_get_by_pos(fd, p_module, p_node, p_this->buffer, &buffer_len) == TR_FAIL, TR_FAIL, "get node fail [%s, %s]", p_node->node_id, p_node->node_pos);
                    TD_TRUE_RETVAL(_td_table_buffer_preparse(p_des, p_this, buffer_len, &p_select->cond, 0, NULL) == TR_FAIL, TR_FAIL, "node [%s, %s], parse fail", p_node->node_id, p_node->node_pos);

                    if(_td_table_condition_check(p_des, p_this, p_node->node_id, &p_select->cond) == TR_SUCCESS){
                        ret ++;
                    }
                }
            }
        }
    }else{
        ret = p_module->used_count;
    }


    return ret;
}



td_int32    tiny_db_table_select_data(td_int32 fd, tbl_manage_t *p_this, char *title, td_elem_list_t *p_elements, tiny_db_callback callback, td_select_t *p_select, void *p_data){
    td_int32            ret = TR_SUCCESS, value = 0;

    tbl_desc_t      *   p_des     = NULL; 
    mod_node_t      *   p_module  = NULL;

    list_head_t     *   p_head = NULL;
    list_head_t     *   p_tmp  = NULL;
    used_node_t     *   p_node = NULL;
    int                 buffer_len = MAX_BUFFER_LEN;

    select_node_t   *   p_sels = NULL;
    int                 sels_count = 0, notify_count = 0;
    select_node_t       head ;

    head.p_next = NULL;

    p_des = _td_table_get_desinfo(fd, p_this, title);
    TD_TRUE_RETVAL(p_des == NULL, TR_FAIL, "table %s is not exist\n", title);

    TD_TRUE_RETVAL(_td_table_element_tag2index(p_des->p_head, p_des->head_cnt, p_elements) == TR_FAIL, TR_FAIL, "elements tag fail\n");

    p_module = &p_des->node;
    TD_TRUE_RETVAL(p_module->used_count <= 0, TR_FAIL, "table %s empty\n", title);

    if(p_select){
        if(p_select->cond.count){
            TD_TRUE_RETVAL(_td_table_condition_tag2index(p_des->p_head, p_des->head_cnt, &p_select->cond) == TR_FAIL, TR_FAIL, "condition tag fail\n", title);
        }

        if(p_select->order.p_tag && p_select->order.type < TD_ORDER_NONE){
            int j = 0;

            p_select->order.tag_idx = -1;
            for(j = 0; j < p_des->head_cnt; j++){
                if(strcmp(p_des->p_head[j].title, p_select->order.p_tag) == 0){
                    if(p_des->p_head[j].attr_mask == TD_ELEM_TYPE_AUTO_INCREASE){
                        if(p_select->order.type == TD_ORDER_DESC){
                            p_select->order.tag_idx = -2;
                        }
                        break;
                    }
                    p_select->order.tag_idx = j;
                    break;
                }
            }

            if(p_select->order.tag_idx != -1){
                sels_count = sizeof(select_node_t) * p_module->used_count;
                p_sels = tiny_db_malloc(sels_count);
                TD_TRUE_JUMP(p_sels == NULL, _error, "no memory\n");
                memset(p_sels, 0, sels_count);
            }
        }
    }

    if(!list_empty(&p_module->list_head)){
        list_for_each_safe(p_head,p_tmp,&p_module->list_head){
            p_node = list_entry(p_head, used_node_t, list);

            if(NULL != p_node){
                TD_TRUE_JUMP(tiny_db_node_get_by_pos(fd, p_module, p_node, p_this->buffer, &buffer_len) == TR_FAIL, _error, "get node fail [%s, %s]", p_node->node_id, p_node->node_pos);
                TD_TRUE_JUMP(_td_table_buffer_preparse(p_des, p_this, buffer_len, &p_select->cond, p_select->order.tag_idx, &value) == TR_FAIL, _error, "node [%s, %s], parse fail", p_node->node_id, p_node->node_pos);

                if(p_select && p_select->cond.count && _td_table_condition_check(p_des, p_this, p_node->node_id, &p_select->cond) != TR_SUCCESS){
                    continue;
                }

                if(p_sels == NULL){
                    if(p_select->limit_count == -1 || notify_count < p_select->limit_count){
                        TD_TRUE_JUMP(_td_table_notify_data(p_des, p_this, p_node->node_id, p_elements, p_data, callback) == TR_FAIL, _error, "copy failed\n");

                        notify_count ++;            
                    }else{
                        ret = TR_SUCCESS;
                        break; 
                    }
                }else{
                    select_node_t *p_nouse = &p_sels[notify_count];

                    if(head.p_next == NULL){
                        head.p_next = p_nouse;
                    }

                    if(p_select->order.tag_idx == -2){
                        p_nouse->order = p_node->node_id;
                    }else{
                        p_nouse->order = value;
                    }
                    p_nouse->p_used_node = p_node;

                    if(head.p_next != p_nouse){
                        select_node_t *p_curn = head.p_next;
                        select_node_t *p_prev = NULL;

                        if(p_select->order.type == TD_ORDER_ASC){
                            while(p_curn && p_nouse->order > p_curn->order){
                                p_prev = p_curn;
                                p_curn = p_curn->p_next;
                            }
                        }else{
                            while(p_curn && p_nouse->order < p_curn->order){
                                p_prev = p_curn;
                                p_curn = p_curn->p_next;
                            }
                        }

                        if(p_prev == NULL){
                            p_nouse->p_next = head.p_next;
                            head.p_next = p_nouse;
                        }else if(p_curn == NULL){
                            p_prev->p_next = p_nouse;
                        }else{
                            p_prev->p_next = p_nouse;
                            p_nouse->p_next = p_curn;
                        }
                    }
                    notify_count ++;
                }
            }
        }

        if(head.p_next){
            select_node_t *p_nouse = head.p_next;
            notify_count = 0;

            while(p_nouse){
                p_node = p_nouse->p_used_node;
                TD_TRUE_JUMP(tiny_db_node_get_by_pos(fd, p_module, p_node, p_this->buffer, &buffer_len) == TR_FAIL, _error, "get node fail [%s, %s]", p_node->node_id, p_node->node_pos);
                TD_TRUE_JUMP(_td_table_buffer_preparse(p_des, p_this, buffer_len, &p_select->cond, p_select->order.tag_idx, &value) == TR_FAIL, _error, "node [%s, %s], parse fail", p_node->node_id, p_node->node_pos);

                if(p_select->limit_count == -1 || notify_count < p_select->limit_count){
                    TD_TRUE_JUMP(_td_table_notify_data(p_des, p_this, p_node->node_id, p_elements, p_data, callback) == TR_FAIL, _error, "copy failed\n");

                    notify_count ++;            
                }else{
                    ret = TR_SUCCESS;
                    break; 
                }

                p_nouse = p_nouse->p_next;
            }
        }

        ret = TR_SUCCESS;
    }

    if(p_sels){
        tiny_db_free(p_sels);
    }

    return ret ;

        _error:
        if(p_sels){
            tiny_db_free(p_sels);
        }
        return TR_FAIL;
}