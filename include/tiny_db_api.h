#ifndef __X_TINY_DB_API_H__
#define __X_TINY_DB_API_H__

/*
    0: 打开文件失败
    other： 成功
    */
int tiny_db_open(char *path);

void tiny_db_close(int handle);

#endif