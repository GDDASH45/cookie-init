# Cookie Init

Cookie Init is a lightweight userspace init system written in C for Cookie Linux.

It is designed to run as process ID 1 (PID 1), prepare the initial userspace environment, mount essential virtual filesystems, and execute a configured startup list.

## Features

- Written in C.
- Designed for Linux.
- Mounts essential virtual filesystems such as `/proc`, `/sys`, and `/dev`.
- Executes startup programs sequentially.
- Waits for each startup program to finish before launching the next.
- Restarts the autoexec sequence when it returns.
- Uses a simple text-based startup configuration.

## Project Structure

```text
cookie-init/
├── Makefile
├── README.md
├── LICENSE
├── init.c
├── autoexeclist.c
├── autoexeclist.h
├── reboot.c
└── reboot.h
```

## Autoexec Configuration

Cookie Init reads its startup list from:

```text
/etc/cookie/autoexec.list
```

Example:

```text
# Cookie Linux startup sequence
/usr/bin/mount-helper
/usr/bin/cookie-service-manager
/usr/bin/login
```

Each non-empty, non-comment line specifies an executable path. Programs are executed in order, and the next program starts after the previous one exits.

This format does not interpret shell syntax, pipelines, or shell variables.

## Building

Build the project using GNU Make and GCC:

```bash
make -j3
```

Install the resulting init executable into a root filesystem:

```bash
make install ROOTFS=/path/to/rootfs
```

Replace `/path/to/rootfs` with the actual root filesystem directory.

## Running

Cookie Init is designed to run as PID 1 in a Linux userspace environment. It is not intended to replace the init system on an ordinary running Linux installation.

Test it in a disposable virtual machine before using it as the init process of a bootable system.

## Current Status

Cookie Init is under development. Features such as comprehensive orphan-process reaping, service supervision, and orderly system shutdown still require further development.

## License

Cookie Init is licensed under the MIT License. See [LICENSE](LICENSE) for the full license text.
