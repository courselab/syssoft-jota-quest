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

---

### a) Running each version

```bash
./p4-v1
./p4-v2
./p4-v3
```

* Both `p4-v1` and `p4-v2` print:

  ```
  Foo
  ```

* Running `p4-v3` results in:

  ```
  ./p4-v3: error while loading shared libraries: libp4.so: cannot open shared object file: No such file or directory
  ```

This occurs because the dynamic linker cannot find `libp4.so` at runtime.

**To fix this**, modify the `p4-v3` build rule to embed a runtime search path (`rpath`) pointing to the executable’s directory:

```make
p4-v3: p4.o libp4.so
	gcc -m32 $< -L. -Wl,-rpath,'$$ORIGIN' -lp4 -o $@
```

`$$ORIGIN` tells the loader to look for shared libraries relative to the executable location, ensuring `libp4.so` is found regardless of the current working directory.

Running again:

```bash
make clean
make p4-v3
./p4-v3
```

* Resulting:
  ```
  Foo
  ```

---

### b) Size comparison

Check file sizes with:

```bash
ls -lh p4-v*
```

Typical results:

* `p4-v1`: largest size — all objects linked directly into the binary
* `p4-v2`: smaller — links static library `.a`
* `p4-v3`: smallest — dynamically links shared library `.so`, binary only contains loader references

Since the program is trivial (only calls foo() printing "Foo"), the overall binary is tiny, and the overhead of ELF headers, runtime startup, and standard libs dominates the file size (~15 KB), making the difference negligible.

---

### c) Symbol table comparison

Use `nm` to inspect the symbol resolution in each version:

```bash
nm p4-v1 | grep foo
nm p4-v2 | grep foo
nm p4-v3 | grep foo

nm p4-v1 | grep bar
nm p4-v2 | grep bar
nm p4-v3 | grep bar
```

Results:

* `p4-v1` shows:

  ```
  000011b3 T foo
  000011de T bar
  ```

  Both functions `foo` and `bar` are **defined inside the executable** (`T` = text section symbol defined in the binary).

* `p4-v2` shows:

  ```
  000011b3 T foo
  ```

  `foo` is defined in the static library (`libp4.a`) and included in the executable.
  `bar` is not included because it is **not called** — static linking includes only the used parts of the library.

* `p4-v3` shows:

  ```
           U foo
  ```

  `foo` is **undefined** (`U`) in the executable because it is expected to be resolved at **runtime** by the dynamic linker using `libp4.so`.
  `bar` is also not listed because it is not used, and the dynamic linker only resolves needed symbols.

**Interpretation:**

| Version | `foo` | `bar` | Notes                                                |
| ------- | ----- | ----- | ---------------------------------------------------- |
| `p4-v1` | `T`   | `T`   | Both symbols are linked directly (object files used) |
| `p4-v2` | `T`   | —     | Only `foo` is included from the static library       |
| `p4-v3` | `U`   | —     | `foo` expected to be resolved at runtime             |

Use of `T` and `U` symbols aligns with how static and dynamic linking works:

* `T`: Defined in the executable
* `U`: Undefined — to be resolved externally at link or load time

> Note: In both `p4-v2` and `p4-v3`, `bar` is excluded because it is never referenced in the program. Static and dynamic linkers avoid unnecessary inclusion of unused symbols to reduce size and overhead.

---

### d) Dynamic section inspection

Inspect shared library dependencies with:

```bash
readelf -d p4-v1
readelf -d p4-v2
readelf -d p4-v3
```

#### Observations

* **`p4-v1` and `p4-v2`** both have:

  ```
  NEEDED             Shared library: [libc.so.6]
  ```

  This means:

  * Both binaries link dynamically to the C standard library (`libc.so.6`)
  * Even though `p4-v2` uses a static custom library (`libp4.a`), the system library is still dynamically linked

* **`p4-v3`** includes:

  ```
  NEEDED             Shared library: [libp4.so]
  NEEDED             Shared library: [libc.so.6]
  RUNPATH            Library runpath: [$ORIGIN]
  ```

  This means:

  * `p4-v3` depends on `libp4.so` at runtime
  * The `RUNPATH` of `$ORIGIN` was correctly added via the `-Wl,-rpath,'$$ORIGIN'` linker flag, which ensures the dynamic linker will search for `libp4.so` in the same directory as the executable

#### Interpretation

* `p4-v1`: all object files linked directly into the binary, only standard libraries resolved dynamically
* `p4-v2`: static custom library (`libp4.a`), dynamic standard libraries
* `p4-v3`: dynamic custom and standard libraries

#### How the dynamic linker uses this

At runtime, the kernel's loader (`ld-linux.so.2` on x86) reads the **dynamic section** to find out which `.so` files must be loaded. The `NEEDED` and `RUNPATH` entries in `p4-v3` tell it:

1. Load `libp4.so` from the same directory as the executable
2. Load `libc.so.6` from the system paths
3. Resolve all undefined symbols from those libraries before handing control to the program

---

### e) Static vs dynamic libraries

| Scenario                                     | Static Library                           | Dynamic Library                          |
| -------------------------------------------- | ---------------------------------------- | ---------------------------------------- |
| Deploying on other systems                   | Easier, single executable, no .so needed | Must provide `.so` files or install them |
| Updating the library                         | Requires recompilation of executables    | Updated `.so` is used automatically      |
| RAM usage when multiple apps use the library | Each executable loads its own copy       | Shared code loaded once in memory        |
| Binary size                                  | Larger executable size                   | Smaller executables                      |

---

## dyn – Static vs Dynamic vs PIC

### a) Static library

* Archive `.a` with object files
* Linked into executable at compile time
* No runtime dependencies
* Larger executable size, less flexible updates

### b) Dynamic library with relocatable code (non-PIC)

* Code relocatable at load time
* Shared at runtime but may cause runtime overhead for relocations
* Less efficient and flexible than PIC

### c) Dynamic library with position-independent code (PIC)

* Fully position-independent, no relocation overhead
* Efficient memory sharing between processes
* Preferred for modern `.so` libraries

### Summary

| Type            | Portability | Efficiency | Updatable | Memory Sharing |
| --------------- | ----------- | ---------- | --------- | -------------- |
| Static          | High        | Low        | No        | No             |
| Dynamic (reloc) | Medium      | Medium     | Yes       | Yes            |
| Dynamic (PIC)   | High        | High       | Yes       | Yes            |

---

---
