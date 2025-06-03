# SysSoft Jota Quest - Memory Detection via BIOS

## Names
**10748500 João Francisco Caprioli Barbosa Camargo de Pinho**

**11795676 Eduardo Garcia de Gaspari Valdejão**

# Project Description
*This work derives from SYSeg (https://gitlab.com/monaco/syseg)*

This exercise demonstrates bare-metal programming using the BIOS service. The challenge involves:
1. Studying the code provided.
2. Writing a custom assembly function that uses the BIOS service.
3. Integrating the function with a minimal C runtime (`main.c`) to print the result.
4. Building and running the code in a QEMU emulator without an operating system.

# Our Solution
## 1. Funcion added in libc.S

The function we chose to create is called ```call_mem```. It uses BIOS interrupt **0x12** to get the amount of conventional memory in kilobytes. The result is returned, as an ```int``` of course, in ```%ax```.

```asm
.global call_mem

call_mem:
        mov   $0x12,%ah               
        int   $0x12
		ret
```

## 2. Calling in main()
In ```main.c```, we call the function:
```C
  int mem_kb = call_mem();
```
however, as we only have access of an implementation of ```printf``` that prints strings (```char*```), we need to convert the number to a string.

## 3. Converting with itoa
To convert the number to a string we implemented ```itoa``` by hand. With its use, we can finally print the amount of conventional memory:
```C
  printf(itoa(mem_kb));
```

One of the values we got, for example, is:
```
    Conventional Memory: 31990
```

## PS. Sanity Checks

We added the declaration of the function to the local implementation of the header ```stdio.h```:

```h
int __attribute__((fastcall, naked)) call_mem(void);
```
With this, we avoid the warning:
```
main.c: In function ‘main’:
main.c:26:16: warning: implicit declaration of function ‘call_mem’ [-Wimplicit-function-declaration]
   26 |   int mem_kb = call_mem();
      |                ^~~~~~~~
gcc -m16 main.o libc.o -nostartfiles -nostdlib -T hello.ld -orphan-handling=discard -o hello.bin
```

# How to Build and Run

### Dependencies
- GCC (with `-m16` support)
- GNU Make
- QEMU (i386 emulator)
- (Optional) GTK3 for graphical QEMU output

### Instructions
1. **Clone the repository**:
   ```bash
   git clone [repository-url]
   cd syssoft-jota-quest/
   ```

2. **Clean previous builds** (if any):
   ```bash
   rm -f main.o hello.bin
   ```

3. **Build the binary**:
   ```bash
   make hello.bin
   ```

4. **Run in QEMU** (choose one method):

   - **Graphical mode** (requires GTK):
     ```bash
     make hello.bin/run
     ```
     *If you get GTK errors, install dependencies:*
     ```bash
     sudo apt install qemu-system-x86 libgtk-3-dev  # Debian/Ubuntu
     ```

   - **Text-only mode** (recommended for SSH/headless):
     ```bash
     qemu-system-i386 -nographic -drive format=raw,file=hello.bin -net none
     ```
     *(Press `Ctrl+A` then `X` to exit QEMU)*
