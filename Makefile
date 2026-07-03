CC      = gcc
CFLAGS  = -Wall -g
LFLAGS  = -lfl

TARGET  = minicc
SRCS    = lex.yy.c parser.tab.c sym_tab.c scope.c gener.c ast.c interp.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

parser.tab.c parser.tab.h: parser.y
	bison -d -v parser.y

lex.yy.c: scanner.l parser.tab.h
	flex scanner.l

clean:
	rm -f lex.yy.c parser.tab.c parser.tab.h parser.output \
	      $(TARGET) *.o