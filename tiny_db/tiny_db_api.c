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
    tbl_manage_t        entrance;
}tiny_db_t;

int tiny_db_open(char *path){
    tiny_db_t *p_this = NULL;

    p_this = tiny_db_malloc(sizeof(tiny_db_t));
    if(p_this == NULL){
        TINY_DB_ERR("no memory\n");
        return 0;
    }
    memset(p_this, 0, sizeof(tiny_db_t));

    p_this->fd = tiny_db_OsOpen(path);
    if(p_this->fd == 0){
        TINY_DB_ERR("%s open fail\n", path);
        goto _err;
    }

    if(TR_SUCCESS != tiny_db_table_init(p_this->fd, &p_this->entrance)){
        TINY_DB_ERR("%s open fail\n", path);
        goto _err;
    }

    return (int)p_this;
    _err:
        if(p_this){
            if(p_this->fd){
                tiny_db_OsClose(p_this->fd);
            }
            tiny_db_free(p_this);
        }
        return 0;
}

void tiny_db_close(int handle){
    tiny_db_t *p_this = NULL;

    p_this = (tiny_db_t *)handle;

    if(p_this){
        tiny_db_table_deinit(p_this->fd, &p_this->entrance);

        if(p_this->fd){
            tiny_db_OsClose(p_this->fd);
        }

        tiny_db_free(p_this);
    }
}
