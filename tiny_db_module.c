
/*
    管理每个功能模块的PAGE使用，方便把不连续的page组装到一起
    */

/*

    模块表示 PageNext FF FF FF FF FF FF FF FF FF FF FF FF FF FF
    FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF

    ........

    FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF

    模块表示：两个字节表明属于哪个模块
    PageNext：FF FF 表示无下个PAGE，否则表示下个PAGE序号
*/




#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "sql_porting.h"
#include "tiny_db_priv.h"
#include "tiny_db_pager.h"
#include "tiny_db_module.h"

#define TABLE_INDEX_MODULE_ID 0 // 表索引模块ID是0，其他具体表从1开始
#define MODULE_ID_BYTES 2


//想要使用模块先创建ID
td_int32 tiny_db_module_require_id(td_int32 fd){
    td_int32 b_occupy = 0;
    td_uint32 offset = 0;
    td_uint16 module_id = 0;
    td_uchar s_mod_id[MODULE_ID_BYTES] = {0};

    tiny_db_pager_is_occupy(fd, TINY_PAGE_INDEX_TABLE, &b_occupy);

    if(b_occupy == 0){
        SQL_TINY_DB_ERR("table not init\n");
        return TR_FAIL;
    }

    offset = tiny_db_get_page_offset(fd, TINY_PAGE_INDEX_TABLE) + MODULE_PAGE_OFFSET;
    sql_tiny_db_OsSeek(fd, offset);
    sql_tiny_db_OsRead(fd, (void *)s_mod_id, MODULE_ID_BYTES);
    sql_tiny_db_OsSeek(fd, offset);

    module_id = TD_MAKE_WORD(s_mod_id);
    SQL_TINY_DB_DBG("new module id: %u\n", module_id);
    module_id ++;
    TD_WORD_SERIALIZE(s_mod_id, module_id);
    sql_tiny_db_OsWrite(fd, (void *)&module_id, MODULE_ID_BYTES);
    return module_id - 1;
}

