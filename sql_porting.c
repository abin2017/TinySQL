
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
#define ONE_BLOCK_SIZE 4096


typedef struct{
    char *buffer[MAX_BLOCK_COUNT];
    int buffer_pos[MAX_BLOCK_COUNT];
    int buffer_index;
}tiny_node_buffer_t;

static tiny_node_buffer_t opt = {{NULL}};

void *  sql_tiny_platform_mem_dbg (char *f, int line){
    #ifdef SUPPORT_DBG
    SQL_TINY_PLATFORM_LOG("%s, line %d, mem peices %d\n", f, line, count);
    #endif
}

void *  sql_tiny_platform_node_alloc (size_t  size ){
    try_next:
    if(opt.buffer_index >= MAX_BLOCK_COUNT || size > ONE_BLOCK_SIZE){
        return NULL;
    }

    if(opt.buffer[opt.buffer_index] == NULL){
        opt.buffer[opt.buffer_index] = sql_tiny_platform_alloc(ONE_BLOCK_SIZE);
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

void    sql_tiny_platform_node_free (){
    int i = 0;

    for(i = 0; i < MAX_BLOCK_COUNT; i++){
        if(opt.buffer[i]){
            sql_tiny_platform_free(opt.buffer[i]);
        }
    }
    memset(&opt, 0, sizeof(opt));
}

char *  sql_tiny_platform_node_strdup (char *str){
    if(str){
        int len = strlen(str);
        char * out = sql_tiny_platform_node_alloc(len + 1);

        if(out){
            strcpy(out, str);
            return out;
        }
    }

    return NULL;
}



void *  sql_tiny_platform_alloc (size_t  size ){
    void * ptr = malloc(size);

    #ifdef SUPPORT_DBG
    if(ptr){
        count ++;
    }
    #endif

    return ptr;
}

void *  sql_tiny_platform_realloc  (void * ptr, size_t  size ){
    void * ptrx = realloc  (ptr, size );

    #ifdef SUPPORT_DBG
    if(ptr == NULL){
        count ++;
    }
    #endif
    return ptrx;
}

void    sql_tiny_platform_free (void *__ptr){
    #ifdef SUPPORT_DBG
    if(__ptr){
        count --;
    }      
    #endif

    free(__ptr);
}

void    sql_tiny_platform_assert(int expression){
    assert(expression);
}

void    sql_tiny_platform_start(char *cmd){
    #ifndef SUPPORT_INTERACTIVE
    strncpy(sql_buffer, cmd, SQL_BUF_LENGTH - 1);
    sql_buffer_pos = 0;
    #endif
}

size_t sql_tiny_platform_fread (void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __stream){
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

size_t sql_tiny_platform_fwrite (const void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __s){
    #ifdef SUPPORT_INTERACTIVE
    return fwrite(__ptr, __size, __n, __s);
    #else
    SQL_TINY_PLATFORM_LOG((char *)__ptr);
    #endif
}

int sql_tiny_platform_ferror (FILE *__stream){
    #ifdef SUPPORT_INTERACTIVE
    return ferror(__stream);
    #else
    return 0;
    #endif
}


int sql_tiny_platform_fileno (FILE *__stream){
    #ifdef SUPPORT_INTERACTIVE
    return fileno(__stream);
    #else
    return 10;//(int)__stream;
    #endif
}

int sql_tiny_platform_isatty (int __fd){
    #ifdef SUPPORT_INTERACTIVE
    return isatty (__fd);
    #else
    return 0;
    #endif
}


#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include "tiny_db_priv.h"

static int malloc_count = 0;

char * sql_tiny_db_strdup_fix(char *str, unsigned int len){
    char *ptr = malloc(len + 1);
    if (ptr != NULL) {
        malloc_count++;
    }

    memcpy(ptr, str, len);
    ptr[len] = '\0';
    return ptr;
}

void * sql_tiny_db_malloc(unsigned int size){
    void *ptr = malloc(size);
    if (ptr != NULL) {
        malloc_count++;
    }
    return ptr;
}

void * sql_tiny_db_realloc(void *p, unsigned int size){
    return realloc(p, size);
}


void * sql_tiny_db_calloc(unsigned int nmemb, unsigned int size){
    void *ptr = calloc(nmemb, size);
    if (ptr!= NULL) {
        malloc_count++;
    }
    return ptr;
}

void sql_tiny_db_free(void * buf){
    free(buf);
    malloc_count--;
}

int sql_tiny_db_OsFileDelete(const char* path){
    unlink(path);
    return TR_SUCCESS;
}

int sql_tiny_db_OsFileExists(const char* path){
    if (access(path, F_OK) == 0) {
        return TR_SUCCESS;
    } else {
        return TR_FAIL;
    }
}

int sql_tiny_db_OsOpen(const char* path){
    FILE *fp = NULL;
    fp = fopen(path, "ab+");

    if(fp){
        fseek((FILE *)fp, 0, SEEK_SET);
    }

    return (int)fp;
}

int sql_tiny_db_OsClose(int OsFile){
    if(OsFile) {
        fclose((FILE *)OsFile);
    }
    return TR_SUCCESS;
}

int sql_tiny_db_OsRead(int OsFile, void* buffer, int amt){
    if(OsFile){
        return fread(buffer, 1, amt, (FILE *)OsFile);
    }else{
        return TR_FAIL;
    }
}

int sql_tiny_db_OsWrite(int OsFile, const void* buffer, int amt){
    if(OsFile){
        return fwrite(buffer, 1, amt, (FILE *)OsFile);
    }else{
        return TR_FAIL;
    }
}

int sql_tiny_db_OsSeek(int OsFile, int offset){
    if(OsFile){
        fseek((FILE *)OsFile, offset, SEEK_SET);
        return TR_SUCCESS;
    }else{
        return TR_FAIL;
    }
}

int sql_tiny_db_OsFileSize(int OsFile, int *pSize){
    //struct stat st;

    *pSize = 0;
    #if 0
    if (stat(path, &st) != 0) {
        return TR_FAIL;
    }
    #endif

    if(OsFile){
        fseek((FILE *)OsFile, 0, SEEK_END);
        *pSize = ftell((FILE *)OsFile);
        fseek((FILE *)OsFile, 0, SEEK_SET);
        return TR_SUCCESS;
    }
    return TR_FAIL;
}

int sql_tiny_db_OsLock(int lock_id){
    pthread_mutex_t *semaphore = (pthread_mutex_t *)lock_id;
    pthread_mutex_lock(semaphore);
    return TR_SUCCESS;   
}

int sql_tiny_db_OsUnlock(int lock_id){
    pthread_mutex_t *semaphore = (pthread_mutex_t *)lock_id;
    pthread_mutex_unlock(semaphore);
    return TR_SUCCESS;
}

int sql_tiny_db_OslockCreate(int *lock_id, int initial_value){
    pthread_mutex_t  *semaphore = sql_tiny_db_malloc(sizeof(pthread_mutex_t ));

    if (semaphore == NULL) {
        return TR_FAIL;
    }

    if (pthread_mutex_init(semaphore, NULL)!= 0) {
        free(semaphore);
        return TR_FAIL;
    }

    if(initial_value == 0) {
        pthread_mutex_lock(semaphore);         
    }

    *lock_id = (int)semaphore;
    return TR_SUCCESS;
}

int sql_tiny_db_OslockDelete(int lock_id){
    pthread_mutex_t *semaphore = (pthread_mutex_t *)lock_id;
    pthread_mutex_destroy(semaphore);
    sql_tiny_db_free(semaphore);
    return TR_SUCCESS;
}

int sql_tiny_db_insert_block(st_data_block_t *in){
    int ret = -1;

    if(*in->current_cnt >= *in->block_cnt){
        int resize = *in->current_cnt + in->enlarge_size;
        void * p_new = sql_tiny_db_realloc(*in->ppblock, resize * in->block_size);

        if(p_new){
            char *p_tmp = (char *)p_new;

            memset(&p_tmp[*in->current_cnt * in->block_size], 0, in->enlarge_size * in->block_size);
            *in->ppblock = p_new;
            *in->block_cnt += resize;
        }else{
            return ret;
        }
    }

    char *p_c = (char *)*in->ppblock;

    memcpy(&p_c[*in->current_cnt * in->block_size], in->block, in->block_size);
    *in->current_cnt += 1;
    ret = 0;

    return ret;
}

void    sql_tiny_db_assert(int expression){
    assert(expression);
}
