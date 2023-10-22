#ifndef _SQL_GRAMMER_H__
#define _SQL_GRAMMER_H__

typedef struct Node {
    struct Node* child;
    struct Node* sibling;
    char str[150];
    char *value;
}stg_node_t;


stg_node_t * sql_tiny_grammer_api_parse(char *sql);

void sql_tiny_grammer_api_end();

void sql_tiny_grammer_api_destroy();


#endif