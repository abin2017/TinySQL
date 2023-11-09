
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "tiny_db_api.h"
#include "tiny_db_platform.h"

static int _tiny_db_dump_callback(void *data, int argc, char **argv, int *argv_len, int *argv_type, char **col_names){
    int i = 0;

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

void tiny_db_test_insert_test1(int fd){
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

        elements[0].content = (int *)i;
        elements[1].content = (int *)i;
        elements[7].content = (int *)i;
        elements[8].content = (int *)i;
        elements[9].content = (int *)i;
        elements[10].content = (int *)i;

        //srand(time(NULL));
        //int random_number = rand();

        column.count = 6 + i%6;//random_number%6;

        ret = tiny_db_api_insert_data(fd, "test1", &column, 0);
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

void tiny_db_test_dump_table_test1(int fd){
    tiny_db_api_show_info(fd, "test1", NULL, _tiny_db_dump_callback);
}

int main() {
    int fd = 0;

    fd = tiny_db_api_open("./test.db");
    tiny_db_assert(0 != fd);

    //tiny_db_test_create_table(fd);

    //tiny_db_test_insert_test1(fd);

    tiny_db_test_dump_table_test1(fd);

    tiny_db_api_close(fd);

    TINY_DB_MEMORY_USAGE();
    return 0;
}