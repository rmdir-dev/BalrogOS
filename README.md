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

# Run instruction 

```shell
make run
```

# Run Debug

First build & run qemu in debug
```shell
make debug run_debug
```

Then connect the remote debugger with the following configuration :
* remote target : localhost:1234
* symbol file : build/os/kernel.elf
* sysroot : -
* Debugger : gdb

# Todo list :

* RAMFS
  * should be ISO exportable to run on real hardware after this.
* USB driver
* USB persistance
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
