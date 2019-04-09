arch ?= x86_64
kernel := build/kernel-$(arch).bin
iso := build/os-$(arch).iso

cross_compiler := ~/opt/cross
linker_script := src/asm/linker.ld
grub_cfg := src/asm/grub.cfg
assembly_source_files := $(wildcard src/asm/*.asm)
assembly_object_files := $(patsubst src/asm/%.asm, \
	build/asm/%.o, $(assembly_source_files))

c_source_files := $(wildcard src/*.c)
c_object_files := $(patsubst src/%.c, build/%.o, $(c_source_files))

fat_img := build/os-$(arch).img

loop0 := /dev/loop23
loop1 := /dev/loop24

.PHONY: all clean run iso

all: $(kernel)

clean:
	rm -r build

run: $(fat_img)
	qemu-system-x86_64 -s -drive format=raw,file=$(fat_img)

debug: $(fat_img)
	x-terminal-emulator -e $(cross_compiler)/bin/x86_64-elf-gdb -x src/debug_script
	#x-terminal-emulator -e gdb -x src/debug_script &
	qemu-system-x86_64 -s -drive format=raw,file=$(fat_img)

iso: $(iso)

$(iso): $(kernel) $(grub_cfg)
	mkdir -p build/isofiles/boot/grub
	cp $(kernel) build/isofiles/boot/kernel.bin
	cp $(grub_cfg) build/isofiles/boot/grub
	grub-mkrescue -o $(iso) build/isofiles 2> /dev/null
	rm -r build/isofiles

$(kernel): $(assembly_object_files) $(c_object_files) $(linker_script)
	ld -n -T $(linker_script) -o $(kernel) $(assembly_object_files) $(c_object_files)

source: $(c_object_files)
	echo $(c_object_files)

# compile assembly files
build/asm/%.o: src/asm/%.asm
	mkdir -p $(shell dirname $@)
	nasm -felf64 $< -o $@

# compile c files
build/%.o: src/%.c
	$(cross_compiler)/bin/x86_64-elf-gcc -g -Wall -Werror -ffreestanding -mno-red-zone -c $< -o $@


image: $(fat_img)

$(fat_img): $(kernel) $(grub_cfg)
	./src/create_fat.sh $(kernel) $(grub_cfg) $(fat_img)