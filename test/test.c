
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "tiny_db_api.h"
#include "tiny_db_platform.h"


int main() {
    int fd = 0;

    fd = tiny_db_api_open("./test.db");
    tiny_db_assert(0 != fd);

    tiny_db_api_close(fd);

    TINY_DB_MEMORY_USAGE();
    return 0;
}