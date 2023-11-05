#ifndef __X_TINY_PORTING_H__
#define __X_TINY_PORTING_H__

#define TINY_DB_ERR printf
#define TINY_DB_DBG printf
#define TINY_DB_WARN printf
#define TINY_LOG_JUMP printf

typedef struct{
    void **ppblock;
    unsigned short *block_cnt;
    unsigned short *current_cnt;
    int block_size;

    int enlarge_size;
    void * block;
}st_data_block_t;

typedef struct{
    unsigned short buffer_length;
    unsigned short buffer_used;
    char *buffer;
}st_data_cpy_t;

char * tiny_db_strdup_fix(char *str, unsigned int len);

char * tiny_db_strdup(char *str);

void * tiny_db_malloc(unsigned int size);

void * tiny_db_realloc(void *p, unsigned int size);

void * tiny_db_calloc(unsigned int nmemb, unsigned int size);

void tiny_db_free(void * buf);

int tiny_db_insert_block(st_data_block_t *in);

int tiny_db_copy_block(st_data_cpy_t *in, char *src, int src_len);

void tiny_db_assert(int expression);

int tiny_db_OsFileDelete(const char* path);

int tiny_db_OsFileExists(const char* path);

int tiny_db_OsOpen(const char* path);

int tiny_db_OsClose(int OsFile);

int tiny_db_OsRead(int OsFile, void* buffer, int amt);

int tiny_db_OsWrite(int OsFile, const void* buffer, int amt);

int tiny_db_OsSeek(int OsFile, int offset);

int tiny_db_OsFileSize(int OsFile, int *pSize);

int tiny_db_OsLock(int lock_id);

int tiny_db_OsUnlock(int lock_id);

int tiny_db_OslockCreate(int *lock_id, int initial_value);

int tiny_db_OslockDelete(int lock_id);


#endif