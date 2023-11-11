/*
    sizeof(tbl_node_t) + [ (sizeof(tbl_item_t) + tbl_item_t.item_title_len) + ... (tbl_node_t.item_count) ] + 表名 + [项名，项名 ...]
    */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "tiny_db_api.h"

#include "list.h"
#include "tiny_db_platform.h"
#include "tiny_db_priv.h"
#include "tiny_db_pager.h"
#include "tiny_db_module.h"
#include "tiny_db_node.h"
#include "tiny_db_table.h"


typedef struct{
    int                 fd;
    int                 lock; 
    tbl_manage_t        entrance;
}tiny_db_t;

int tiny_db_api_open(char *path){
    tiny_db_t *p_this = NULL;

    p_this = tiny_db_malloc(sizeof(tiny_db_t));
    TD_TRUE_RETVAL(p_this == NULL, 0, "no memory\n");
    memset(p_this, 0, sizeof(tiny_db_t));

    p_this->fd = tiny_db_OsOpen(path);
    TD_TRUE_JUMP(p_this->fd == 0, _err, "%s open fail\n", path);

    tiny_db_OslockCreate(&p_this->lock, 1);

    TD_TRUE_JUMP(TR_SUCCESS != tiny_db_table_init(p_this->fd, &p_this->entrance), _err, "%s open fail\n", path);

    return (int)p_this;
    _err:
        if(p_this){
            if(p_this->lock){
                tiny_db_OslockDelete(p_this->lock);
            }

            if(p_this->fd){
                tiny_db_OsClose(p_this->fd);
            }

            tiny_db_free(p_this);
        }
        return 0;
}

void tiny_db_api_close(int handle){
    tiny_db_t *p_this = NULL;

    p_this = (tiny_db_t *)handle;

    if(p_this){
        tiny_db_table_deinit(p_this->fd, &p_this->entrance);

        if(p_this->fd){
            tiny_db_OsClose(p_this->fd);
        }

        if(p_this->lock){
            tiny_db_OslockDelete(p_this->lock);
        }

        tiny_db_free(p_this);
    }
}

int tiny_db_api_create_table(int handle, char *title, td_elem_list_t *p_column, int param){
    int ret = TD_FAIL, i = 0;
    tiny_db_t *p_this = (tiny_db_t *)handle;

    TD_TRUE_RETVAL(NULL == title || NULL == p_column || handle == 0, ret, "title %p, column %p, %x\n", title, p_column, handle);
    TD_TRUE_RETVAL(p_column->count == 0, ret, "column count %d\n", p_column->count);

    for(i = 0; i < p_column->count; i++){
        td_elem_t *p_elem = &p_column->p_elem[i];

        TD_TRUE_RETVAL(NULL == p_elem->p_tag, ret, "column[%d] tag is null\n", i);
    }

    return tiny_db_table_create(p_this->fd, &p_this->entrance, title, p_column);
}

int tiny_db_api_delete_table(int handle, char *title, int param){
    tiny_db_t *p_this = (tiny_db_t *)handle;

    TD_TRUE_RETVAL(NULL == title, TD_FAIL, "title %p\n", title);

    return tiny_db_table_destroy(p_this->fd, &p_this->entrance, title);   
}

int tiny_db_api_edit_table(int handle, char *title, td_elem_list_t *p_column, int param){

    TINY_DB_ERR("Not support edit\n");
    return TD_FAIL;   
}

int tiny_db_api_insert_data(int handle, char *title, td_elem_list_t *p_elements, int param){
    tiny_db_t *p_this = (tiny_db_t *)handle;

    TD_TRUE_RETVAL(NULL == title, TD_FAIL, "title %p\n", title);
    TD_TRUE_RETVAL(NULL == p_elements || NULL == p_elements->p_elem, TD_FAIL, "param error\n");

    return tiny_db_table_insert(p_this->fd, &p_this->entrance, title, p_elements->p_elem, p_elements->count);
}

int tiny_db_api_delete_data(int handle, char *title, td_condition_t *p_cond, int param){
    tiny_db_t *p_this = (tiny_db_t *)handle;

    TD_TRUE_RETVAL(NULL == title, TD_FAIL, "title %p\n", title);
    TD_TRUE_RETVAL(NULL == p_cond || 0 >= p_cond->count, TD_FAIL, "param error\n");

    return tiny_db_table_delete(p_this->fd, &p_this->entrance, title, p_cond);
}

int tiny_db_api_update_data(int handle, char *title, td_elem_list_t *p_elements, td_condition_t *p_cond, int param){
    tiny_db_t *p_this = (tiny_db_t *)handle;

    TD_TRUE_RETVAL(NULL == title, TD_FAIL, "title %p\n", title);
    TD_TRUE_RETVAL(NULL == p_elements || NULL == p_elements->p_elem || NULL == p_cond, TD_FAIL, "param error\n");

    return tiny_db_table_update(p_this->fd, &p_this->entrance, title, p_elements, p_cond);
}

int tiny_db_api_select_count(int handle, char *title, td_select_t *p_select, int param){
    tiny_db_t *p_this = (tiny_db_t *)handle;

    TD_TRUE_RETVAL(NULL == title, 0, "title %p\n", title);

    return tiny_db_table_select_count(p_this->fd, &p_this->entrance, title, p_select);
}

int tiny_db_api_select_data(int handle, char *title, td_elem_list_t *p_elements, tiny_db_callback callback, td_select_t *p_select, void *p_data){
    tiny_db_t *p_this = (tiny_db_t *)handle;

    TD_TRUE_RETVAL(NULL == title || NULL == callback, 0, "title %p\n", title);

    return tiny_db_table_select_data(p_this->fd, &p_this->entrance, title, p_elements, callback, p_select, p_data);
}

int tiny_db_api_show_info(int handle, char *title, void *p_data, tiny_db_callback callback){
    tiny_db_t *p_this = (tiny_db_t *)handle;

    TD_TRUE_RETVAL(NULL == title, 0, "title %p\n", title);

    return tiny_db_table_show_info(p_this->fd, &p_this->entrance, title, p_data, callback);
}

void tiny_db_api_lock(int handle){
    tiny_db_t *p_this = (tiny_db_t *)handle;
    
    tiny_db_OsLock(p_this->lock);
}

void tiny_db_api_unlock(int handle){
    tiny_db_t *p_this = (tiny_db_t *)handle;
    
    tiny_db_OsUnlock(p_this->lock);
}