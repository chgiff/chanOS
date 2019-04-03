arch ?= x86_64
kernel := build/kernel-$(arch).bin
iso := build/os-$(arch).iso

linker_script := src/arch/$(arch)/linker.ld
grub_cfg := src/arch/$(arch)/grub.cfg
assembly_source_files := $(wildcard src/arch/$(arch)/*.asm)
assembly_object_files := $(patsubst src/arch/$(arch)/%.asm, \
	build/arch/$(arch)/%.o, $(assembly_source_files))

kernel_img := .img/kernel.img

.PHONY: all clean run iso

all: $(kernel)

clean:
	@rm -r build

run: $(iso)
	@qemu-system-x86_64 -cdrom $(iso)

iso: $(iso)

$(iso): $(kernel) $(grub_cfg)
	@mkdir -p build/isofiles/boot/grub
	@cp $(kernel) build/isofiles/boot/kernel.bin
	@cp $(grub_cfg) build/isofiles/boot/grub
	@grub-mkrescue -o $(iso) build/isofiles 2> /dev/null
	@rm -r build/isofiles

$(kernel): $(assembly_object_files) $(linker_script)
	@ld -n -T $(linker_script) -o $(kernel) $(assembly_object_files)

# compile assembly files
build/arch/$(arch)/%.o: src/arch/$(arch)/%.asm
	@mkdir -p $(shell dirname $@)
	@nasm -felf64 $< -o $@

image:
	dd if=/dev/zero of=$(kernel_img) bs=512 count=32768
	sudo parted $(kernel_img) mklabel msdos
	sudo parted $(kernel_img) mkpart primary fat32 2048s 30720s
	sudo parted $(kernel_img) set 1 boot on
	#$(eval LOOP1 := $(shell sudo losetup -f) LOOP2 := $(shell sudo losetup -f))
	sudo losetup $(eval $(shell sudo losetup -f)) $(kernel_img)
	#$(eval LOOP2 := $(shell sudo losetup -f))
	sudo losetup $(eval $(shell sudo losetup -f)) $(kernel_img) -o 1048576
	sudo mkdosfs -F32 -f 2 $(LOOP2)
	sudo mount $(LOOP2) /mnt/fatgrub
	sudo grub-install --root-directory=/mnt/fatgrub --no-floppy --modules="normal part msdos ext2 multiboot" $(LOOP1)
	sudo cp -r .img/* /mnt/fatgrub
	sudo umount /mnt/fatgrub
	losetup -d $(LOOP1)
	losetup -d $(LOOP2)