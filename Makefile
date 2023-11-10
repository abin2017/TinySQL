-include .config

OBJDIR := $(OUTPUT_TMP)
TARGET :=	$(LIBNMAE)

		
OBJS  := $(OBJDIR)/tiny_db_platform.o \
			$(OBJDIR)/tiny_db_module.o \
			$(OBJDIR)/tiny_db_node.o \
			$(OBJDIR)/tiny_db_pager.o \
			$(OBJDIR)/tiny_db_table.o \
			$(OBJDIR)/tiny_db_api.o \
				 
OBJS_GRAMMER := $(OBJDIR)/parse.tab.o \
				 $(OBJDIR)/sql_porting.o \

OBJS_TEST := $(OBJDIR)/test.o	$(OBJDIR)/command.o

OBJS_COMPILE := $(OBJS) 

ifeq ($(ENABLE_SQL_GRAMMER), Y)
OBJS_COMPILE += $(OBJS_GRAMMER)
endif


TEST_OBJS := $(OBJS_TEST)
TEST_ELF := test.elf


all:	env $(TARGET)
	@echo "build ok..."
	rm -f $(TARGET)
	cp -f  $(OBJDIR)/$(TARGET) $(TARGET)

env:
	mkdir -p ./$(OBJDIR)

clean:
	@echo "Distcleaning ..."
	rm -f $(OBJDIR)/$(TARGET) $(OBJS_COMPILE) lib*.a *.lib

ifeq ($(ENABLE_TEST), Y)
test: env $(TEST_ELF)
	@echo "finished compile ..."

test_clean:
	@echo "test clean ..."
	rm -f $(TEST_ELF) $(TEST_OBJS)

test_rebuild: clean test_clean all $(TEST_ELF)
	@echo "test rebuild finished ..."
endif

$(TARGET):	$(OBJS_COMPILE)
	$(AR)  crs $(OBJDIR)/$(TARGET) $(OBJS_COMPILE)

$(OBJS):	$(OBJDIR)/%.o:tiny_db/%.c
	$(CC)  $(CFLAGS) ${DEFS} -c $<  -o $@

ifeq ($(ENABLE_SQL_GRAMMER), Y)
$(OBJS_GRAMMER):	$(OBJDIR)/%.o:grammer/%.c
	$(CC)  $(CFLAGS) ${DEFS} -c $<  -o $@
endif

ifeq ($(ENABLE_TEST), Y)
$(TEST_ELF): $(OBJS) $(TEST_OBJS)
	$(CC) -o $(TEST_ELF) $(TEST_OBJS) ${CFLAGS} ${LINK_FLAGS}

$(OBJS_TEST): $(OBJDIR)/%.o:test/%.c
	$(CC)  $(TEST_CFLAGS) ${DEFS} -c $<  -o $@
endif