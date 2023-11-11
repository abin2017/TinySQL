#ifndef __TEST_H__
#define __TEST_H__


int     test_tiny_open_database();

void    test_tiny_close_database(int fd, int be_del) ;

void    tiny_db_test_create_table(int fd);

void    tiny_db_test_insert_test(int fd);

void    test_tiny_dump_table(int fd, char *table);

void    test_tiny_select_count(int fd);

void    test_tiny_select_data(int fd);

void    test_tiny_update_data(int fd, int id);

void    test_tiny_delete_data(int fd, int id);

#endif