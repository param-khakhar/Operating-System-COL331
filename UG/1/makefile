run:
	gcc -c -g -Os -march=i686 -m32 -ffreestanding -fno-exceptions -fstrength-reduce -fomit-frame-pointer -finline-functions -Wall -Werror test.c -o test.o
	ld -static -Ttest.ld -nostdlib --nmagic -o test.elf test.o -melf_i386
	objcopy -O binary test.elf test.bin
	truncate test.bin -s 1200k
	mkisofs -o final.iso -b test.bin ./
	#qemu-system-x86_64 -drive format=raw,file=test.bin
	#qemu-system-i386 -drive format=raw,file=test.bin
	#dd if=/dev/zero of=dtt.img bs=512 count=2880
	#dd if=test.bin of=dtt.img
	#qemu-system-x86_64 -drive format=raw,file=dtt.img,index=0
other:
	as test.S -o test.o
	ld -Ttext 0x7c00 --oformat=binary test.o -o test.bin
	qemu-system-x86_64 test.bin
	dd if=/dev/zero of=dtt.img bs=512 count=2880
	dd if=test.bin of=dtt.img
	qemu-system-x86_64 -drive format=raw,file=dtt.img,index=0

