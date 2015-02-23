CC=gcc
CFLAGS=-O0 -g -std=c99 -I./src
CFLAGS+=$(shell pkg-config --cflags gtk+-3.0)
CFLAGS+=$(shell sdl2-config --cflags)

SRC:=src
UI:=ui
CORE:=core

MAIN_SRCS:=main.c log.c
MAIN_SRCS_ALL:=$(MAIN_SRCS) log.h

MAIN_SRCS:=$(addprefix $(SRC)/,$(MAIN_SRCS))
MAIN_SRCS_ALL:=$(addprefix $(SRC)/,$(MAIN_SRCS_ALL))

CORE_SRCS:=core.c cpu/cpu.c mmu/mmu.c
CORE_SRCS_ALL:=$(CORE_SRCS) core.h cpu/cpu.h mmu/mmu.h

CORE_SRCS:=$(addprefix $(SRC)/$(CORE)/,$(CORE_SRCS))
CORE_SRCS_ALL:=$(addprefix $(SRC)/$(CORE)/,$(CORE_SRCS_ALL))

UI_SRCS:=ui.c ui_gtk.c gtk_opengl.c
UI_SRCS_ALL:=$(UI_SRCS) ui.h ui_gtk.h gtk_opengl.h

UI_SRCS:=$(addprefix $(SRC)/$(UI)/,$(UI_SRCS))
UI_SRCS_ALL:=$(addprefix $(SRC)/$(UI)/,$(UI_SRCS_ALL))

LIBS:=$(shell pkg-config --libs gtk+-3.0 gmodule-2.0) 
LIBS+=$(shell sdl2-config --libs)

.PHONY: all

all: qpra write_kpr

qpra: $(MAIN_SRCS_ALL) libcore.so libui.so
	$(CC) $(CFLAGS) $(MAIN_SRCS) -o $@ $(LIBS) -Wl,-rpath,./ -L./ -lcore -lui

libcore.so: $(CORE_SRCS_ALL)
	$(CC) $(CFLAGS) -fPIC $(CORE_SRCS) -shared -o $@

libui.so: $(UI_SRCS_ALL)
	$(CC) $(CFLAGS) -fPIC $(UI_SRCS) -shared -o $@

write_kpr: write_kpr.c
	$(CC) -O2 $< -o $@
