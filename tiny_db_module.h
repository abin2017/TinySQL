
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




#ifndef _TINY_DB_MODULE_
#define _TINY_DB_MODULE_


#define MODULE_PAGE_OFFSET 4


typedef struct{
    td_uint16 page_id; //保存page index
    void * p_handle;
}td_mod_page_t;

typedef struct{
    td_uint16       module_id;
    td_uint16       first_page_id;
    td_uint16       page_count; // already saved page count
    td_uint16       pages_buffer_size; // number of pages buffer size
    td_mod_page_t * p_pages; //保存page info
}td_mod_info_t;

//想要使用模块先创建ID
td_int32 tiny_db_module_require_id(td_int32 fd);

//初始化指定module，即将指定moduleID的page管理起来
td_int32 tiny_db_module_init(td_int32 fd, td_mod_info_t *p_mod);

//删除指定module，即将指定moduleID的page删除
td_int32 tiny_db_module_delete(td_int32 fd, td_mod_info_t *p_mod);

//指定module，申请新的页面
td_int32 tiny_db_module_enlarge(td_int32 fd, td_mod_info_t *p_mod);

//module 的page index从0开始，返回真实的偏移, page index 指向的是module数据结构中p_pages的下标
td_int32 tiny_db_module_map(td_int32 fd, td_mod_info_t *p_mod, td_int32 page_index);

//指定module，获取总共空间
td_int32 tiny_db_module_size(td_int32 fd, td_mod_info_t *p_mod);

#endif