
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "ting_db_platform.h"

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
