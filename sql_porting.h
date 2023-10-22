#ifndef __X_SQL_PORTING_H__
#define __X_SQL_PORTING_H__

//#define SUPPORT_INTERACTIVE

#define SQL_TINY_ERR printf
#define SQL_TINY_LOG printf

#ifdef SUPPORT_INTERACTIVE
#define TINY_STDIN      stdin
#define TINY_STDOUT     stdout
#else
#define TINY_STDIN      (FILE *)1
#define TINY_STDOUT     (FILE *)2
#endif

#define SQL_MEM_DBG() sql_tiny_mem_dbg((char *)__FUNCTION__, (int)__LINE__)

void *  sql_tiny_node_alloc (size_t  size );

void    sql_tiny_node_free ();

void *  sql_tiny_alloc (size_t  size );

void *  sql_tiny_realloc  (void * ptr, size_t  size );

void    sql_tiny_free (void *__ptr);

void    sql_tiny_assert(int expression);

void    sql_tiny_start(char *cmd);

size_t  sql_tiny_fwrite (const void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __s);

size_t  sql_tiny_fread (void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __stream);

int     sql_tiny_fileno (FILE *__stream);

int     sql_tiny_ferror (FILE *__stream);

int     sql_tiny_isatty (int __fd);

void *  sql_tiny_mem_dbg (char *f, int line);

#endif