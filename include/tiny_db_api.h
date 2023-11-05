#ifndef __X_TINY_DB_API_H__
#define __X_TINY_DB_API_H__

#define TD_SUCCESS 0
#define TD_FAIL -1

typedef enum{
    TD_ELEM_TYPE_STRING = 0,
    TD_ELEM_TYPE_INTEGER = 1,
    TD_ELEM_TYPE_AUTO_INCREASE = 2,
    TD_ELEM_TYPE_STRING_FIXED = 3,
}td_etype_t;

typedef struct{
    td_etype_t      type;
    char    *       p_tag;
    int     *       content;
}td_elem_t;

typedef struct{
    int             count; /* number of elements */
    td_elem_t   *   p_elem; /* element */
}td_elem_list_t;

/*
    0: 打开文件失败
    other： 成功
    */
int tiny_db_api_open(char *path);

void tiny_db_api_close(int handle);

int tiny_db_api_create_table(int handle, char *title, td_elem_list_t *p_column, int param);

int tiny_db_api_delete_table(int handle, char *title, int param);

int tiny_db_api_edit_table(int handle, char *title, td_elem_list_t *p_column, int param);

#endif