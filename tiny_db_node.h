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


#ifndef _TINY_DB_NODE
#define _TINY_DB_NODE

//RECORD ID 4 bytes, 递增，从0开始

typedef struct{
    td_uint32 node_state : 4;
    td_uint32 node_id : 24;
    td_uint32 rev : 4;
}db_node_t;

//可能多个node拼凑一个Record
#define RECORD_STATE_FREE       0b11
#define RECORD_STATE_START      0b10
#define RECORD_STATE_UESD       0b00


td_int32     tiny_db_node_init(td_int32 module_id);

td_int32 *   tiny_db_node_get_all(td_int32 module_id);

td_int32     tiny_db_node_get_by_id(td_int32 module_id, td_char *buffer, td_int32 buffer_len);


//make buffer
//parse buffer

#endif
