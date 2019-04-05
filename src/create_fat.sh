#args are kernel path, grub config path, fat image path

#create folder structure
mkdir -p build/imgfiles/boot/grub 
cp $1 build/imgfiles/boot/kernel.bin
cp $2 build/imgfiles/boot/grub
dd if=/dev/zero of=$3 bs=512 count=32768

#create mbr, primary partition, and set bootable
sudo parted $3 mklabel msdos
sudo parted $3 mkpart primary fat32 2048s 30720s
sudo parted $3 set 1 boot on

#setup loopback devices
loop0=$(sudo losetup -f)
sudo losetup $loop0 $3
loop1=$(sudo losetup -f)
sudo losetup $loop1 $3 -o 1048576

#make fat32 fs on primary partition and mount it
sudo mkdosfs -F32 -f 2 $loop1
sudo mount $loop1 /mnt/fatgrub

#install grub
sudo grub-install -d /usr/lib/grub/i386-pc --root-directory=/mnt/fatgrub --no-floppy --modules="normal part_msdos ext2 multiboot" $loop0

#copy os files
sudo cp -r build/imgfiles/* /mnt/fatgrub

#cleanup
sudo umount /mnt/fatgrub
sudo losetup -d $loop0
sudo losetup -d $loop1