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

When executed multiple times, the address of `main` varies with each run, for example:

```
0x5609c4481149
0x55e5c899d149
0x5581fc6d7149
0x561eb1dad149
```

### Explanation

The changing addresses occur because the executable is compiled as a **PIE (Position Independent Executable)** and the system has **ASLR (Address Space Layout Randomization)** enabled.

* **PIE** allows the program’s code to be loaded at different memory locations each time it runs.
* **ASLR** randomizes memory layout for security, making addresses unpredictable.

On many modern systems, even when compiling for 32-bit (`-m32`), compilers default to producing PIE binaries unless explicitly disabled.

### Fixed address scenario

If PIE is disabled (e.g., by compiling with `-no-pie`), the executable is loaded at a fixed memory address, resulting in the same `main` address being printed on each execution.

---

## p2.c – Fix design vulnerability

### What happens

The program is compiled using:

```make
gcc -Wall -m32 -O0 -fno-stack-protector -fno-pic -fno-pie -Wl,-no-pie p2.c -o p2
```

It disables both stack protection and position independence — intentionally making it vulnerable for educational purposes.

If we input a long string such as `youshallnotpass`, it overflows `user_key[10]` and may overwrite the nearby `verified` variable, granting unauthorized access.

However, even though the Makefile tries to disable stack protection with `-fno-stack-protector`, some compilers or distributions enable it by default or override your flags. Not only that, some systems use hardened runtime checks, where buffer overflows are caught and prevented; as well as some architectures or compiler versions may place variables in different orders (or apply optimizations), reducing overflow impact.

### Fixing in p2fix.c

```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  int verified = 0;
  char user_key[10];

  printf("Enter password: ");
  fgets(user_key, sizeof(user_key), stdin);  // replaced scanf with fgets for safe bounded input
  user_key[strcspn(user_key, "\n")] = '\0';  // removes the newline character left by fgets

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

Here's the updated and corrected version of the **p3.c solution**, maintaining the original Markdown structure and incorporating important clarifications about 64-bit vs 32-bit behavior based on the `Makefile` provided:

---

## p3.c – Explain the function calls

### Build details

The binary is compiled using:

```make
gcc -Wall -m32 -O0 -fno-pic -fno-pie -Wl,-no-pie p3.c -o p3
```

This ensures:

* Compilation in **32-bit mode** using the `-m32` flag
* No optimizations (`-O0`), which preserves full function prologues and variable usage
* No position-independent code (`-fno-pic`, `-fno-pie`)
* A classic, non-PIE ELF binary layout

> Note: On some systems, the compiler may silently fall back to 64-bit mode if the necessary multilib (32-bit) toolchain is not installed. If this happens, the disassembly will reflect 64-bit conventions instead of 32-bit. All the following explanations assume that the compilation was indeed in 32-bit mode as intended.

---

### a) How `foo` calls `bar`

In 32-bit mode, the `cdecl` calling convention is used:

* The caller (`foo`) **pushes the argument** onto the stack using the `push` instruction.
* Then, `call bar` transfers control to the callee.
* The **caller is responsible for cleaning up the stack** after the call.

This behavior matches the **System V ABI** for the x86 (IA-32) architecture, which defines stack-based parameter passing from right to left.

> In contrast, 64-bit System V ABI (used when `-m32` is not applied) passes the first arguments via registers like `%rdi`, `%rsi`, etc.

---

### b) How `bar` returns

The return value of `bar` is stored in the `%eax` register (in 32-bit mode). This register is used for returning integers and pointers.

* After `bar` returns, its result is already in `%eax`.
* `foo` uses this value directly or stores it in a local variable.

> In 64-bit mode, the same logic applies, but the return value would be stored in `%rax`. The principle of using a dedicated register for return values remains the same across architectures.

---

### c) Function prologue and epilogue

A typical function prologue in 32-bit looks like:

```asm
push %ebp
mov  %esp, %ebp
sub  $X, %esp  ; optional, for local variables
```

And the epilogue is:

```asm
leave
ret
```

Explanation:

* `push %ebp` saves the previous frame pointer.
* `mov %esp, %ebp` establishes a new base pointer for the current frame.
* `leave` is equivalent to `mov %ebp, %esp; pop %ebp` — it restores the previous stack frame.

This structure is necessary when the function uses local variables or accesses arguments via base pointer offsets (e.g., `8(%ebp)`).

> Note: With optimization (`-O1` or higher), these can be omitted if no locals are needed. In 64-bit, modern compilers often omit the frame pointer entirely and use stack-relative offsets or debug info instead.

---

### d) Purpose of `sub $X, %esp`

This instruction reserves space on the stack for local variables:

```asm
sub $0x18, %esp
```

Meaning:

* 24 bytes of space are reserved for locals or alignment padding.
* Stack alignment is critical, particularly when calling library functions or using SIMD instructions.

In the 32-bit System V ABI, stack alignment is typically to 4 bytes, but compilers may align to higher values (e.g., 16 bytes) to be safe or compatible with other ABIs.

> In 64-bit ABI, alignment requirements are stricter: the stack must be 16-byte aligned before a function call.

---

### e) Return type change

If you change `bar`'s return type from `int` to `char`:

```c
char bar(int m) {
  char b = m + 1;
  return b;
}
```

Behavior:

* The compiler still uses `%eax` to store the return value.
* Only the **lower 8 bits** (`%al`) are meaningful.
* If the caller (`foo`) expects an `int`, and `bar` returns `char`, **sign-extension** or zero-extension may occur.

**Why prototypes matter**:

* If `bar` is implicitly declared or misdeclared, the compiler may generate incorrect code for argument passing or return value handling.
* Declaring `int bar(int);` or `char bar(int);` before `main()` ensures that type expectations are matched correctly during compilation.

> Note: In 64-bit mode, return values smaller than 32 bits are also placed in `%al`, but the full 64-bit `%rax` may be zero- or sign-extended depending on the type. Type declarations remain critical for correctness in both architectures.
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
