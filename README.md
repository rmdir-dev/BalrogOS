# BalrogOS

BalrogOS is a basic Operating system written in C and ASM for educational purposes.

The goal of BalrogOS is to show how an operating system work under the hood.

# Starting with 

1) Install the toolbox to install toolbox dependencies
```shell
make install_toolbox
```

# Build instruction

To build the kernel : 
```shell
make bootloader kernel os
```
* booloader : build the bootloader
* kernel : build the kernel
* os : create the ISO file from the binaries.

To build the toolkit (shell, ls, ...)
```shell
make tools
```

For UEFI :

```shell
make kernel esp                                                                                                                                                          
make run_uefi
```

# Run instruction 

```shell
make run
```

# Run Debug

First build & run qemu in debug
```shell
make debug run_debug
```

# Release for a bios machine

```shell
make release
```

Builds `build/release/balrog-bios.img`, and `balrog-bios.vdi` next to it when
VBoxManage is installed.

```shell
lsblk
sudo dd if=build/release/balrog-bios.img of=$(DEV) bs=4M conv=fsync status=progress
sudo sync
```

For VirtualBox, attach the `.vdi` to a SATA port with EFI **off**. There is no
usb mass storage controller in vbox, so a usb key is tested this way : the
firmware sees a disk with a partition table either way.

# Release for an uefi machine

```shell
make release_uefi
```

Builds `build/release/balrog-uefi.img`, a GPT disk holding one FAT32 ESP with
`/EFI/BOOT/BOOTX64.EFI`, and `balrog-uefi.vdi` next to it.

```shell
lsblk
sudo dd if=build/release/balrog-uefi.img of=$(DEV) bs=4M conv=fsync status=progress
sudo sync
```

For VirtualBox, attach the `.vdi` to a SATA port with EFI **on**.

Then connect the remote debugger with the following configuration :
* remote target : localhost:1234
* symbol file : build/os/kernel.elf
* sysroot : -
* Debugger : gdb

# Todo list :

* RAMFS : done
* shutdown : done
  * should be ISO exportable to run on real hardware after this. (done BIOS TODO : UEFI -currently not working on test laptop 2-)
* USB driver : done
* USB persistance : wip need to finish pstore fully to debug on real hardware
* signal
* improve shell (besh add pipe/redirect/signal)
* proper shutdown
* procfs (/proc)
* mmap
* cow fork
* shared library
* proper zombie process killer -> named Morgoth
* network 
* firewall -> named Angband
* multicore processing support
* improve scheduler (ATM round robin)
* graphical interface (maybe)
