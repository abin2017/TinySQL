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

#ifndef _TINY_DB_NODE
#define _TINY_DB_NODE

//RECORD ID 4 bytes, 递增，从0开始

typedef struct{
    td_uint32 node_state : 4;
    td_uint32 node_id : 24;
    td_uint32 rev : 4;
}db_node_t;

typedef struct{
    td_uint32 node_id ;          // node id 一个递增值，中间可能会隔断，即中间node被删除了
    td_uint32 node_pos ;   // 存的是node位置连续序号
    list_head_t list;
}used_node_t;

typedef struct{
    td_uchar        node_length;
    td_uchar        node_cnt_in_1_page;
    td_uint16       last_node_id;

    td_uint16       free_node_count; // already saved node count
    td_uint16       free_node_buffer_size; // number of nodes buffer size
    td_uint16 *     p_free_nodes; //保存空闲列表，保存的是node位置连续序号，从0开始     序号/（一页 node 数目） = 在第几页（moudle中的数组下标）
    
    list_head_t     list_head;
    td_uint16       used_count;

    td_mod_info_t   module;
}mod_node_t;

td_int32    tiny_db_node_init(td_int32 fd, mod_node_t *p_module);

td_int32    tiny_db_node_deinit(td_int32 fd, mod_node_t *p_module);

td_int32    tiny_db_node_get_by_pos(td_int32 fd, mod_node_t *p_module, used_node_t *p_rec, td_char *buffer, td_int32 *buffer_len);

td_int32    tiny_db_node_get_by_id(td_int32 fd, mod_node_t *p_module, td_int16 node_id, td_char *buffer, td_int32 *buffer_len);

td_int32    tiny_db_node_del_by_id(td_int32 fd, mod_node_t *p_module, td_int16 node_id);

td_int32    tiny_db_node_set(td_int32 fd, mod_node_t *p_module, td_char *buffer, td_int32 buffer_len);

td_int32    tiny_db_node_destroy(td_int32 fd, mod_node_t *p_module);

td_int32    tiny_db_node_update_by_id(td_int32 fd, mod_node_t *p_module, td_int16 node_id, td_char *buffer, td_int32 buffer_len);

td_int32    tiny_db_node_update_by_pos(td_int32 fd, mod_node_t *p_module, used_node_t *p_node, td_char *buffer, td_int32 buffer_len);

#endif
