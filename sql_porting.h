#ifndef __X_SQL_PORTING_H__
#define __X_SQL_PORTING_H__

//#define SUPPORT_INTERACTIVE

#define SQL_TINY_PLATFORM_ERR printf
#define SQL_TINY_PLATFORM_LOG printf
//#define SQL_TINY_PLATFORM_INFO printf

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

#endif