# Laboratori de SOA

ZeOS is a Linux like OS that runs on Bochs with the following features:
* Interruptions (clock & keyboard)
* Syscalls & Wrappers (write, fork, exit, etc)
* Process Management with a Round Robin Schedule
* Windows like Multi-Thread Management with a POSIX interface (create, join, exit)
* Semaphores

This repo contains two different implementations of this project, a initial one without Multi-Threads Managment that can be found inside the ZeOS folder, and a final implementation inside Projecte.

### Usage

* `make emuldbg` run OS with bochs internal debugger
* `make gdb` to run with GDB as external debugger

### Authors

* Albert Vilardell
* Josep Maria Olivé

#### Warnings

* Usage of `char* name = "";` non-fixed size char arrays on user code leads to potential memory issues, and unexpected results.
