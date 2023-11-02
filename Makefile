CC = gcc
AR = ar

CFLAGS := -g -O0 -m32 -Wno-format-security -DSQL_GRAMMER_TEST 
DEFS := 
LINKFLAGS :=

OBJDIR :=
OBJS := sql_porting.o \
		parse.tab.o \
		tiny_db_pager.o \
		tiny_db_module.o \
		tiny_db_node.o 

TARGET = parser

$(TARGET): 	$(OBJS) 
	$(CC) $(CFLAGS) $^ -o $@  $(LINKFLAGS)
	chmod +x $(TARGET)
	

check_folder:
	@if [ ! -d "$(OBJDIR)" ]; then \
		echo "Creating folder $(OBJDIR)..."; \
		mkdir -p "$(OBJDIR)"; \
	fi

clean:
	rm -rf *.o $(TARGET)

