
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "tiny_db_api.h"
#include "tiny_db_platform.h"

static int _tiny_db_dump_callback(void *data, int argc, char **argv, int *argv_len, int *argv_type, char **col_names){
    int i = 0;
    int *phandle = (int *)data;
    if(phandle && *phandle){
        *phandle = 0;

        tiny_db_printf("\e[1;34m[TITLE]: ");

        for(i = 0; i < argc; i++){
            tiny_db_printf("%s\t", col_names[i]);
        }
        tiny_db_printf("\033[0m\n");
    }

    tiny_db_printf("[START]: ");
    for(i = 0; i < argc; i++){
    
        if(argv_type[i] == TD_ELEM_TYPE_INTEGER || argv_type[i] == TD_ELEM_TYPE_AUTO_INCREASE){
            if(argv_len[i] == 0){
                tiny_db_printf("(null)\t");
            }else{
                tiny_db_printf("%d\t", (int)argv[i]);
            }
        }else{
            if(argv[i] == NULL){
                tiny_db_printf("(null)\t");
            }else{
                if(argv_len[i] == 0){
                    tiny_db_printf("[len 0]\t");
                }else{
                    char bak = argv[i][argv_len[i]];
                    argv[i][argv_len[i]] = 0;
                    tiny_db_printf("%s\t", argv[i]);
                    argv[i][argv_len[i]] = bak;
                }
            }
        }
    }
    tiny_db_printf("\n");

    return 0;
}

void tiny_db_test_insert_test(int fd){
    td_elem_list_t      column = {0};
    td_elem_t           elements[12];  
    int ret = TD_SUCCESS;

    column.count = 11;
    column.p_elem = elements;

    elements[2].type = TD_ELEM_TYPE_STRING_FIXED;
    elements[3].type = TD_ELEM_TYPE_STRING;
    elements[4].type = TD_ELEM_TYPE_STRING;
    elements[5].type = TD_ELEM_TYPE_STRING;
    elements[6].type = TD_ELEM_TYPE_STRING;
    elements[7].type = TD_ELEM_TYPE_INTEGER;
    elements[8].type = TD_ELEM_TYPE_INTEGER;
    elements[9].type = TD_ELEM_TYPE_INTEGER;
    elements[10].type = TD_ELEM_TYPE_INTEGER;
    elements[0].type = TD_ELEM_TYPE_INTEGER;
    elements[1].type = TD_ELEM_TYPE_INTEGER;


    char channel_url[64] = "aa-channel_url_test01234567890123456789abcdefghijk";
    char timeshift[32] = "aa-timeshift_test";
    char prefix[32] = "aa-prefix_test";
    char name[32] = "aa-name_test";
    char c_type[32] = "aa-c_type_test";
    int i = 0;

    elements[2].p_tag = "channel_url";
    elements[2].content = (int *)channel_url;
    elements[3].p_tag = "timeshift";
    elements[3].content = (int *)timeshift;
    elements[4].p_tag = "prefix";
    elements[4].content = (int *)prefix;
    elements[5].p_tag = "name";
    elements[5].content = (int *)name;
    elements[6].p_tag = "c_type";
    elements[6].content = (int *)c_type;

    elements[0].p_tag = "type";
    elements[0].content = (int *)1;
    elements[1].p_tag = "_order";
    elements[1].content = (int *)1;
    elements[7].p_tag = "sub";
    elements[7].content = (int *)1;
    elements[8].p_tag = "fav";
    elements[8].content = (int *)1;
    elements[9].p_tag = "auth";
    elements[9].content = (int *)1;
    elements[10].p_tag = "reply";
    elements[10].content = (int *)1;

    for(i = 1; i < 244; i ++){
        if(channel_url[0] == 'z'){
            channel_url[0] = 'a';
            channel_url[1] = channel_url[1] + 1;
        }else{
            channel_url[0] = channel_url[0] + 1;
        }

        memcpy(timeshift, channel_url, 2);
        memcpy(prefix, channel_url, 2);
        memcpy(name, channel_url, 2);
        memcpy(c_type, channel_url, 2);

        elements[0].content = (int *)(i%5);
        elements[1].content = (int *)(i%10);
        elements[7].content = (int *)(i%20);
        elements[8].content = (int *)i;
        elements[9].content = (int *)i;
        elements[10].content = (int *)i;

        //srand(time(NULL));
        //int random_number = rand();

        column.count = 6 + i%6;//random_number%6;

        ret = tiny_db_api_insert_data(fd, "test1", &column, 0);
        tiny_db_assert(ret == TD_SUCCESS);
    }

    elements[0].p_tag = "id";
    elements[1].p_tag = "language_name";
    elements[2].p_tag = "flag";

    strcpy(channel_url, "aa-language_name_test01234567890123456789abcdefghijk");
    strcpy(name, "aa-flag_tes");
    i = 1;
    for(i = 1; i < 244; i ++){
        elements[0].content = (int *)(i%5);
        elements[1].content = (int *)channel_url;
        elements[2].content = (int *)name;

        if(channel_url[0] == 'z'){
            channel_url[0] = 'a';
            channel_url[1] = channel_url[1] + 1;
        }else{
            channel_url[0] = channel_url[0] + 1;
        }
        memcpy(name, channel_url, 2);

        column.count = 2 + i%2;

        ret = tiny_db_api_insert_data(fd, "test3", &column, 0);
        tiny_db_assert(ret == TD_SUCCESS);
    }    
}

