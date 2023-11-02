/*
    数据库页管理。每页2K
    Page0 用于存储数据库页的状态，每页用1bit表示，0：被占用， 1： 未被使用
    Page1 用于描述TABLE信息

    tage_START  0xAA55AA55
    tage_END    0x55AA55AA

    最大管理 (2K - 8 - 16) * 32

    即每个数据库page0和1被占用
    */

/*
    AA 55 AA 55  CF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
    
    ......
    
    FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 55 AA 55 AA
*/

/*
    tage_START[4 bytes] [2000 bytes] [reserve 40 bytes] tage_END[0x55AA55AA]
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "sql_porting.h"
#include "tiny_db_priv.h"
#include "tiny_db_pager.h"

#define TINY_DB_PAGE_SIZE       2048
#define TINY_DB_START_TAG       0xAA55AA55
#define TINY_DB_END_TAG         0x55AA55AA
#define TINY_DB_START_BYTE      4
#define TINY_DB_VALID_BYTE      2000
#define TINY_DB_RESERVE         (2000 + TINY_DB_START_BYTE)

#define LINE_BYTE 20
#define LINE_COUNT 100 //(TINY_DB_PAGE_SIZE - TINY_DB_START_BYTE * 2)/ LINE_BYTE


td_int32 tiny_db_pager_init(td_int32 fd){
    td_int32    ret = TR_FAIL;
    td_int32    tag = 0;
    char *      p_buf = NULL;

    sql_tiny_db_OsSeek(fd, 0);
    sql_tiny_db_OsRead(fd, (void *)&tag, 4);

    if(tag == TINY_DB_START_TAG){
        sql_tiny_db_OsSeek(fd, TINY_DB_PAGE_SIZE - 4);
        sql_tiny_db_OsRead(fd, (void *)&tag, 4);

        if(tag == TINY_DB_END_TAG){
            return TR_SUCCESS;
        }
    }

    p_buf = sql_tiny_db_malloc(TINY_DB_PAGE_SIZE);

    if(p_buf == NULL){
        memset(&p_buf[4], 0xFF, TINY_DB_PAGE_SIZE - 8);
        tag = TINY_DB_START_TAG;
        memcpy(&p_buf[0], &tag, 4);

        tag = TINY_DB_END_TAG;
        memcpy(&p_buf[TINY_DB_PAGE_SIZE - 4], &tag, 4);
        
        TD_CLR_BIT(p_buf[TINY_DB_START_BYTE], 0);

        sql_tiny_db_OsSeek(fd, 0);
        
        if(sql_tiny_db_OsWrite(fd, p_buf, TINY_DB_PAGE_SIZE) == TINY_DB_PAGE_SIZE){
            ret = TR_SUCCESS;
        }

        sql_tiny_db_free(p_buf);

        tiny_db_pager_set_rev(fd, TD_PAGER_REV_TBL_LAST_NODE, 0);
    }

    return ret;
}


td_int16 tiny_db_pager_malloc(td_int32 fd){
    td_uchar    line[LINE_BYTE] = {0};
    td_uchar    full[LINE_BYTE] = {0};
    int         i = 0;

    sql_tiny_db_OsSeek(fd, TINY_DB_START_BYTE);
    for(; i < LINE_COUNT; i ++){
        if(LINE_BYTE != sql_tiny_db_OsRead(fd, (void *)line, LINE_BYTE)){
            return TINY_INVALID_PAGE_ID;
        }

        if(memcmp(line, full, LINE_BYTE) != 0){
            int j = 0;
            for(; j < LINE_BYTE; j ++){
                if(line[j] != 0){
                    int x = 0;

                    for(; x < 8; x ++){
                        if(i == 0 && j == 0 && x < 2){//skip TINY_PAGE_INDEX_MAN & TINY_PAGE_INDEX_TABLE
                            continue;
                        }

                        if(TD_GET_BIT(line[j], x)){
                            td_uint32 pos =  (i * LINE_BYTE) + j;

                            TD_CLR_BIT(line[j], x);
                            sql_tiny_db_OsSeek(fd, TINY_DB_START_BYTE + pos);
                            sql_tiny_db_OsWrite(fd, &line[j], 1);
                            return pos * 8 + x;
                        }            
                    }
                }
            }
        }
    }
    return TINY_INVALID_PAGE_ID;
}


td_int32 tiny_db_pager_free(td_int32 fd, td_int16 index){
    int total = TINY_DB_VALID_BYTE;
    int byte_index = index / 8;
    int byte_off = index % 8;
    td_uchar mask = 0;

    if(byte_index >= total){
        SQL_TINY_DB_ERR("total %d, curn %d\n", total, byte_index);
        return TR_FAIL;
    }

    sql_tiny_db_OsSeek(fd, TINY_DB_START_BYTE + byte_index);
    sql_tiny_db_OsRead(fd, (void *)&mask, 1);

    TD_SET_BIT(mask, byte_off);
    sql_tiny_db_OsSeek(fd, TINY_DB_START_BYTE + byte_index);

    return sql_tiny_db_OsWrite(fd, (void *)&mask, 1) == 1 ? TR_SUCCESS : TR_FAIL;
}


td_int32 tiny_db_get_page_offset(td_int32 fd, td_int32 index){
    if(index == 0 || (index / 8) >= TINY_DB_VALID_BYTE){
        SQL_TINY_DB_ERR("page %d invalid\n", index);
        return TR_FAIL;
    }

    return index * TINY_DB_PAGE_SIZE;
}


td_int32 tiny_db_get_page_size(td_int32 fd){
    return TINY_DB_PAGE_SIZE;
}

td_int32 tiny_db_pager_is_occupy(td_int32 fd, td_int32 index, td_int32 *b_occupy){
    int total = TINY_DB_VALID_BYTE;
    int byte_index = index / 8;
    int byte_off = index % 8;
    td_uchar mask = 0;

    if(byte_index >= total){
        SQL_TINY_DB_ERR("total %d, curn %d\n", total, byte_index);
        return TR_FAIL;
    }

    sql_tiny_db_OsSeek(fd, TINY_DB_START_BYTE + byte_index);
    sql_tiny_db_OsRead(fd, (void *)&mask, 1);

    *b_occupy = TD_GET_BIT(mask, byte_off) == 0;
    
    return TR_SUCCESS;
}

td_int32 tiny_db_pager_occupy_page(td_int32 fd, td_int16 page_id){
    td_uchar    mask = 0xFF;
    int         offset = 0, byte_off = page_id % 8;

    if(page_id <= 0){
        return TR_FAIL;
    }
    offset = TINY_DB_START_BYTE + page_id/8 ;
    sql_tiny_db_OsSeek(fd, offset);
    sql_tiny_db_OsRead(fd, (void *)&mask, 1);

    TD_CLR_BIT(mask, byte_off);

    sql_tiny_db_OsSeek(fd, offset);
 
    return sql_tiny_db_OsWrite(fd, (void *)&mask, 1) == 1 ? TR_SUCCESS : TR_FAIL;
}


td_int32 tiny_db_pager_get_rev(td_int32 fd, td_pager_rev_t rev_id, td_uchar * p_val){
    td_int32 offset = TINY_DB_VALID_BYTE + TINY_DB_START_BYTE + rev_id * TINY_REV_BYTES_LEN;

    sql_tiny_db_assert(sql_tiny_db_OsSeek(fd, offset) == TR_SUCCESS);
    sql_tiny_db_assert(sql_tiny_db_OsRead(fd, (void *)p_val, TINY_REV_BYTES_LEN) == TINY_REV_BYTES_LEN);

    return TR_SUCCESS;
}

td_int32 tiny_db_pager_set_rev(td_int32 fd, td_pager_rev_t rev_id, td_uchar *p_val){
    td_int32 offset = TINY_DB_VALID_BYTE + TINY_DB_START_BYTE + rev_id * TINY_REV_BYTES_LEN;

    sql_tiny_db_assert(sql_tiny_db_OsSeek(fd, offset) == TR_SUCCESS);
    sql_tiny_db_assert(sql_tiny_db_OsWrite(fd, (void *)p_val, TINY_REV_BYTES_LEN) == TINY_REV_BYTES_LEN);

    return TR_SUCCESS;
}