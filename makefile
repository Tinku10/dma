# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -fsanitize=address -fsanitize=undefined

# Directories
SRCDIR := ./src
INCDIR := ./src
BUILDDIR := ./build
TARGETDIR := ./bin

# Target executable name
TARGET := $(TARGETDIR)/vsgc

# Source files and object files
SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

# Include directories
INCLUDES := -I$(INCDIR)

# Rules
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(TARGETDIR)
	$(CC) $^ -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@rm -rf $(BUILDDIR) $(TARGETDIR)

run:
	./$(TARGETDIR)/vsgc
