CFLAGS  = -std=c99 -MMD -O2 -Wall -Werror -ggdb -fno-builtin \
          -fno-pic -fno-stack-protector -fno-omit-frame-pointer \
          -m32 -march=i386 -I./am -I./framework -I./include
LDFLAGS = -melf_i386 -Ttext 0x00100000 

SRCS = $(shell find src -name "*.c") framework/main.c
OBJS = $(addprefix build/, $(addsuffix .o, $(basename $(SRCS))))
DEPS = $(addprefix build/, $(addsuffix .d, $(basename $(SRCS))))

.PHONY: run clean

build/os.img: build/kernel
	cat am/mbr $< > $@

run: build/os.img
	qemu-system-i386 -serial stdio $<

clean:
	rm -rf build

build/kernel: $(OBJS)
	ld $(LDFLAGS) -o $@ $(OBJS) am/am-x86-qemu.a

build/%.o: %.c
	@mkdir -p $(dir $@)
	gcc $(CFLAGS) -c -o $@ $<
