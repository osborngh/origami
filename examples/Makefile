CC = clang
CFLAGS = -g -Og -Wall -Wextra -std=c11 -O3
INCLUDE = -I ../include/
LDFLAGS = -lglfw3 -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lm
TARGET = ./build/app

FILE = ./red_window.c

all:
	rm -rf ./build/*
	mkdir -p build
	make -C ../
	$(CC) $(CFLAGS) $(INCLUDE) -o $(TARGET) $(FILE) -L../lib -lorigami $(LDFLAGS) && $(TARGET)

debug:
	gdb -q $(TARGET)


clean:
	rm -rf ./build/*
