
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "tiny_db_api.h"
#include "tiny_db_platform.h"


static void tiny_db_test_create_table(int fd){
    td_elem_list_t      column = {0};
    td_elem_t           elements[12];  
    int ret = TD_SUCCESS;

    memset(elements, 0, sizeof(td_elem_t) * 10);

    column.count = 12;
    column.p_elem = elements;

    elements[0].type = TD_ELEM_TYPE_AUTO_INCREASE;
    elements[0].p_tag = "id";
    elements[1].type = TD_ELEM_TYPE_INTEGER;
    elements[1].p_tag = "_order";
    elements[2].type = TD_ELEM_TYPE_STRING_FIXED;
    elements[2].p_tag = "channel_url";
    elements[2].content = (int *)48;
    elements[3].type = TD_ELEM_TYPE_STRING;
    elements[3].p_tag = "timeshift";
    elements[4].type = TD_ELEM_TYPE_STRING;
    elements[4].p_tag = "prefix";
    elements[5].type = TD_ELEM_TYPE_STRING;
    elements[5].p_tag = "name";
    elements[6].type = TD_ELEM_TYPE_STRING;
    elements[6].p_tag = "c_type";
    elements[7].type = TD_ELEM_TYPE_INTEGER;
    elements[7].p_tag = "sub";
    elements[8].type = TD_ELEM_TYPE_INTEGER;
    elements[8].p_tag = "fav";
    elements[9].type = TD_ELEM_TYPE_INTEGER;
    elements[9].p_tag = "auth";
    elements[10].type = TD_ELEM_TYPE_INTEGER;
    elements[10].p_tag = "reply";
    elements[11].type = TD_ELEM_TYPE_INTEGER;
    elements[11].p_tag = "type";

    ret = tiny_db_api_create_table(fd, "TBL_ONLINETV_chn", &column, 0);
    tiny_db_assert(ret == TD_SUCCESS);
}

int main() {
    int fd = 0;

    fd = tiny_db_api_open("./test.db");
    tiny_db_assert(0 != fd);

    tiny_db_test_create_table(fd);

    tiny_db_api_close(fd);

    TINY_DB_MEMORY_USAGE();
    return 0;
}