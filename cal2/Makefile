# Define the compiler and compiler flags
CC = gcc
CFLAGS = -Wall
LIBS = -lncurses -ljson-c

# Define the source and target files
SRC = calendar.c
TARGET = calendar

# Define the build rule
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^

# Define a clean rule
clean:
	rm -f $(TARGET)

# Define a run rule for running the program
run: $(TARGET)
	./$(TARGET)

# Default rule
all: $(TARGET)

.PHONY: clean run
