CC = gcc
AR = ar

CFLAGS :=
DEFS := 

OBJDIR := obj
OBJS := $(OBJDIR)/sql_porting.o \
		$(OBJDIR)/parse.tab.o 

TARGET = sql_parser

$(TARGET): check_folder	$(OBJS)
	$(AR)  crs $(OBJDIR)/$(TARGET) $(OBJS)

$(OBJS):	$(OBJDIR)/%.o:%.c
	$(CC)  $(CFLAGS) ${DEFS} -c $<  -o $@

check_folder:
	@if [ ! -d "$(OBJDIR)" ]; then \
		echo "Creating folder $(OBJDIR)..."; \
		mkdir -p "$(OBJDIR)"; \
	fi

clean:
	rm -rf ./$(OBJDIR) $(TARGET)

