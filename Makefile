CC = clang
CFLAGS = -g -Og -Wall -Wextra -std=c11 -O0
INCLUDE = -I ./include
LDFLAGS = -lglfw3 -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lm
TARGET = ./lib/liborigami.a
SRC = $(wildcard ./src/*.c)
OBJS = $(patsubst ./src/%.c, ./lib/%.o, $(SRC))

all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(OBJS): $(SRC)
	mkdir -p ./lib/
	$(CC) $(CFLAGS) -fPIE -static $(INCLUDE) -c $< -o $@

clean:
	rm -rf $(TARGET) ./lib/*