void tiny_db_test_create_table(int fd){
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

    ret = tiny_db_api_create_table(fd, "test1", &column, 0);
    tiny_db_assert(ret == TD_SUCCESS);

    column.count = 10;
    column.p_elem = elements;

    elements[0].type = TD_ELEM_TYPE_AUTO_INCREASE;
    elements[0].p_tag = "id";
    elements[1].type = TD_ELEM_TYPE_INTEGER;
    elements[1].p_tag = "countTimeShiftOn";
    elements[2].type = TD_ELEM_TYPE_STRING_FIXED;
    elements[2].p_tag = "icon";
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
    elements[7].p_tag = "countStreamOn";
    elements[8].type = TD_ELEM_TYPE_INTEGER;
    elements[8].p_tag = "position";
    elements[9].type = TD_ELEM_TYPE_INTEGER;
    elements[9].p_tag = "position";

    ret = tiny_db_api_create_table(fd, "test2", &column, 0);
    tiny_db_assert(ret == TD_SUCCESS);

    column.count = 3;
    column.p_elem = elements;

    elements[0].type = TD_ELEM_TYPE_AUTO_INCREASE;
    elements[0].p_tag = "id";
    elements[1].type = TD_ELEM_TYPE_STRING_FIXED;
    elements[1].p_tag = "language_name";
    elements[1].content = (int *)48;
    elements[2].type = TD_ELEM_TYPE_STRING;
    elements[2].p_tag = "flag";

    ret = tiny_db_api_create_table(fd, "test3", &column, 0);
    tiny_db_assert(ret == TD_SUCCESS);

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

    ret = tiny_db_api_create_table(fd, "test4", &column, 0);
    tiny_db_assert(ret == TD_SUCCESS);
}

void tiny_db_test_dump_table_test(int fd){
    tiny_db_api_show_info(fd, "test1", NULL, _tiny_db_dump_callback);
    tiny_db_api_show_info(fd, "test3", NULL, _tiny_db_dump_callback);
}




#if 0
int main() {
    int fd = 0;

    fd = tiny_db_api_open("./test.db");
    tiny_db_assert(0 != fd);

    //tiny_db_test_create_table(fd);

    //tiny_db_test_insert_test(fd);

    tiny_db_test_dump_table_test(fd);



    tiny_db_api_close(fd);

    
    return 0;
}
#endif

int test_tiny_open_database(){
    return tiny_db_api_open("./test.db");
}

void test_tiny_close_database(int fd, int be_del){
    tiny_db_api_close(fd);

    if(be_del){
        remove("./test.db");
    }
}

void test_tiny_dump_table(int fd, char *table){
    tiny_db_api_show_info(fd, table, NULL, _tiny_db_dump_callback);
}


void test_tiny_select_count(int fd){
    td_select_t select;
    td_cond_elem_t cond_elem[2];
    int count = 0;

    memset(&select, 0, sizeof(select));
    memset(cond_elem, 0, sizeof(td_cond_elem_t) * 2);
    
    count = tiny_db_api_select_count(fd, "test1", NULL, 0);
    TINY_DB_DBG("test1 total count %d\n", count);

    select.cond.p_elements = cond_elem;
    select.cond.count = 1;
    select.cond.logic = TD_LOGIC_AND;

    cond_elem[0].arithmetic = TD_ARITHMETIC_EQUAL;
    cond_elem[0].p_tag = "type";
    cond_elem[0].content = (int *)4;

    count = tiny_db_api_select_count(fd, "test1", &select, 0);
    TINY_DB_DBG("test1 'type'==4 count %d\n", count);

    select.cond.count = 2;
    cond_elem[1].arithmetic = TD_ARITHMETIC_EQUAL;
    cond_elem[1].p_tag = "_order";
    cond_elem[1].content = (int *)4;
    count = tiny_db_api_select_count(fd, "test1", &select, 0);
    TINY_DB_DBG("test1 'type'==4 and '_order'==4 count %d\n", count);

    select.cond.count = 2;
    cond_elem[1].arithmetic = TD_ARITHMETIC_UNEQUAL;
    cond_elem[1].p_tag = "_order";
    cond_elem[1].content = (int *)4;
    count = tiny_db_api_select_count(fd, "test1", &select, 0);
    TINY_DB_DBG("test1 'type'==4 and '_order'!=4 count %d\n", count);
}

