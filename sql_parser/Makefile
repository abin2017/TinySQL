CC=gcc
LEX=flex
BISON=bison
BISONFLAGS=-d #-Wno-write-strings
TARGET=sql_parser

$(TARGET): lex.yy.c parse.tab.c
	$(CC) parse.tab.c -o $(TARGET)

lex.yy.c: flex.l
	$(LEX) flex.l

parse.tab.c parse.tab.h: parse.y
	$(BISON) $(BISONFLAGS) parse.y

clean:
	rm -f lex.yy.c parse.tab.c parse.tab.h $(TARGET)

