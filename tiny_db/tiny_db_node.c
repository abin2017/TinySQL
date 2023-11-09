/*
    数据库页管理。每页2K
    Page0 用于存储数据库页的状态，每页用1bit表示，0：被占用， 1： 未被使用
    Page1 用于描述TABLE信息

    tage_START  0xAA55AA55
    tage_END    0x55AA55AA

    最大管理 (2K - 8) * 32

    即每个数据库page0和1被占用
    */

/*
    AA 55 AA 55  CF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
    
    ......
    
    FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 55 AA 55 AA
*/

/*
    | 4字节     |  156字节，每个Node4字节  |
    | module管理|  描述一个page最大38个Node |
    每个Node最小48个字节， 有效字节为46[前面两个字节为NEXT]
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

//可能多个node拼凑一个Record
#define RECORD_STATE_FREE       0b11
#define RECORD_STATE_START      0b10
#define RECORD_STATE_UESD       0b00

#define INVALID_NODE_ID       0xFFFF
#define INVALID_NODE_POS       0xFFFF
#define FREE_QUEUE_INCREASE_CNT  48
#define NODE_MIN_LEN  48

#define NODE_MANAGE_START 4
#define NODE_CONTENT_START 160
#define NODE_NEXT_LEN  2
#define NODE_LENGTH_LEN  2

static td_int32 _td_node_write(td_int32 fd, mod_node_t *p_module, td_uint16 * p_nodes, int node_cnt, td_uint16 node_id, td_char *buffer, td_int32 buffer_len){
    td_int32 ret = TR_SUCCESS, offset = 0, node_head = 0, node_buff = 0;
    td_uint16 save_pos = 0, i = 0;
    td_uint16 page_index = 0, node_index = 0;
    db_node_t header;

    for(i = 0; i < node_cnt; i++){
        char buf[p_module->node_length], *p_cursor;
        page_index = p_nodes[i] / p_module->node_cnt_in_1_page;
        node_index = p_nodes[i] % p_module->node_cnt_in_1_page;

        offset = tiny_db_module_map(fd, &p_module->module, page_index);

        node_head = offset + NODE_MANAGE_START + node_index * sizeof(db_node_t);
        node_buff = offset + NODE_CONTENT_START + node_index * p_module->node_length;

        memset(&header, 0xFF, sizeof(db_node_t));

        tiny_db_assert(tiny_db_OsSeek(fd, node_head) == TR_SUCCESS);

        if(i == 0){
            header.node_state = RECORD_STATE_START;
        }else{
            header.node_state = RECORD_STATE_UESD;      
        }

        header.node_id = node_id;
        tiny_db_assert(tiny_db_OsWrite(fd, &header, sizeof(db_node_t)) == sizeof(db_node_t));

        p_cursor = &buffer[save_pos];

        if(i == node_cnt - 1){
            TD_WORD_SERIALIZE(buf, INVALID_NODE_POS);

            if(i == 0){
                char *p = &buf[NODE_NEXT_LEN];

                TD_WORD_SERIALIZE(p, buffer_len);     
                memcpy(&buf[NODE_NEXT_LEN + NODE_LENGTH_LEN], p_cursor, buffer_len - save_pos);    
            }else{
                memcpy(&buf[NODE_NEXT_LEN], p_cursor, buffer_len - save_pos);
            }
            
        }else{
            TD_WORD_SERIALIZE(buf, p_nodes[i + 1]);

            if(i == 0){
                char *p = &buf[NODE_NEXT_LEN];

                TD_WORD_SERIALIZE(p, buffer_len);

                memcpy(&buf[NODE_NEXT_LEN + NODE_LENGTH_LEN], p_cursor, p_module->node_length - NODE_NEXT_LEN - NODE_LENGTH_LEN);
                save_pos += (p_module->node_length - NODE_NEXT_LEN - NODE_LENGTH_LEN);
            }else{
                memcpy(&buf[NODE_NEXT_LEN], p_cursor, p_module->node_length - NODE_NEXT_LEN);
                save_pos += (p_module->node_length - NODE_NEXT_LEN);
            }
        }

        tiny_db_assert(tiny_db_OsSeek(fd, node_buff) == TR_SUCCESS);
        tiny_db_assert(tiny_db_OsWrite(fd, buf, p_module->node_length) == p_module->node_length);
    }


    return ret;
}

static td_int32 _td_node_set(td_int32 fd, mod_node_t *p_module, td_char *buffer, td_int32 buffer_len, used_node_t *p_node){
    int need_cnt = (buffer_len + NODE_LENGTH_LEN + p_module->node_length - NODE_NEXT_LEN - 1)/(p_module->node_length - NODE_NEXT_LEN);
    td_int32 ret = TR_FAIL;

    if(need_cnt == 0){
        TINY_DB_ERR("error, need_cnt = %d\n", need_cnt);
        return TR_FAIL;
    }

    if(need_cnt > p_module->node_cnt_in_1_page){
        TINY_DB_ERR("buffer_len too big\n");
        return TR_FAIL;
    }

    if(need_cnt > p_module->free_node_count){
        int i = 0;
        td_uint16 node_pos = 0;
        st_data_block_t block = {NULL};
        
        //目前空缺的node不够，申请新的pager
        if(tiny_db_module_enlarge(fd, &p_module->module) != TR_SUCCESS){
            TINY_DB_ERR("malloc page failed\n");
            return TR_FAIL;
        }

        block.ppblock = (void **)&p_module->p_free_nodes;
        block.current_cnt = (td_uint16 *)&p_module->free_node_count;
        block.block_cnt = (td_uint16 *)&p_module->free_node_buffer_size;
        block.block_size = sizeof(td_uint16);//td_mod_page_t);
        block.enlarge_size = FREE_QUEUE_INCREASE_CNT * 2;
        block.block = (void *)&node_pos;

        for(i = 0; i < p_module->node_cnt_in_1_page; i ++){
            node_pos = (p_module->module.page_count - 1) * p_module->node_cnt_in_1_page + i;
            ret = tiny_db_insert_block(&block);
            tiny_db_assert(ret == TR_SUCCESS);
        }
    }

    if(NULL == p_node){
        p_node = tiny_db_malloc(sizeof(used_node_t));
        if(NULL == p_node){
            TINY_DB_ERR("no memeory\n");
            return TR_FAIL;
        }

        memset(p_node, 0, sizeof(used_node_t));
        list_add_tail(&p_node->list, &p_module->list_head);

        p_node->node_id = p_module->last_node_id;
        p_module->last_node_id = p_module->last_node_id >= INVALID_NODE_ID ? 0 : p_module->last_node_id + 1;
    }

    p_module->free_node_count -= need_cnt;
    p_node->node_pos = p_module->p_free_nodes[p_module->free_node_count];
    p_module->used_count ++;
    p_module->last_node_pos = p_node->node_pos;

    _td_node_write(fd, p_module, &p_module->p_free_nodes[p_module->free_node_count], need_cnt, p_node->node_id, buffer, buffer_len);
    
    return TR_SUCCESS;
}

static td_int32 _td_node_del_by_pos(td_int32 fd, mod_node_t *p_module, used_node_t *p_rec){
    td_int32 ret = TR_FAIL;
    st_data_block_t block = {NULL};
    
    td_uint16 node_pos = p_rec->node_pos;

    td_int32 offset = 0, node_head = 0, node_buff = 0;
    td_uint16 page_index = 0, node_index = 0;
    db_node_t header;
    td_uchar bf[NODE_NEXT_LEN] = {0};

    p_module->used_count --;

    block.ppblock = (void **)&p_module->p_free_nodes;
    block.current_cnt = (td_uint16 *)&p_module->free_node_count;
    block.block_cnt = (td_uint16 *)&p_module->free_node_buffer_size;
    block.block_size = sizeof(td_uint16);//td_mod_page_t);
    block.enlarge_size = FREE_QUEUE_INCREASE_CNT * 2;
    block.block = (void *)&node_pos;

    while(node_pos != INVALID_NODE_POS){
        page_index = node_pos / p_module->node_cnt_in_1_page;
        node_index = node_pos % p_module->node_cnt_in_1_page;

        offset = tiny_db_module_map(fd, &p_module->module, page_index);

        node_head = offset + NODE_MANAGE_START + node_index * sizeof(db_node_t);
        node_buff = offset + NODE_CONTENT_START + node_index * p_module->node_length;

        tiny_db_assert(tiny_db_OsSeek(fd, node_head) == TR_SUCCESS);
        tiny_db_assert(tiny_db_OsRead(fd, &header, sizeof(db_node_t)) == 4 && (header.node_state == RECORD_STATE_START || header.node_state == RECORD_STATE_UESD) && (header.node_id != p_rec->node_id));
        header.node_state = RECORD_STATE_FREE;
        tiny_db_assert(tiny_db_OsSeek(fd, node_head) == TR_SUCCESS);
        tiny_db_assert(tiny_db_OsWrite(fd, &header, sizeof(db_node_t)) == 4);

        ret = tiny_db_insert_block(&block);
        tiny_db_assert(ret == TR_SUCCESS);

        tiny_db_assert(tiny_db_OsSeek(fd, node_buff) == TR_SUCCESS);
        tiny_db_assert(tiny_db_OsRead(fd, bf, NODE_NEXT_LEN) == NODE_NEXT_LEN);

        node_pos = TD_MAKE_WORD(bf);
    }

    return TR_SUCCESS;
}

static td_int32 _td_node_del_by_id(td_int32 fd, mod_node_t *p_module, td_int16 node_id, used_node_t **p_node){
    td_int32 ret = TR_FAIL;
    used_node_t *p_rec = NULL;
    
    if(!list_empty(&p_module->list_head)){
        list_head_t *p_head = NULL;
        list_head_t *p_tmp  = NULL;

        list_for_each_safe(p_head,p_tmp,&p_module->list_head){
            p_rec = list_entry(p_head, used_node_t, list);

            if(NULL != p_rec){
                if(p_rec->node_id == node_id){
                    list_del(&p_rec->list);
                    ret = TR_SUCCESS;
                    break;
                }
            }
        }
    }

    if(ret == TR_SUCCESS){
        _td_node_del_by_pos(fd, p_module, p_rec);

        if(p_node){
            *p_node = p_rec;
        }else{
            tiny_db_free(p_rec);
        }
    }      

    return ret;
}

td_int32    tiny_db_node_adjust_node_len(td_int32 fd, td_uchar len){
    if(len < NODE_MIN_LEN){
        len = NODE_MIN_LEN;   
    }else{
        int page_size = tiny_db_get_page_size(fd);
        int count = 0;

        count = (page_size - NODE_CONTENT_START)/len;

        len = (page_size - NODE_CONTENT_START)/count;

        if(len > 255){
            len = 255;
        }
    }
    return len;
}

td_int32    tiny_db_node_init(td_int32 fd, mod_node_t *p_module){
    int page_size = 0, i = 0, j = 0;
    td_int32 ret = TR_FAIL, offset = 0;
    td_int16 node_pos = 0;

    st_data_block_t block = {NULL};

    if(p_module->node_length < NODE_MIN_LEN){
        p_module->node_length = NODE_MIN_LEN;   
    }

    list_init(&p_module->list_head);
    p_module->used_count = 0;

    page_size = tiny_db_get_page_size(fd);
    p_module->node_cnt_in_1_page = (page_size - NODE_CONTENT_START)/p_module->node_length;

    ret = tiny_db_module_init(fd, &p_module->module);
    tiny_db_assert(ret == TR_SUCCESS);

    block.ppblock = (void **)&p_module->p_free_nodes;
    block.current_cnt = (td_uint16 *)&p_module->free_node_count;
    block.block_cnt = (td_uint16 *)&p_module->free_node_buffer_size;
    block.block_size = sizeof(td_uint16);//td_mod_page_t);
    block.enlarge_size = FREE_QUEUE_INCREASE_CNT * p_module->module.page_count;
    block.block = (void *)&node_pos;

    for(i = 0; i < p_module->module.page_count; i ++){
        offset = tiny_db_module_map(fd, &p_module->module, i);

        tiny_db_assert(tiny_db_OsSeek(fd, offset + NODE_MANAGE_START) == TR_SUCCESS);

        for(j = 0; j < p_module->node_cnt_in_1_page; j ++){
            db_node_t node;

            tiny_db_assert(tiny_db_OsRead(fd, (void *)&node, sizeof(db_node_t)) == 4);

            if((node.node_state & 0x3) == RECORD_STATE_FREE){
                node_pos = i * p_module->node_cnt_in_1_page + j;

                ret = tiny_db_insert_block(&block);
                tiny_db_assert(ret == TR_SUCCESS);
            }else if((node.node_state & 0x3) == RECORD_STATE_START){
                used_node_t *p_node = tiny_db_malloc(sizeof(used_node_t));
                
                tiny_db_assert(p_node != NULL);
                p_node->node_id = node.node_id;
                p_node->node_pos = i * p_module->node_cnt_in_1_page + j;

                list_head_t *p_last = &p_module->list_head;
                if(!list_empty(&p_module->list_head)){
                    list_head_t *p_head = NULL;
                    list_head_t *p_tmp  = NULL;
                    used_node_t *p_rec = NULL;
                    list_for_each_safe(p_head,p_tmp,&p_module->list_head){
                        p_rec = list_entry(p_head, used_node_t, list);

                        if(NULL != p_rec){
                            if(p_rec->node_id > p_node->node_id){
                                list_add(&p_node->list, p_last);
                                p_module->used_count ++;
                                goto  _end;          
                            }         
                        }
                        p_last = &p_rec->list;
                    }
                }

                list_add_tail(&p_node->list, &p_module->list_head);
                p_module->used_count ++;
                _end:
                    continue;
            }
        }
    }

    return TR_SUCCESS;
}

td_int32    tiny_db_node_deinit(td_int32 fd, mod_node_t *p_module){
    list_head_t *p_head = NULL;
    list_head_t *p_tmp  = NULL;
    used_node_t *p_node = NULL;

    if(!list_empty(&p_module->list_head)){
        list_for_each_safe(p_head,p_tmp,&p_module->list_head){
            p_node = list_entry(p_head, used_node_t, list);

            if(NULL != p_node){
                list_del(&p_node->list);
                tiny_db_free(p_node);
            }
        }
    }

    if(p_module->p_free_nodes){
        tiny_db_free(p_module->p_free_nodes);
    }

    tiny_db_module_deinit(fd, &p_module->module);

    return TR_SUCCESS;
}

td_int32    tiny_db_node_get_by_pos(td_int32 fd, mod_node_t *p_module, used_node_t *p_rec, td_char *buffer, td_int32 *buffer_len){
    td_int32 ret = TR_FAIL;
    td_uint16 node_pos = p_rec->node_pos;

    td_int32 offset = 0, node_head = 0, node_buff = 0;
    td_uint16 page_index = 0, node_index = 0, save_pos = 0;
    db_node_t header;
    td_int32 o_len = -1;
    td_uchar s_bf[p_module->node_length];

    ret = TR_FAIL;
    while(node_pos != INVALID_NODE_POS){
        page_index = node_pos / p_module->node_cnt_in_1_page;
        node_index = node_pos % p_module->node_cnt_in_1_page;

        offset = tiny_db_module_map(fd, &p_module->module, page_index);

        node_head = offset + NODE_MANAGE_START + node_index * sizeof(db_node_t);
        node_buff = offset + NODE_CONTENT_START + node_index * p_module->node_length;

        tiny_db_assert(tiny_db_OsSeek(fd, node_head) == TR_SUCCESS);
        tiny_db_assert(tiny_db_OsRead(fd, &header, sizeof(db_node_t)) == 4 && (header.node_state == RECORD_STATE_START || header.node_state == RECORD_STATE_UESD) && (header.node_id == p_rec->node_id));

        tiny_db_assert(tiny_db_OsSeek(fd, node_buff) == TR_SUCCESS);
        tiny_db_assert(tiny_db_OsRead(fd, s_bf, p_module->node_length) == p_module->node_length);

        node_pos = TD_MAKE_WORD(s_bf);

        if(header.node_state != RECORD_STATE_START && o_len == -1){
            TINY_DB_ERR("no start node %d\n", p_rec->node_id);
            return ret;
        }

        if(header.node_state == RECORD_STATE_START){
            td_uchar * p_b = &s_bf[NODE_NEXT_LEN];
            o_len = TD_MAKE_WORD(p_b);

            if(o_len > *buffer_len){
                TINY_DB_ERR("node length %d, buffer len %d\n", o_len, *buffer_len);
                return ret;
            }

            *buffer_len = o_len;

            if(o_len > p_module->node_length - NODE_NEXT_LEN - NODE_LENGTH_LEN){
                memcpy(&buffer[save_pos], &s_bf[NODE_NEXT_LEN + NODE_LENGTH_LEN], p_module->node_length - NODE_NEXT_LEN - NODE_LENGTH_LEN);
                save_pos += (p_module->node_length - NODE_NEXT_LEN - NODE_LENGTH_LEN);      
            }else{
                memcpy(&buffer[save_pos], &s_bf[NODE_NEXT_LEN + NODE_LENGTH_LEN], o_len);
                save_pos += o_len;
            }
            
        }else{
            td_int32 left = o_len - save_pos;

            if(left > p_module->node_length - NODE_NEXT_LEN){
                memcpy(&buffer[save_pos], &s_bf[NODE_NEXT_LEN ], p_module->node_length - NODE_NEXT_LEN);
                save_pos += (p_module->node_length - NODE_NEXT_LEN );      
            }else{
                memcpy(&buffer[save_pos], &s_bf[NODE_NEXT_LEN ], left);
                save_pos += left;
            }
        }
    }
    return TR_SUCCESS;
}

td_int32    tiny_db_node_get_by_id(td_int32 fd, mod_node_t *p_module, td_int16 node_id, td_char *buffer, td_int32 *buffer_len){
    td_int32 ret = TR_FAIL;
    used_node_t *p_rec = NULL;
    
    if(!list_empty(&p_module->list_head)){
        list_head_t *p_head = NULL;
        list_head_t *p_tmp  = NULL;

        list_for_each_safe(p_head,p_tmp,&p_module->list_head){
            p_rec = list_entry(p_head, used_node_t, list);

            if(NULL != p_rec){
                if(p_rec->node_id == node_id){
                    ret = TR_SUCCESS;
                    break;
                }         
            }
        }
    }

    if(ret == TR_SUCCESS){
        ret = tiny_db_node_get_by_pos(fd, p_module, p_rec, buffer, buffer_len);
    }      

    return ret;
}

td_int32    tiny_db_node_del_by_id(td_int32 fd, mod_node_t *p_module, td_int16 node_id){
    return _td_node_del_by_id(fd, p_module, node_id, NULL);
}

td_int32    tiny_db_node_del_by_pos(td_int32 fd, mod_node_t *p_module, used_node_t *p_rec){
    td_int32 ret = TR_FAIL;

    if(p_rec){
        list_del(&p_rec->list);
        _td_node_del_by_pos(fd, p_module, p_rec);
        tiny_db_free(p_rec);
    }      

    return ret;
}

//自增夹node id
//返回当前node id
td_int32    tiny_db_node_set(td_int32 fd, mod_node_t *p_module, td_char *buffer, td_int32 buffer_len){
    return _td_node_set(fd, p_module, buffer, buffer_len, NULL);
}

td_int32    tiny_db_node_update_by_id(td_int32 fd, mod_node_t *p_module, td_int16 node_id, td_char *buffer, td_int32 buffer_len){
    used_node_t *p_node = NULL;

    if(_td_node_del_by_id(fd, p_module, node_id, &p_node) == TR_SUCCESS && p_node){
        if(_td_node_set(fd, p_module, buffer, buffer_len, p_node) == TR_SUCCESS){
            return TR_SUCCESS;      
        }else{
            tiny_db_free(p_node);
            return TR_FAIL;
        }
    }
    TINY_DB_ERR("can't find node %d\n", node_id);
    return TR_FAIL;
}

td_int32    tiny_db_node_update_by_pos(td_int32 fd, mod_node_t *p_module, used_node_t *p_node, td_char *buffer, td_int32 buffer_len){
    
    if(p_node && _td_node_del_by_pos(fd, p_module, p_node) == TR_SUCCESS){
        if(_td_node_set(fd, p_module, buffer, buffer_len, p_node) == TR_SUCCESS){
            return TR_SUCCESS;      
        }else{
            tiny_db_free(p_node);
            return TR_FAIL;
        }
    }
    return TR_FAIL;
}

td_int32    tiny_db_node_destroy(td_int32 fd, mod_node_t *p_module){
    list_head_t *p_head = NULL;
    list_head_t *p_tmp  = NULL;
    used_node_t *p_node = NULL;

    if(!list_empty(&p_module->list_head)){
        list_for_each_safe(p_head,p_tmp,&p_module->list_head){
            p_node = list_entry(p_head, used_node_t, list);

            if(NULL != p_node){
                list_del(&p_node->list);
                tiny_db_free(p_node);
            }
        }
    }

    if(p_module->p_free_nodes){
        tiny_db_free(p_module->p_free_nodes);
    }

    tiny_db_module_delete(fd, &p_module->module);

    return TR_SUCCESS;
}

td_int32    tiny_db_node_get_start_offset_by_pos(td_int32 fd, mod_node_t *p_module, td_int16 node_pos){
    td_int32 page_index = node_pos / p_module->node_cnt_in_1_page;
    td_int32 node_index = node_pos % p_module->node_cnt_in_1_page, offset = 0;
    //(tbl_node_t *)table_buffer;
    offset = tiny_db_module_map(fd, &p_module->module, page_index);

    offset = offset + NODE_CONTENT_START + node_index * p_module->node_length;

    return offset + NODE_NEXT_LEN + NODE_LENGTH_LEN;
}