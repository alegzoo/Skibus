# Define the compiler to use
CC=gcc

# Define compiler options: standard, warnings, error handling, and strict ISO compliance
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -pthread

# Define linker flags (if needed, this is currently empty)
LDFLAGS=

# Define the name of the final executable file
TARGET=proj2

# Define the source file from which to build
SOURCE=proj2.c

# 'all' is the default target
all: $(TARGET)

# Build the executable from the source file
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

# 'clean' target for removing compiled files to start build process afresh
clean:
	rm -f $(TARGET)
