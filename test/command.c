#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>

#include "tiny_db_api.h"
#include "tiny_db_platform.h"
#include "test.h"

int main() {
    int opt;
    char input[100];
    int fd = 0;

    while (1) {
        printf("Enter a command: ");
        fgets(input, 100, stdin);
        input[strcspn(input, "\n")] = 0; // remove newline character
        char *args[10] = {"cmd"};
        int i = 1;
        char *token = strtok(input, " ");

        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;
        optind = 0;

        while ((opt = getopt(i, args, "htimcq:d:u:s:a:")) != -1) {
            switch (opt) {
                case 'i':
                    TINY_DB_INFO("init\n");
                    fd = test_tiny_open_database();
                    break;

                case 'q':
                    {
                        char * param = (char*)optarg;
                        printf("quiet\n");
                        if(optarg && strcmp(param, "del") == 0){
                            test_tiny_close_database(fd, 1);
                        }else{
                            test_tiny_close_database(fd, 0);        
                        }
                        goto _exit;
                    }
                    break;
                
                case 's':
                    {
                        char * param = (char*)optarg;
                        if(optarg){
                            test_tiny_delete_data(fd, atoi(param));
                        }
                    }
                    break;

                case 'a':
                    {
                        char * param = (char*)optarg;
                        if(optarg){
                            test_tiny_delete_table(fd, param);
                        }
                    }
                    break;
                
                case 'u':
                    {
                        char * param = (char*)optarg;
                        if(optarg){
                            test_tiny_update_data(fd, atoi(param));
                        }
                    }
                    break;
                
                case 't':
                    {
                        test_tiny_select_count(fd);        
                    }
                    break;
                
                case 'h':
                    {
                        test_tiny_select_data(fd);
                    }
                    break;

                case 'c':
                    {
                        tiny_db_test_create_table(fd);
                        tiny_db_test_insert_test(fd);
                    }
                    break;
                
                case 'm':
                    TINY_DB_MEMORY_USAGE();
                    break;
                
                case 'd':
                    {
                        char * param = (char*)optarg;

                        if(optarg){
                            test_tiny_dump_table(fd, param);
                        }else{
                            printf("input table name\n"); 
                        }
                    }
                    break;

                default:
                    if (optopt == 'q'){
                        printf("quiet\n");
                        test_tiny_close_database(fd, 0);        
                        goto _exit;       
                    }else{
                        printf("Usage: %s [-q exit] [-i init] [-m test] [-c create tables test1/2/3/4]\n\t[-d dump 'table'] [-t show select total]\n", args[0]);
                    }
                    break;
            }
        }
    }

    _exit:
        TINY_DB_MEMORY_USAGE();
        return 0;
}



