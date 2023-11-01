#ifndef __X_SQL_PORTING_H__
#define __X_SQL_PORTING_H__

//#define SUPPORT_INTERACTIVE

#define SQL_TINY_PLATFORM_ERR printf
#define SQL_TINY_PLATFORM_LOG printf
//#define SQL_TINY_PLATFORM_INFO printf

#define SQL_TINY_DB_ERR printf
#define SQL_TINY_DB_DBG printf

//#define SQL_TINY_PLATFORM_ERR(...) do{}while(0)
//#define SQL_TINY_PLATFORM_LOG(...) do{}while(0)
#define SQL_TINY_PLATFORM_INFO(...) do{}while(0)

#ifdef SUPPORT_INTERACTIVE
#define TINY_STDIN      stdin
#define TINY_STDOUT     stdout
#else
#define TINY_STDIN      (FILE *)1
#define TINY_STDOUT     (FILE *)2
#endif

#define SQL_MEM_DBG() sql_tiny_platform_mem_dbg((char *)__FUNCTION__, (int)__LINE__)

void *  sql_tiny_platform_node_alloc (size_t  size );

void    sql_tiny_platform_node_free ();

char *  sql_tiny_platform_node_strdup (char *str);

void *  sql_tiny_platform_alloc (size_t  size );

void *  sql_tiny_platform_realloc  (void * ptr, size_t  size );

void    sql_tiny_platform_free (void *__ptr);

void    sql_tiny_platform_assert(int expression);

void    sql_tiny_platform_start(char *cmd);

size_t  sql_tiny_platform_fwrite (const void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __s);

size_t  sql_tiny_platform_fread (void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __stream);

int     sql_tiny_platform_fileno (FILE *__stream);

int     sql_tiny_platform_ferror (FILE *__stream);

int     sql_tiny_platform_isatty (int __fd);

void *  sql_tiny_platform_mem_dbg (char *f, int line);



typedef struct{
    void **ppblock;
    unsigned short *block_cnt;
    unsigned short *current_cnt;
    int block_size;

    int enlarge_size;
    void * block;
}st_data_block_t;


void * sql_tiny_db_malloc(unsigned int size);

void * sql_tiny_db_realloc(void *p, unsigned int size);

void * sql_tiny_db_calloc(unsigned int nmemb, unsigned int size);

void sql_tiny_db_free(void * buf);

int sql_tiny_db_insert_block(st_data_block_t *in);

void sql_tiny_db_assert(int expression);

int sql_tiny_db_OsFileDelete(const char* path);

int sql_tiny_db_OsFileExists(const char* path);

int sql_tiny_db_OsOpen(const char* path);

int sql_tiny_db_OsClose(int OsFile);

int sql_tiny_db_OsRead(int OsFile, void* buffer, int amt);

int sql_tiny_db_OsWrite(int OsFile, const void* buffer, int amt);

int sql_tiny_db_OsSeek(int OsFile, int offset);

int sql_tiny_db_OsFileSize(int OsFile, int *pSize);

int sql_tiny_db_OsLock(int lock_id);

int sql_tiny_db_OsUnlock(int lock_id);

int sql_tiny_db_OslockCreate(int *lock_id, int initial_value);

int sql_tiny_db_OslockDelete(int lock_id);


#endif