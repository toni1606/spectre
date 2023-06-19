CC=gcc
CFLAGS=-Wall -pedantic -Werror
LDFLAGS=-lSDL2 -lSDL2_mixer -lm

SRCDIR=src
OBJDIR=obj
BINDIR=bin

SOURCES=$(wildcard src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(subst $(SRCDIR)/,,$(SOURCES)))
TARGET=main

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $(BINDIR)/$@ $(foreach o,$^,$(OBJDIR)/$(o)) $(LDFLAGS)

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/$< -o $(OBJDIR)/$@

run: all
	$(BINDIR)/$(TARGET)

clean:
	rm -rf $(BINDIR)/*
	rm -rf $(OBJDIR)/*