//初始化指定module，即将指定moduleID的page管理起来
td_int32 tiny_db_module_init(td_int32 fd, td_mod_info_t *p_mod){
    td_int32 ret = TR_FAIL, offset = 0;
    td_uint16 val = 0, page_id = 0xFFFF;
    td_uchar *p_cursor = NULL;
    td_uchar * p_buffer = NULL;

    st_data_block_t block = {NULL};
    //td_mod_page_t page = {TINY_INVALID_PAGE_ID};
    td_uint16 page = TINY_INVALID_PAGE_ID;
    td_uchar s_buf[4] = {0};

    td_uint32 size = tiny_db_get_page_size(fd);

    if(p_mod->module_id == TABLE_INDEX_MODULE_ID){
        td_int32 b_occupy = 0;

        tiny_db_pager_is_occupy(fd, TINY_PAGE_INDEX_TABLE, &b_occupy);
        p_mod->first_page_id = TINY_PAGE_INDEX_TABLE;
        if(b_occupy == 0){
            p_buffer = sql_tiny_db_malloc(size);

            if(p_buffer){
                p_cursor = &p_buffer[0];
                TD_WORD_SERIALIZE(p_cursor, p_mod->module_id); //设置当前页面moduleID

                val = TINY_INVALID_PAGE_ID;
                p_cursor = &p_buffer[2];
                TD_WORD_SERIALIZE(p_cursor, val); //设置当前module下一页

                val = p_mod->module_id + 1; //设置下个moduleID
                p_cursor = &p_buffer[4];
                TD_WORD_SERIALIZE(p_cursor, val);

                memset(&p_buffer[6], 0xFF, size - 6);

                offset = tiny_db_get_page_offset(fd, TINY_PAGE_INDEX_TABLE);
                sql_tiny_db_OsSeek(fd, offset);
                if(sql_tiny_db_OsWrite(fd, (void *)p_buffer, size) == size){
                    //page.p_handle = NULL;
                    //page.page_id = TINY_PAGE_INDEX_TABLE;
                    page = TINY_PAGE_INDEX_TABLE;
                    
                    block.ppblock = (void **)&p_mod->p_pages;
                    block.current_cnt = (td_uint16 *)&p_mod->page_count;
                    block.block_cnt = (td_uint16 *)&p_mod->pages_buffer_size;
                    block.block_size = sizeof(td_uint16);//sizeof(td_mod_page_t);
                    block.enlarge_size = 2;
                    block.block = (void *)&page;

                    ret = sql_tiny_db_insert_block(&block);

                    if(ret == 0){
                        ret = tiny_db_pager_occupy_page(fd, TINY_PAGE_INDEX_TABLE);
                    }else{
                        SQL_TINY_DB_ERR("no memory\n");
                    }
                }else{
                    SQL_TINY_DB_ERR("table first page write fail\n");
                }
                
                sql_tiny_db_free(p_buffer);

                return ret;
            }else{
                SQL_TINY_DB_ERR("no memory\n");
                return ret;
            }
        }
    }

    if(p_mod->first_page_id == TINY_INVALID_PAGE_ID){ //刚require新的moduleid，没有申请page
        /*
        page_id = tiny_db_pager_malloc(fd);

        if(page_id == TINY_INVALID_PAGE_ID){
            SQL_TINY_DB_ERR("no pages\n");
            return ret;
        }

        p_mod->first_page_id = page_id;
        ret = tiny_db_module_enlarge(fd, p_mod);

        if(ret != TR_SUCCESS){
            tiny_db_pager_free(fd, page_id);
        }

        return ret;
        */

        return tiny_db_module_enlarge(fd, p_mod);
    }

    //page.p_handle = NULL;
    //page.page_id = p_mod->first_page_id;
    page = p_mod->first_page_id;

    memset(&block, 0, sizeof(block));

    block.ppblock = (void **)&p_mod->p_pages;
    block.current_cnt = (td_uint16 *)&p_mod->page_count;
    block.block_cnt = (td_uint16 *)&p_mod->pages_buffer_size;
    block.block_size = sizeof(td_uint16);//sizeof(td_mod_page_t);
    block.enlarge_size = 8;
    block.block = (void *)&page;

    do{
        offset = tiny_db_get_page_offset(fd, page);//.page_id);
        ret = sql_tiny_db_OsSeek(fd, offset);
        if(ret != TR_SUCCESS){
            tiny_db_pager_free(fd, page);//.page_id);
            break;
        }

        if(sql_tiny_db_OsRead(fd, (void *)s_buf, MODULE_ID_BYTES) != MODULE_PAGE_OFFSET){
            ret = TR_FAIL;
            break;
        }

        p_cursor = s_buf;
        val = TD_MAKE_WORD(p_cursor);

        p_cursor = &s_buf[2];
        page_id = TD_MAKE_WORD(p_cursor);

        if(val == p_mod->module_id){
            ret = sql_tiny_db_insert_block(&block);

            if(ret != TR_SUCCESS){
                SQL_TINY_DB_ERR("no memory\n");           
            }else{
                break;
            }
            page/*.page_id*/ = page_id;
        }else{
            ret = TR_FAIL;
            break;
        }
    }while(page/*.page_id*/ != TINY_INVALID_PAGE_ID);

    return ret;
}

