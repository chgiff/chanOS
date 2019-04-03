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

run: $(iso)
	qemu-system-x86_64 -s -cdrom $(iso)

debug: $(iso)
	x-terminal-emulator -e $(cross_compiler)/bin/x86_64-elf-gdb -x src/debug_script &
	qemu-system-x86_64 -S -s -cdrom $(iso)

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
	$(cross_compiler)/bin/x86_64-elf-gcc -g -std=c99 -c $< -o $@


image: $(fat_img)

$(fat_img): $(kernel) $(grub_cfg)
	mkdir -p build/imgfiles/boot/grub 
	cp $(kernel) build/imgfiles/boot/kernel.bin
	cp $(grub_cfg) build/imgfiles/boot/grub
	dd if=/dev/zero of=$(fat_img) bs=512 count=32768
	#create mbr, primary partition, and set bootable
	sudo parted $(fat_img) mklabel msdos
	sudo parted $(fat_img) mkpart primary fat32 2048s 30720s
	sudo parted $(fat_img) set 1 boot on
	#setup loopback devices
	sudo losetup $(loop0) $(fat_img)
	sudo losetup $(loop1) $(fat_img) -o 1048576
	#make fat32 fs on primary partition and mount it
	sudo mkdosfs -F32 -f 2 $(loop1)
	sudo mount $(loop1) /mnt/fatgrub
	#install grub
	sudo grub-install --root-directory=/mnt/fatgrub --no-floppy --modules="normal part msdos ext2 multiboot" $(loop0)
	#copy os files
	cp -r /build/imgfiles* /mnt/fatgrub
	#cleanup
	sudo umount /mnt/fatgrub
	sudo losetup -d $(loop0)
	sudo losetup -d $(loop1)