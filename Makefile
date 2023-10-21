CC = gcc
AR = ar

CFLAGS := -g -O0
DEFS := 
LINKFLAGS :=

OBJDIR :=
OBJS := sql_porting.o \
		parse.tab.o 

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

