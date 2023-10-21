#ifndef __X_SQL_PORTING_H__
#define __X_SQL_PORTING_H__

void *  sql_tiny_alloc (size_t  size );

void *  sql_tiny_realloc  (void * ptr, size_t  size );

void    sql_tiny_free (void *__ptr);

#endif