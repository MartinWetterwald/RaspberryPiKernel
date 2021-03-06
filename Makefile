THIS := $(lastword $(MAKEFILE_LIST))
MAKEFLAGS += --output-sync=target

#--------DIRECTORIES--------#
BUILDDIR = build/
DEPDIR = $(BUILDDIR)dep/
PREDIR = $(BUILDDIR)pre/
ASMDIR = $(BUILDDIR)asm/
OBJDIR = $(BUILDDIR)obj/
BINDIR = $(BUILDDIR)bin/
MISCDIR = $(BUILDDIR)misc/
SRCDIR = src/
MAKEINCDIR = make/
GDBDIR = gdb/
GDBDEFAULT = $(GDBDIR)default.gdb
UIMAGE = $(BINDIR)u$(TARGETNAME).img

CLEANDIR = $(DEPDIR) $(PREDIR) $(ASMDIR) $(OBJDIR) $(MISCDIR)

#--------AUTOMATIC RECURSIVE FILE FINDING--------#
SOURCES_C = $(shell find $(SRCDIR) -name "*.c")
SOURCES_ASM = $(shell find $(SRCDIR) -name "*.s")
THIS += $(wildcard $(MAKEINCDIR)*.mk)

#--------INTERMIDIATE FILES--------#
PRE = $(addprefix $(PREDIR), $(notdir $(SOURCES_C:%.c=%.i)))
ASM = $(addprefix $(ASMDIR), $(notdir $(SOURCES_C:%.c=%.s)))
OBJC = $(addprefix $(OBJDIR), $(notdir $(ASM:%.s=%.c.o)))
OBJASM += $(addprefix $(OBJDIR), $(notdir $(SOURCES_ASM:%.s=%.s.o)))
OBJ = $(OBJC) $(OBJASM)
DEP = $(addprefix $(DEPDIR), $(notdir $(SOURCES_C:%.c=%.d)))

#--------FINAL FILES--------#
TARGETNAME = kernel
KERNELIMG = $(BINDIR)$(TARGETNAME).img
KERNELELF = $(BINDIR)$(TARGETNAME).elf
KERNELLIST = $(MISCDIR)$(TARGETNAME).list
LINKERSCRIPT = $(MAKEINCDIR)$(TARGETNAME).ld
MAPFILE = $(MISCDIR)$(TARGETNAME).map
UBOOTSCRIPT = $(MISCDIR)$(UBOOTSRC).uimg

-include $(MAKEINCDIR)config.inc.mk

#--------COMMANDS--------#
HIDE ?= @
PREFIX ?= arm-none-eabi-
CMD_PREFIX ?= $(HIDE)$(PREFIX)
GDB ?= $(PREFIX)gdb
QEMU ?= qemu-system-arm
MKDIR = $(HIDE)mkdir -p
ECHO = $(HIDE)echo
RM = $(HIDE)rm -rf
PRINTF = $(HIDE)printf

#--------COMP, ASM & EXE OPTIONS--------#
CC_FLAGS_KERNEL = -nostdlib -fomit-frame-pointer -mno-apcs-frame -nostartfiles -ffreestanding
CC_FLAGS = $(CC_FLAGS_KERNEL) -mcpu=arm1176jzf-s -std=c99 -Wall -Wextra -Werror -g -O0
ASM_FLAGS = -mcpu=arm1176jzf-s -g
QEMU_FLAGS = -kernel $(KERNELELF) -cpu arm1176 -m 512 -M raspi -nographic -monitor none -no-reboot -S -s -serial stdio

include $(MAKEINCDIR)colors.inc.mk
include $(MAKEINCDIR)errorHandler.inc.mk

#--------SPECIAL RULES--------#
.PHONY: all clean mrproper emu run list deploy sdcopy umount ubootscript default
.PRECIOUS: $(PRE) $(ASM) $(OBJ) $(DEP)
.SECONDEXPANSION:

#--------RULES--------#
include $(MAKEINCDIR)rules.inc.mk

#--------PHONY RULES--------#
include $(MAKEINCDIR)rules.phony.inc.mk

#--------AUTOMATIC DEPENDENCIES--------#
-include $(DEP)
