
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


void *  sql_tiny_alloc (size_t  size ){
    return malloc(size);
}

void *  sql_tiny_realloc  (void * ptr, size_t  size ){
    return realloc  (ptr, size );
}

void    sql_tiny_free (void *__ptr){
    free(__ptr);
}
