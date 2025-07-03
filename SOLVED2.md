---

# SYSeg – SOLVED Questions

## p1.c – Explain the results

### What happens

The program prints the address of the `main` function:

```c
#include <stdio.h>

int main() {
  printf("Main's address: %p\n", main);
  return 0;
}
```

Running `make p1` builds it with:

```make
gcc -m32 p1.c -o p1
```

### Explanation

This creates a 32-bit **non-PIE executable**. Without PIE (Position Independent Executable), the program is always loaded at a fixed address, so the printed address of `main` is the same across executions.

---

## p2.c – Fix design vulnerability

### What happens

The program is compiled using:

```make
gcc -Wall -m32 -O0 -fno-stack-protector -fno-pic -fno-pie -Wl,-no-pie p2.c -o p2
```

It disables both stack protection and position independence — intentionally making it vulnerable for educational purposes.

If we input a long string such as `youshallnotpass`, it overflows `user_key[10]` and may overwrite the nearby `verified` variable, granting unauthorized access.

### Fixing in p2fix.c

```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  int verified = 0;
  char user_key[10];

  printf("Enter password: ");
  fgets(user_key, sizeof(user_key), stdin);
  user_key[strcspn(user_key, "\n")] = '\0';

  if (!strcmp(user_key, "foo"))
    verified = 1;

  if (!verified) {
    printf("Access denied\n");
    exit(1);
  }

  printf("Access granted.\n");
  return 0;
}
```

### Makefile rule for p2fix

```make
all : p1 p2fix p3 p4-v1 p4-v2 p4-v3

p2fix : p2fix.c
	gcc -Wall -m32  -O0 -fno-stack-protector -fno-pic -fno-pie -Wl,-no-pie $(CFLAGS) $<  -o $@
```

---

## p3.c – Explain the function calls

### Build details

The binary is compiled using:

```make
gcc -Wall -m32 -O0 -fno-pic -fno-pie -Wl,-no-pie p3.c -o p3
```

This ensures:

* 32-bit mode
* No compiler optimizations
* No PIE or PIC
* Simple, predictable stack frame setup

### a) How `foo` calls `bar`

The `call` instruction is preceded by `push` to pass the argument. This follows the **cdecl** calling convention, where:

* Arguments go on the stack
* Caller cleans up afterward

Defined by the **System V x86 ABI**.

### b) How `bar` returns

`bar` uses `%eax` to return the result — standard for integer return values in the x86 ABI. This convention is portable and OS-independent.

### c) Function prologue and epilogue

For example, in `foo`:

```asm
55            push %ebp
89 e5         mov  %esp, %ebp
...
c9            leave
```

These set up and tear down the stack frame:

* Save old base pointer
* Create new frame for locals
* Restore caller frame

These are optional in very simple or optimized functions but are required for consistent stack access in standard C code.

### d) Purpose of `sub $X, %esp`

This reserves space on the stack for local variables:

```asm
83 ec 18      sub $0x18, %esp
```

This aligns the stack and provides space for locals. This is part of x86 ABI compliance (e.g., 16-byte alignment).

### e) Return type change

When `bar()` is changed to return `char`, the return still goes via `%eax`, but only the lowest byte is valid. The disassembly shows simpler instructions or truncation logic. Declaring function prototypes is crucial so the compiler handles arguments and return types correctly.

---

## p4.c – How libraries work

### Build options (from Makefile)

```make
p4-v1: p4.o p4a.c p4b.o
	gcc -m32 $^ -o $@

p4-v2: p4.o libp4.a
	gcc -m32 $< -L. -Wl,-Bstatic -lp4 -Wl,-Bdynamic -o $@

p4-v3: p4.o libp4.so
	gcc -m32 $< -L. -lp4 -o $@
```

### a) Running each version

```bash
./p4-v1
./p4-v2
./p4-v3
```

All print:

```
Foo
```

However, `p4-v3` may fail if `libp4.so` is not found. To ensure it works regardless of location, add `rpath`:

```make
-Wl,-rpath,'$ORIGIN'
```

Modified `p4-v3` rule:

```make
p4-v3: p4.o libp4.so
	gcc -m32 $< -L. -Wl,-rpath,'$$ORIGIN' -lp4 -o $@
```

### b) Size comparison

Use:

```bash
ls -lh p4-v*
```

* `p4-v1`: largest — includes all objects
* `p4-v2`: smaller — links static `.a`
* `p4-v3`: smallest — depends on dynamic `.so`

### c) Symbol table comparison

Use `nm`:

```bash
nm p4-v1 | grep foo
nm p4-v2 | grep foo
nm p4-v3 | grep foo
```

* `T`: defined symbol
* `U`: undefined (resolved at runtime)

Only dynamic version shows `U`.

### d) Dynamic section

Use `readelf -d`:

```bash
readelf -d p4-v3
```

Shows:

```
Shared library: [libp4.so]
```

This tells the dynamic linker to resolve symbols using `libp4.so`.

### e) Static vs dynamic libraries

| Scenario                   | Static        | Dynamic               |
| -------------------------- | ------------- | --------------------- |
| Deploying on other systems | Easier        | Requires .so files    |
| Updating library           | Needs rebuild | Auto-inherits changes |
| RAM usage across programs  | Duplicated    | Shared                |
| Binary size                | Larger        | Smaller               |

---

## dyn – Static vs Dynamic vs PIC

### a) Static library

* `.a` archive embedded in binary
* Fast, portable, no runtime dependencies
* Larger executables

### b) Dynamic with relocatable code

* Not PIC, but still shared at runtime
* May require runtime relocations
* Moderate overhead

### c) Dynamic with PIC

* Fully position-independent
* Efficient sharing across processes
* Preferred for `.so` on modern systems

### Summary

| Type            | Portability | Efficiency | Updateable | Sharing |
| --------------- | ----------- | ---------- | ---------- | ------- |
| Static          | High        | Low        | No         | No      |
| Dynamic (reloc) | Medium      | Medium     | Yes        | Yes     |
| Dynamic (PIC)   | High        | High       | Yes        | Yes     |

---
