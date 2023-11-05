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


typedef enum{
    TD_LOGIC_OR     = 0,
    TD_LOGIC_AND    = 1,
    TD_LOGIC_MAX
}td_logical_t;

typedef enum{
    TD_ARITHMETIC_EQUAL             = 0, //== number & string
    TD_ARITHMETIC_UNEQUAL           = 1, //!= number & string

    TD_ARITHMETIC_GREATER           = 2, //>  number
    TD_ARITHMETIC_GREATER_QUUAL     = 3, //>= number
    TD_ARITHMETIC_LESS              = 4, //<  number
    TD_ARITHMETIC_LESS_QUUAL        = 5, //<= number

    TD_ARITHMETIC_MAX
}td_arithmetic_t;

struct td_condition;

typedef struct td_cond_elem{
    td_etype_t                  type;
    char                    *   p_tag;
    int                         tag_idx;
    int                     *   content;
    td_arithmetic_t             arithmetic;
    struct td_condition     *   p_next;
}td_cond_elem_t;

typedef struct td_condition{
    td_logical_t    logic;
    int             count;
    td_cond_elem_t  *p_elements;
}td_condition_t;

/*
    0: 打开文件失败
    other： 成功
    */
int tiny_db_api_open(char *path);

void tiny_db_api_close(int handle);

int tiny_db_api_create_table(int handle, char *title, td_elem_list_t *p_column, int param);

int tiny_db_api_delete_table(int handle, char *title, int param);

int tiny_db_api_edit_table(int handle, char *title, td_elem_list_t *p_column, int param);

int tiny_db_api_insert_data(int handle, char *title, td_elem_list_t *p_elements, int param);

int tiny_db_api_update_data(int handle, char *title, td_elem_list_t *p_elements, td_condition_t *p_cond, int param);

#endif