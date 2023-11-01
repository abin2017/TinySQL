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


#ifndef _TINY_DB_PAGER
#define _TINY_DB_PAGER

#define TINY_PAGE_INDEX_MAN     0
#define TINY_PAGE_INDEX_TABLE   1

#define TINY_INVALID_PAGE_ID 0xFFFF

#define TR_SUCCESS  0
#define TR_FAIL     -1


//用于数据库创建，用于重置所有page状态，打上TAG tage_START/tage_END
// page 0 需要SET 1
//用于初始化数据库page，更加tag判断数据库是否合法
td_int32 tiny_db_pager_init(td_int32 fd);


//返回pager index，从2开始
td_int16 tiny_db_pager_malloc(td_int32 fd);


//用于初始化数据库page，更加tag判断数据库是否合法
td_int32 tiny_db_pager_free(td_int32 fd, td_int16 index);


//用于初始化数据库page偏移
td_int32 tiny_db_get_page_offset(td_int32 fd, td_int32 index);


//用于初始化数据库page 大小
td_int32 tiny_db_get_page_size(td_int32 fd);

//判断page index是否被占用
td_int32 tiny_db_pager_is_occupy(td_int32 fd, td_int32 index, td_int32 *b_occupy);

//强行占用指定页面
td_int32 tiny_db_pager_occupy_page(td_int32 fd, td_int16 page_id);

#endif
