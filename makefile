OPT = -g3 -Og
LIB_SOURCES1 = main.c

LIB_SOURCES = $(addprefix src/, $(LIB_SOURCES1))
CC = gcc
TARGET = run.exe
LIB_OBJECTS =$(LIB_SOURCES:.c=.o)
LEVEL_CS = $(addprefix src/, $(LEVEL_SOURCES:.data=.c))
LDFLAGS= -L. $(OPT) -Wextra 
LIBS= -lGL -lGLEW -lglfw -lm -liron -licydb
ALL= $(TARGET)
CFLAGS = -Isrc/ -Iinclude/ -std=gnu11 -c $(OPT) -Wall -Wextra -Werror=implicit-function-declaration -Wformat=0 -D_GNU_SOURCE -fdiagnostics-color -Wextra  -Wwrite-strings -Werror -msse4.2 -Werror=maybe-uninitialized 

$(TARGET): $(LIB_OBJECTS)
	$(CC) $(LDFLAGS) $(LIB_OBJECTS) $(LIBS) -o $@

all: $(ALL)

.c.o: $(HEADERS) $(LEVEL_CS)
	$(CC) $(CFLAGS) $< -o $@ -MMD -MF $@.depends

depend: h-depend
clean:
	rm -f $(LIB_OBJECTS) $(ALL) src/*.o.depends src/*.o src/level*.c src/*.shader.c 
.PHONY: test
test: $(TARGET)
	make -f makefile.compiler
	make -f makefile.test test

-include $(LIB_OBJECTS:.o=.o.depends)