void test_tiny_select_data(int fd){
    td_select_t select;
    td_cond_elem_t cond_elem[3];
    int ret = TD_SUCCESS;
    int show_title = 0;
    memset(&select, 0, sizeof(select));
    memset(cond_elem, 0, sizeof(td_cond_elem_t) * 3);

    select.cond.p_elements = cond_elem;
    select.cond.count = 1;
    select.cond.logic = TD_LOGIC_AND;
    select.limit_count = TD_NO_LIMIT;
    
    cond_elem[0].arithmetic = TD_ARITHMETIC_EQUAL;
    cond_elem[0].p_tag = "type";
    cond_elem[0].content = (int *)4;

    show_title = 1;
    ret = tiny_db_api_select_data(fd, "test1", NULL, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 'type'==4  %d\n", ret);

    show_title = 1;
    select.cond.logic = TD_LOGIC_OR;
    select.cond.count = 2;
    cond_elem[1].arithmetic = TD_ARITHMETIC_EQUAL;
    cond_elem[1].p_tag = "timeshift";
    cond_elem[1].content = (int *)"ca-timeshift_test";
    ret = tiny_db_api_select_data(fd, "test1", NULL, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 'type'==4 or timeshift=ca-timeshift_test %d\n", ret);


    select.cond.logic = TD_LOGIC_AND;
    select.cond.count = 1;

    show_title = 1;
    select.limit_count = 5;
    ret = tiny_db_api_select_data(fd, "test1", NULL, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 'limit'=%d 'type'=4  %d\n", select.limit_count, ret);

    show_title = 1;
    select.limit_count = 10;
    select.order.type = TD_ORDER_DESC;
    select.order.p_tag = "id";
    ret = tiny_db_api_select_data(fd, "test1", NULL, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 ret=%d 'limit'=%d 'type'=4, order=%s, tag=%s\n", ret, select.limit_count, select.order.type == TD_ORDER_DESC ? "DESC" : "ASC", select.order.p_tag);

    show_title = 1;
    select.limit_count = 30;
    select.order.type = TD_ORDER_ASC;
    select.order.p_tag = "_order";
    ret = tiny_db_api_select_data(fd, "test1", NULL, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 ret=%d 'limit'=%d 'type'=4, order=%s, tag=%s\n", ret, select.limit_count, select.order.type == TD_ORDER_DESC ? "DESC" : "ASC", select.order.p_tag);

    show_title = 1;
    select.limit_count = TD_NO_LIMIT;
    select.order.type = TD_ORDER_ASC;
    select.order.p_tag = "sub";
    select.cond.count = 2;
    cond_elem[1].arithmetic = TD_ARITHMETIC_EQUAL;
    cond_elem[1].p_tag = "_order";
    cond_elem[1].content = (int *)4;
    ret = tiny_db_api_select_data(fd, "test1", NULL, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 ret=%d 'limit'=%d 'type'=4, order=%s, tag=%s\n", ret, select.limit_count, select.order.type == TD_ORDER_DESC ? "DESC" : "ASC", select.order.p_tag);

    
    show_title = 1;
    select.limit_count = TD_NO_LIMIT;
    select.order.type = TD_ORDER_ASC;
    select.order.p_tag = "sub";
    select.cond.count = 2;
    cond_elem[1].arithmetic = TD_ARITHMETIC_UNEQUAL;
    cond_elem[1].p_tag = "timeshift";
    cond_elem[1].content = (int *)"vi-timeshift_test";
    ret = tiny_db_api_select_data(fd, "test1", NULL, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 ret=%d 'limit'=%d 'type'=4, order=%s, tag=%s\n", ret, select.limit_count, select.order.type == TD_ORDER_DESC ? "DESC" : "ASC", select.order.p_tag);

    show_title = 1;
    select.limit_count = TD_NO_LIMIT;
    select.order.type = TD_ORDER_ASC;
    select.order.p_tag = "sub";
    select.cond.count = 2;
    cond_elem[1].arithmetic = TD_ARITHMETIC_UNEQUAL;
    cond_elem[1].p_tag = "_order";
    cond_elem[1].content = (int *)4;
    ret = tiny_db_api_select_data(fd, "test1", NULL, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 ret=%d 'limit'=%d 'type'=4, order=%s, tag=%s\n", ret, select.limit_count, select.order.type == TD_ORDER_DESC ? "DESC" : "ASC", select.order.p_tag);



    show_title = 1;
    select.cond.count = 1;
    td_elem_list_t s_list;
    td_elem_t      s_elems[6];
    
    s_list.count = 6;
    s_list.p_elem = s_elems;
    s_elems[0].type = TD_ELEM_TYPE_AUTO_INCREASE;
    s_elems[0].p_tag = "id";
    s_elems[1].type = TD_ELEM_TYPE_STRING_FIXED;
    s_elems[1].p_tag = "channel_url";
    s_elems[2].type = TD_ELEM_TYPE_STRING;
    s_elems[2].p_tag = "timeshift";
    s_elems[3].type = TD_ELEM_TYPE_INTEGER;
    s_elems[3].p_tag = "sub";
    s_elems[4].type = TD_ELEM_TYPE_INTEGER;
    s_elems[4].p_tag = "type";
    s_elems[5].type = TD_ELEM_TYPE_STRING;
    s_elems[5].p_tag = "c_type";
    ret = tiny_db_api_select_data(fd, "test1", &s_list, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 ret=%d 'limit'=%d 'type'=4, order=%s, tag=%s\n", ret, select.limit_count, select.order.type == TD_ORDER_DESC ? "DESC" : "ASC", select.order.p_tag);


    show_title = 1;
    select.cond.count = 2;
    cond_elem[1].arithmetic = TD_ARITHMETIC_EQUAL_NULL;
    cond_elem[1].p_tag = "sub";
    cond_elem[1].content = (int *)4;
    ret = tiny_db_api_select_data(fd, "test1", &s_list, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 ret=%d 'limit'=%d 'type'=4, order=%s, tag=%s\n", ret, select.limit_count, select.order.type == TD_ORDER_DESC ? "DESC" : "ASC", select.order.p_tag);


    show_title = 1;
    select.cond.count = 3;
    cond_elem[2].arithmetic = TD_ARITHMETIC_UNEQUAL_NULL;
    cond_elem[2].p_tag = "c_type";
    cond_elem[2].content = (int *)4;
    ret = tiny_db_api_select_data(fd, "test1", &s_list, _tiny_db_dump_callback, &select, &show_title);
    TINY_DB_DBG("test1 ret=%d 'limit'=%d 'type'=4, order=%s, tag=%s\n", ret, select.limit_count, select.order.type == TD_ORDER_DESC ? "DESC" : "ASC", select.order.p_tag);
}

void test_tiny_update_data(int fd, int id){
    int ret = TD_SUCCESS;
    td_condition_t cond;
    td_elem_list_t elements;
    td_cond_elem_t c_item[1];
    td_elem_t      m_item[2];

    char t_str[96] = {0};

    memset(&cond, 0, sizeof(td_condition_t));
    memset(&elements, 0, sizeof(td_elem_list_t));
    memset(c_item, 0, sizeof(td_cond_elem_t) * 1);

    cond.p_elements = c_item;
    cond.count = 1;
    cond.logic = TD_LOGIC_AND;

    c_item[0].arithmetic = TD_ARITHMETIC_EQUAL;
    c_item[0].p_tag = "id";
    c_item[0].content = (int *)id;

    elements.count = 2;
    elements.p_elem = m_item;

    m_item[0].type = TD_ELEM_TYPE_INTEGER;
    m_item[0].p_tag = "reply";
    m_item[0].content = (int *)(id * 100);

    m_item[1].type = TD_ELEM_TYPE_STRING_FIXED;
    m_item[1].p_tag = "c_type";
    m_item[1].content = (int *)t_str;

    snprintf(t_str, 95, "modify c_type %d abcdefg123456789abcdefg123456789abcdefg123456789", id);

    ret = tiny_db_api_update_data(fd, "test1", &elements, &cond, 0);
    TINY_DB_WARN("test1 ret=%d 'nodeId'=%d\n", ret, id);

    id += 2;
    c_item[0].p_tag = "_order";
    c_item[0].content = (int *)id;

    m_item[0].p_tag = "reply";
    m_item[0].content = (int *)(id * 100);

    m_item[1].type = TD_ELEM_TYPE_STRING;
    m_item[1].p_tag = "channel_url";
    m_item[1].content = (int *)t_str;

    snprintf(t_str, 95, "modify channel_url %d abcdefg123456789abcdefg123456789abcdefg123456789", id);

    ret = tiny_db_api_update_data(fd, "test1", &elements, &cond, 0);
    TINY_DB_WARN("test1 ret=%d '_order'=%d\n", ret, id);
}
