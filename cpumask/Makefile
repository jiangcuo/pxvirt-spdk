CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = cpumask_tool
SOURCE = cpumask_tool.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/bin/

.PHONY: all clean install