//指定module，申请新的页面
td_int32 tiny_db_module_enlarge(td_int32 fd, td_mod_info_t *p_mod){
    td_int32 ret = TR_FAIL, offset = 0;
    td_uint16 val = 0;
    td_uchar *p_cursor = NULL;
    td_uchar * p_buffer = NULL;

    st_data_block_t block = {NULL};
    //td_mod_page_t page = {TINY_INVALID_PAGE_ID};
    td_uint16 page = TINY_INVALID_PAGE_ID;
    td_uchar s_buf[MODULE_PAGE_OFFSET] = {0};

    td_uint32 size = tiny_db_get_page_size(fd);
    td_uint16 page_id = tiny_db_pager_malloc(fd), prev_id = 0;

    if(page_id == TINY_INVALID_PAGE_ID){
        SQL_TINY_DB_ERR("no pages\n");
        return ret;
    }

    p_buffer = sql_tiny_db_malloc(size);

    if(p_buffer){
        p_cursor = &p_buffer[0];
        TD_WORD_SERIALIZE(p_cursor, p_mod->module_id); //设置当前页面moduleID

        val = TINY_INVALID_PAGE_ID;
        p_cursor = &p_buffer[2];
        TD_WORD_SERIALIZE(p_cursor, val); //设置当前module下一页

        memset(&p_buffer[4], 0xff, size - 4);

        offset = tiny_db_get_page_offset(fd, page_id);
        sql_tiny_db_OsSeek(fd, offset);
        if(sql_tiny_db_OsWrite(fd, (void *)p_buffer, size) == size){
            //page.p_handle = NULL;
            page/*.page_id*/ = page_id;
            
            block.ppblock = (void **)&p_mod->p_pages;
            block.current_cnt = (td_uint16 *)&p_mod->page_count;
            block.block_cnt = (td_uint16 *)&p_mod->pages_buffer_size;
            block.block_size = sizeof(td_uint16);//td_mod_page_t);
            block.enlarge_size = 8;
            block.block = (void *)&page;

            ret = sql_tiny_db_insert_block(&block);

            if(ret == 0){
                if(p_mod->first_page_id == TINY_INVALID_PAGE_ID){
                    p_mod->first_page_id = page_id;
                }else{
                    td_int32 b_occupy = 0;

                    sql_tiny_db_assert(p_mod->page_count >= 2);
                    //prev_id = p_mod->p_pages[p_mod->page_count - 2].page_id;
                    prev_id = p_mod->p_pages[p_mod->page_count - 2];
                    sql_tiny_db_assert(tiny_db_pager_is_occupy(fd, prev_id, &b_occupy) == TR_SUCCESS && b_occupy);

                    offset = tiny_db_get_page_offset(fd, prev_id);

                    sql_tiny_db_assert(sql_tiny_db_OsSeek(fd, offset) == TR_SUCCESS && sql_tiny_db_OsRead(fd, s_buf, MODULE_PAGE_OFFSET) == MODULE_PAGE_OFFSET);

                    p_cursor = &p_buffer[0];
                    val = TD_MAKE_WORD(p_cursor);

                    p_cursor = &p_buffer[2];
                    prev_id = TD_MAKE_WORD(p_cursor);

                    sql_tiny_db_assert(prev_id == TINY_INVALID_PAGE_ID && val == p_mod->module_id);
                    TD_WORD_SERIALIZE(p_cursor, page_id);

                    sql_tiny_db_OsSeek(fd, offset);
                    sql_tiny_db_OsWrite(fd, s_buf, MODULE_PAGE_OFFSET);
                }
                ret = TR_SUCCESS;
            }else{
                SQL_TINY_DB_ERR("no memory\n");
            }
        }else{
            SQL_TINY_DB_ERR("table first page write fail\n");
        }
    }else{
        SQL_TINY_DB_ERR("no memory\n");
    }

    if(ret != TR_SUCCESS){
        tiny_db_pager_free(fd, page_id);
    }

    if(p_buffer){
        sql_tiny_db_free(p_buffer);
    }

    return ret;
}


//删除指定module，即将指定moduleID的page删除
td_int32 tiny_db_module_delete(td_int32 fd, td_mod_info_t *p_mod){
    int i = 0;

    for(i = 0; i < p_mod->page_count; i ++){
        tiny_db_pager_free(fd, p_mod->p_pages[i]);//.page_id);
    }

    if(p_mod->p_pages){
        sql_tiny_db_free(p_mod->p_pages);
    }

    return TR_SUCCESS;
}


//module 的page index从0开始，返回真实的偏移, page index 指向的是module数据结构中p_pages的下标
td_int32 tiny_db_module_map(td_int32 fd, td_mod_info_t *p_mod, td_int32 page_index){
    td_uint32 size = tiny_db_get_page_size(fd);

    sql_tiny_db_assert(page_index >= p_mod->page_count);
    return tiny_db_get_page_offset(fd, p_mod->p_pages[page_index]);//.page_id);
}


//指定module，获取总共空间
td_int32 tiny_db_module_size(td_int32 fd, td_mod_info_t *p_mod){
    td_uint32 size = tiny_db_get_page_size(fd);

    return size * p_mod->page_count;
}
