# Based on DOS/DJGPP tests makefile for Glide3
# Initial makefiles:
#  Copyright (c) 2002 - Borca Daniel
#  Email : dborca@users.sourceforge.net
#  Web   : http://www.geocities.com/dborca
#


.PHONY: all clean
.SUFFIXES: .c .o .exe

CC = i586-pc-msdosdjgpp-gcc
CFLAGS = -MMD -Wall -std=gnu99 -O2 -ffast-math -march=i386 -mtune=i586 -fomit-frame-pointer -fgnu89-inline
CFLAGS += -Iglide3/include
CFLAGS += -D__DOS__ -DSST1
CFLAGS += -D__DOS32__
CCTOOLS = /usr/bin/g++


LDFLAGS = -s -Lglide3/lib/

#LDLIBS = -lglide3i
LDLIBS = -lglide3x

build/%.o: src/%.c
	$(shell mkdir -p build) 
	$(CC) -o $@ $(CFLAGS) -c $<

dragon.exe: build/main.o build/geometry.o build/glide_helpers.o build/keyboard.o build/scene.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

all: dragon.exe

tools:
	$(CCTOOLS) -o image_tool resources/tools/image_tool.c
	$(CCTOOLS) -o mesh_tool resources/tools/mesh_tool.cpp

clean:
	@echo Cleaning...
	$(shell rm dragon.exe &> /dev/null) 
	$(shell rm -r build/ &> /dev/null) 
