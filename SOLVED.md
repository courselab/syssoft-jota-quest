# 1. Funcion added in libc.S

The function we chose to create is called ```call_mem```. It uses BIOS interrupt **0x12** to get the amount of conventional memory in kilobytes. The result is returned, as an ```int``` of course, in ```%ax```.

```asm
.global call_mem

call_mem:
        mov   $0x12,%ah               
        int   $0x12
		ret
```

# 2. Calling in main()
In ```main.c```, we call the function:
```C
  int mem_kb = call_mem();
```
however, as we only have access of an implementation of ```printf``` that prints strings (```char*```), we need to convert the number to a string.

# 3. Converting with itoa
To convert the number to a string we implemented ```itoa``` by hand. With its use, we can finally print the amount of conventional memory:
```C
  printf(itoa(mem_kb));
```

One of the values we got, for example, is:
```
    Conventional Memory: 31990
```

# PS. Sanity Checks

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