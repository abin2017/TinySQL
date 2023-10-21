#ifndef __X_SQL_PORTING_H__
#define __X_SQL_PORTING_H__

#define SQL_TINY_ERR printf
#define SQL_TINY_LOG printf

void *  sql_tiny_alloc (size_t  size );

void *  sql_tiny_realloc  (void * ptr, size_t  size );

void    sql_tiny_free (void *__ptr);

void    sql_tiny_assert(int expression);

#endif