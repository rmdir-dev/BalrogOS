# BalrogOS

BalrogOS is a basic Operating system written in C and ASM for educational purposes.

The goal of BalrogOS is to show how an operating system work and how to implement it.

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

# BalrogOS is back ! (I know nobody was waiting for it !)

Todo list :
* add signal
* add shared libraries loading.
* add network functionality
* add firewall 
* add multicore support
