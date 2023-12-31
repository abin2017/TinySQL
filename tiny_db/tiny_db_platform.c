
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "tiny_db_platform.h"

#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include "tiny_db_priv.h"

static int malloc_count = 0;

void tiny_db_memory_usage(char *_func, int _line){
    tiny_db_printf("\e[1;36m[%s] line %d malloc count %d\033[0m\n", _func, _line, malloc_count);
}

char * tiny_db_strdup_fix(char *str, unsigned int len){
    char *ptr = malloc(len + 1);
    if (ptr != NULL) {
        malloc_count++;
    }

    memcpy(ptr, str, len);
    ptr[len] = '\0';
    return ptr;
}

char * tiny_db_strdup(char *str){
    char *ptr = malloc(strlen(str) + 1);
    if (ptr != NULL) {
        malloc_count++;
    }

    strcpy(ptr, str);
    return ptr;
}


void * tiny_db_malloc(unsigned int size){
    void *ptr = malloc(size);
    if (ptr != NULL) {
        malloc_count++;
    }
    return ptr;
}

void * tiny_db_realloc(void *p, unsigned int size){
    void *p_ptr = NULL;
    p_ptr =  realloc(p, size);

    if(p_ptr && p == NULL){
        malloc_count++;
    }
    return p_ptr;
}


void * tiny_db_calloc(unsigned int nmemb, unsigned int size){
    void *ptr = calloc(nmemb, size);
    if (ptr!= NULL) {
        malloc_count++;
    }
    return ptr;
}

void tiny_db_free(void * buf){
    free(buf);
    malloc_count--;
}

int tiny_db_OsFileDelete(const char* path){
    unlink(path);
    return TR_SUCCESS;
}

int tiny_db_OsFileExists(const char* path){
    if (access(path, F_OK) == 0) {
        return TR_SUCCESS;
    } else {
        return TR_FAIL;
    }
}

int tiny_db_OsOpen(const char* path){
    FILE *fp = NULL;
    fp = fopen(path, "rb+");

    if(fp == NULL){
        fp = fopen(path, "wb+");
    }

    if(fp){
        fseek((FILE *)fp, 0, SEEK_SET);
    }

    return (int)fp;
}

int tiny_db_OsClose(int OsFile){
    if(OsFile) {
        fclose((FILE *)OsFile);
    }
    return TR_SUCCESS;
}

int tiny_db_OsRead(int OsFile, void* buffer, int amt){
    if(OsFile){
        return fread(buffer, 1, amt, (FILE *)OsFile);
    }else{
        return TR_FAIL;
    }
}

int tiny_db_OsWrite(int OsFile, const void* buffer, int amt){
    if(OsFile){
        int c = fwrite(buffer, 1, amt, (FILE *)OsFile);
        fflush((FILE *)OsFile);
        return c;
    }else{
        return TR_FAIL;
    }
}

int tiny_db_OsSeek(int OsFile, int offset){
    if(OsFile){
        fseek((FILE *)OsFile, offset, SEEK_SET);
        return TR_SUCCESS;
    }else{
        return TR_FAIL;
    }
}

int tiny_db_OsFileSize(int OsFile, int *pSize){
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

int tiny_db_OsLock(int lock_id){
    pthread_mutex_t *semaphore = (pthread_mutex_t *)lock_id;
    pthread_mutex_lock(semaphore);
    return TR_SUCCESS;   
}

int tiny_db_OsUnlock(int lock_id){
    pthread_mutex_t *semaphore = (pthread_mutex_t *)lock_id;
    pthread_mutex_unlock(semaphore);
    return TR_SUCCESS;
}

int tiny_db_OslockCreate(int *lock_id, int initial_value){
    pthread_mutex_t  *semaphore = tiny_db_malloc(sizeof(pthread_mutex_t ));

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

int tiny_db_OslockDelete(int lock_id){
    pthread_mutex_t *semaphore = (pthread_mutex_t *)lock_id;
    pthread_mutex_destroy(semaphore);
    tiny_db_free(semaphore);
    return TR_SUCCESS;
}

int tiny_db_insert_block(st_data_block_t *in){
    int ret = -1;

    if(*in->current_cnt >= *in->block_cnt){
        int resize = *in->block_cnt + in->enlarge_size;
        void * p_new = tiny_db_realloc(*in->ppblock, resize * in->block_size);

        if(p_new){
            char *p_tmp = (char *)p_new;

            memset(&p_tmp[*in->current_cnt * in->block_size], 0, in->enlarge_size * in->block_size);
            *in->ppblock = p_new;
            *in->block_cnt = resize;
        }else{
            return ret;
        }
    }

    char *p_c = (char *)*in->ppblock;

    memcpy(&p_c[*in->current_cnt * in->block_size], in->block, in->block_size);
    *in->current_cnt += 1;

    return 0;
}

void    tiny_db_assert(int expression){
    assert(expression);
}

int tiny_db_copy_block(st_data_cpy_t *in, char *src, int src_len){
    int left = in->buffer_length - in->buffer_used;
    int cpy_len = left > src_len ? src_len : left;

    if(left == 0 || src_len == 0){
        TINY_DB_WARN("src len %d, left len %d\n", src_len, left);
        return 0;
    }

    memcpy(&in->buffer[in->buffer_used], src, cpy_len);
    in->buffer_used += cpy_len;

    return cpy_len;
}

void tiny_db_printf(char *format, ...){
    va_list args;

    #if 0
    if(file1 == NULL){
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char buf[100], out[128];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
        snprintf(out, sizeof(out), "test/out_log/%s_out.txt", buf);
        file1 = fopen(out, "a");
    }
    #endif
    va_start(args, format);
    int written = vfprintf(stdout, format, args);
    //vfprintf(file1, format, args);
    //fflush(file1);
    va_end(args);

    if (written < 0) {
        fprintf(stderr, "An error occurred in custom_printf\n");
        return;
    }
    fflush(stdout);
}
