CC := g++
CFLAGS := -g -Wall -std=c++11 -pthread 

SRCDIR := src
BUILDDIR := build
BINDIR := bin

SOURCES := $(SRCDIR)/jobCommander.cpp $(SRCDIR)/jobExecutorServer.cpp
OBJECTS := $(BUILDDIR)/jobCommander.o $(BUILDDIR)/jobExecutorServer.o

EXECUTABLES := $(BINDIR)/jobCommander $(BINDIR)/jobExecutorServer

$(shell mkdir -p $(BUILDDIR) $(BINDIR))

all: $(EXECUTABLES)

$(BINDIR)/jobCommander: $(BUILDDIR)/jobCommander.o | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BINDIR)/jobExecutorServer: $(BUILDDIR)/jobExecutorServer.o | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<



clean:
	rm -rf $(BINDIR)/* $(BUILDDIR)/*

.PHONY: all clean
