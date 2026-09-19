# ft_strace

A small reimplementation of `strace` in C. It uses Linux `ptrace` to follow a
program and print its system calls to standard error.

The project currently handles x86 and x86-64 system-call registers.

## Build

```sh
make
```

This builds the `ft_strace` executable with Clang.

## Usage

```sh
./ft_strace <command> [args...]
```

For example:

```sh
./ft_strace /bin/echo "Hello, world!"
./ft_strace ls -la
```

## TODO

- Signal management: verify and fix signal handling, as it may not currently
  behave like the original `ft_strace`.
