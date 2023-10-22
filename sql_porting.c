
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "sql_porting.h"

#ifdef SUPPORT_INTERACTIVE
#include <unistd.h>
#include <errno.h>
#endif



#ifndef SUPPORT_INTERACTIVE
#define SQL_BUF_LENGTH 1024
char sql_buffer[SQL_BUF_LENGTH] = {0};
int sql_buffer_pos = 0;
#endif

#define SUPPORT_DBG

#ifdef SUPPORT_DBG
static int count  = 0;
#endif

#define MAX_BLOCK_COUNT 8
#define ONE_BLOCK_SIZE 3072


typedef struct{
    char *buffer[MAX_BLOCK_COUNT];
    int buffer_pos[MAX_BLOCK_COUNT];
    int buffer_index;
}tiny_node_buffer_t;

static tiny_node_buffer_t opt = {{NULL}};

void *  sql_tiny_mem_dbg (char *f, int line){
    #ifdef SUPPORT_DBG
    SQL_TINY_LOG("%s, line %d, mem peices %d\n", f, line, count);
    #endif
}

void *  sql_tiny_node_alloc (size_t  size ){
    try_next:
    if(opt.buffer_index >= MAX_BLOCK_COUNT || size > ONE_BLOCK_SIZE){
        return NULL;
    }

    if(opt.buffer[opt.buffer_index] == NULL){
        opt.buffer[opt.buffer_index] = sql_tiny_alloc(ONE_BLOCK_SIZE);
        if(opt.buffer[opt.buffer_index] == NULL){
            return NULL;
        }

        opt.buffer_pos[opt.buffer_index] = size; 

        return (void *) opt.buffer[opt.buffer_index];
    }else{
        if(size > ONE_BLOCK_SIZE - opt.buffer_pos[opt.buffer_index]){
            opt.buffer_index ++;
            goto try_next;
        }else{
            char *p = &opt.buffer[opt.buffer_index][opt.buffer_pos[opt.buffer_index]];

            opt.buffer_pos[opt.buffer_index] += size;
            return (void *) p;
        }
    }
}

void    sql_tiny_node_free (){
    int i = 0;

    for(i = 0; i < MAX_BLOCK_COUNT; i++){
        if(opt.buffer[i]){
            sql_tiny_free(opt.buffer[i]);
        }
    }
    memset(&opt, 0, sizeof(opt));
}



void *  sql_tiny_alloc (size_t  size ){
    void * ptr = malloc(size);

    #ifdef SUPPORT_DBG
    if(ptr){
        count ++;
    }
    #endif

    return ptr;
}

void *  sql_tiny_realloc  (void * ptr, size_t  size ){
    void * ptrx = realloc  (ptr, size );

    #ifdef SUPPORT_DBG
    if(ptr == NULL){
        count ++;
    }
    #endif
    return ptrx;
}

void    sql_tiny_free (void *__ptr){
    #ifdef SUPPORT_DBG
    if(__ptr){
        count --;
    }      
    #endif

    free(__ptr);
}

void    sql_tiny_assert(int expression){
    assert(expression);
}

void    sql_tiny_start(char *cmd){
    #ifndef SUPPORT_INTERACTIVE
    strncpy(sql_buffer, cmd, SQL_BUF_LENGTH - 1);
    sql_buffer_pos = 0;
    #endif
}

size_t sql_tiny_fread (void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __stream){
    #ifdef SUPPORT_INTERACTIVE
    return fread(__ptr, __size, __n, __stream);
    #else
    int len = __size * __n, sql_len = strlen(sql_buffer), left = 0;

    if(sql_len <= 0 || len <= 0){
        return 0;
    }

    left = sql_len - sql_buffer_pos;

    if(left <= 0){
        return 0;
    }

    if(left <= len){
        strncpy((char *)__ptr, &sql_buffer[sql_buffer_pos], left);
        sql_buffer_pos += left;

        return left/__size;
    }else{
        left = len;
        strncpy((char *)__ptr, &sql_buffer[sql_buffer_pos], left);
        sql_buffer_pos += left;

        return left/__size;
    }
    return 0;
    #endif
}

size_t sql_tiny_fwrite (const void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __s){
    #ifdef SUPPORT_INTERACTIVE
    return fwrite(__ptr, __size, __n, __s);
    #else
    SQL_TINY_LOG((char *)__ptr);
    #endif
}

int sql_tiny_ferror (FILE *__stream){
    #ifdef SUPPORT_INTERACTIVE
    return ferror(__stream);
    #else
    return 0;
    #endif
}


int sql_tiny_fileno (FILE *__stream){
    #ifdef SUPPORT_INTERACTIVE
    return fileno(__stream);
    #else
    return 10;//(int)__stream;
    #endif
}

int sql_tiny_isatty (int __fd){
    #ifdef SUPPORT_INTERACTIVE
    return isatty (__fd);
    #else
    return 0;
    #endif
}
