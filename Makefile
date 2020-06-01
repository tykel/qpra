CC=gcc
#CFLAGS=-Og -g -std=c11 -I./src -DLOG_LEVEL=3 -D_DEBUG_MEMORY -D_DEBUG
CFLAGS=-O3 -ffast-math -ftree-vectorize -std=c11 -I./src -DLOG_LEVEL=1 #-D_DEBUG_MEMORY -D_DEBUG
CFLAGS+=$(shell pkg-config --cflags gtk+-3.0)
CFLAGS+=$(shell sdl2-config --cflags)

SRC:=src
UI:=ui
CORE:=core

MAIN_SRCS:=main.c log.c
MAIN_SRCS_ALL:=$(MAIN_SRCS) log.h

MAIN_SRCS:=$(addprefix $(SRC)/,$(MAIN_SRCS))
MAIN_SRCS_OBJ:=$(MAIN_SRCS:.c=.o)
MAIN_SRCS_ALL:=$(addprefix $(SRC)/,$(MAIN_SRCS_ALL))

CORE_SRCS:=core.c cpu/cpu.c cpu/hrc.c mmu/mmu.c vpu/vpu.c cart/cart.c
CORE_SRCS_ALL:=$(CORE_SRCS) core.h cpu/cpu.h cpu/hrc.h mmu/mmu.h vpu/vpu.h cart/cart.h

CORE_SRCS:=$(addprefix $(SRC)/$(CORE)/,$(CORE_SRCS))
CORE_SRCS_OBJ:=$(CORE_SRCS:.c=.o)
CORE_SRCS_ALL:=$(addprefix $(SRC)/$(CORE)/,$(CORE_SRCS_ALL))

UI_SRCS:=ui.c ui_gtk.c gtk_opengl.c
UI_SRCS_ALL:=$(UI_SRCS) ui.h ui_gtk.h gtk_opengl.h

UI_SRCS:=$(addprefix $(SRC)/$(UI)/,$(UI_SRCS))
UI_SRCS_OBJ:=$(UI_SRCS:.c=.o)
UI_SRCS_ALL:=$(addprefix $(SRC)/$(UI)/,$(UI_SRCS_ALL))

LIBS:=-lGL $(shell pkg-config --libs gtk+-3.0 gmodule-2.0) 
LIBS+=$(shell sdl2-config --libs)

.PHONY: all clean

all: qpra #test.kpr

qpra: $(MAIN_SRCS_OBJ) $(CORE_SRCS_OBJ) $(UI_SRCS_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ $(LIBS)

test.kpr: asm/test.s
	./as.py $<

clean:
	rm -f qpra test.kpr
	find . -name "*.o" -type f -delete
