# Generic Circular Buffer in C

This project provides two generic circular-buffer implementations behind one wrapper API:

- array-backed circular buffer
- linked-list-backed circular queue

The public entry point is `buffer_api.h`. The demo program in `test.c` uses `ncurses` to visualize the stored items through a callback-based iterator.

## Project Layout

- `buffer_api.c` / `buffer_api.h`: public wrapper API used by callers
- `buffer_array.c` / `buffer_array.h`: array implementation
- `buffer_list.c` / `buffer_list.h`: linked-list implementation
- `buffer_log.h`: logging macro that prefixes messages with `__func__`
- `test.c`: interactive demo/test program using `ncurses`
- `Makefile`: builds the libraries and demo executables

## Public API

The public API is intentionally centered on the wrapper type:

```c
typedef struct buffer_inf {
    void *buffer;
    int type;
} buffer_inf;
```

Supported operations:

- `create_cb`
- `delete_cb`
- `push_value`
- `pop_value`
- `is_empty`
- `is_full`
- `foreach_value`
- `get_size`
- `get_capacity`

Implementation type is selected with:

- `ARRAY`
- `LLIST`

## Iteration and Display

The library exposes a callback iterator:

```c
typedef void (*cb_iter_fn)(const void *item, int i);
```

This lets the library walk either implementation while user code decides how to display or process one item at a time.

Example:

```c
void print_student(const void *item, int i) {
    const student *s = item;
    printf("%d: %s %d\n", i, s->name, s->id);
}

foreach_value(buf, print_student);
```

## Logging

Errors are reported through:

```c
#define CB_LOG_ERROR(message) fprintf(stderr, "%s: %s\n", __func__, message)
```

That means log output includes the function name automatically.

## Library Types

There are two main library categories:

- `static library` (`.a`): linked into the executable at build time
- `shared/dynamic library` (`.so`, `.dylib`, `.dll`): loaded from a separate library file at runtime

For this project:

- `libcb.a` is the static library
- `libcb.so` is a Linux-style shared library target
- `libcb.dylib` is a macOS dynamic library target

`shared` and `dynamic` are conceptually the same kind of runtime-loaded library. The filename and linker mode differ by platform.

## Building With Make

Available targets:

- `make static`: build `libcb.a`
- `make shared`: build `libcb.so`
- `make dynamic`: build `libcb.dylib`
- `make test`: build `test.out` linked against the static library
- `make test-dynamic`: build `test_dynamic.out` linked against `libcb.dylib`
- `make all`: build the static library, both runtime-loaded libraries, and the static test executable
- `make clean`: remove generated files

Examples:

```sh
make static
make shared
make dynamic
make test
make test-dynamic
```

Notes:

- `make static` builds the static library only
- `make shared` builds the Linux-style shared library only
- `make dynamic` builds the macOS dynamic library only
- `make test` and `make test-dynamic` need `test.c`
- the library targets do not need `test.c`

## Building Manually

Compile the object files:

```sh
cc -Wall -Wextra -Werror -std=c11 -c buffer_api.c buffer_array.c buffer_list.c
```

Build the static library:

```sh
ar rcs libcb.a buffer_api.o buffer_array.o buffer_list.o
```

Build a Linux-style shared library:

```sh
cc -shared -o libcb.so buffer_api.o buffer_array.o buffer_list.o
```

Build a macOS dynamic library:

```sh
cc -dynamiclib -o libcb.dylib buffer_api.o buffer_array.o buffer_list.o
```

Build the demo program against the static library:

```sh
cc -Wall -Wextra -Werror -std=c11 test.c ./libcb.a -lcurses -o test.out
```

Build the demo program against the macOS dynamic library:

```sh
cc -Wall -Wextra -Werror -std=c11 test.c -L. -lcb -Wl,-rpath,@executable_path -lcurses -o test_dynamic.out
```

## Versioned Library Releases

You asked for tagged/versioned library builds without changing the `Makefile`, so this repo now includes:

- `release_version.sh`

This script uses the existing `make` targets, then creates versioned library artifacts in `dist/<version>/`.

Example:

```sh
./release_version.sh 1.0.0
```

That will:

- run `make clean`
- build `libcb.a`, `libcb.so`, and `libcb.dylib`
- create a release folder like `dist/1.0.0/`
- create versioned files and symlinks

Output layout:

```text
dist/1.0.0/
  libcb-1.0.0.a
  libcb.a -> libcb-1.0.0.a
  libcb.so.1.0.0
  libcb.so.1 -> libcb.so.1.0.0
  libcb.so -> libcb.so.1
  libcb.1.0.0.dylib
  libcb.1.dylib -> libcb.1.0.0.dylib
  libcb.dylib -> libcb.1.dylib
```

If you also want a matching Git tag:

```sh
./release_version.sh 1.0.0 --git-tag
```

That creates an annotated tag:

```text
v1.0.0
```

The Git tag is optional because it is a source-control release marker, while the versioned libraries are just build artifacts.

## Demo Controls

While `test.c` is running:

- `i`: push one item
- `r`: pop one item
- `d`: destroy the buffer

## Current Notes

- `test.c` is a demo/testing program, not part of the library itself
- the public API is in `buffer_api.h`
- internal implementation details stay in the array/list source files
