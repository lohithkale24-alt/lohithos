# Makefile for LohithOS (Ubuntu WSL2 / Linux GCC toolchain)
CC = gcc
AS = as
LD = ld

CFLAGS  = -m32 -ffreestanding -O2 -Wall -Wextra -Iinclude -fno-pie -fno-stack-protector -nostdlib
ASFLAGS = --32
LDFLAGS = -m elf_i386 -T boot/linker.ld -nostdlib

OBJS = boot/boot.o \
       boot/interrupts.o \
       kernel/string.o \
       kernel/gdt.o \
       kernel/idt.o \
       kernel/pic.o \
       kernel/isr.o \
       kernel/timer.o \
       kernel/heap.o \
       kernel/net.o \
       kernel/fs.o \
       kernel/gui.o \
       kernel/kernel.o \
       drivers/font.o \
       drivers/graphics.o \
       drivers/keyboard.o \
       drivers/mouse.o \
       drivers/pci.o \
       drivers/e1000.o \
       apps/apps.o \
       apps/games.o

all: kernel.bin iso

boot/%.o: boot/%.S
	$(CC) $(CFLAGS) -c $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

drivers/%.o: drivers/%.c
	$(CC) $(CFLAGS) -c $< -o $@

apps/%.o: apps/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

iso: kernel.bin
	cp kernel.bin iso/boot/kernel.bin
	grub-mkrescue -o lohithos.iso iso

clean:
	rm -f $(OBJS) kernel.bin iso/boot/kernel.bin lohithos.iso
