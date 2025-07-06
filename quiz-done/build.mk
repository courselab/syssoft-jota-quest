p1 : p1.c
	gcc -m32 $< -o $@

p2 : p2.c
	gcc -Wall -m32  -O0 -fno-stack-protector -fno-pic -fno-pie -Wl,-no-pie $(CFLAGS) -O0 $<  -o $@

p3 : p3.c
	gcc -Wall -m32  -O0  -fno-pic -fno-pie -Wl,-no-pie $< -o $@

p4-v1 : p4.o p4a.c p4b.o
	gcc -m32 $^ -o $@

p4-v2 : p4.o libp4.a
	gcc -m32 $< -L. -Wl,-Bstatic -lp4 -Wl,-Bdynamic -o $@

p4-v3 : p4.o libp4.so
	gcc -m32 $< -L. -lp4 -o $@

p4.o p4a.o p4b.o : %.o : %.c
	gcc -m32 -c $< -o $@

libp4.a : p4a.o p4b.o
	ar rcs $@ $^

libp4.so : p4a.o p4b.o
	gcc -m32 --shared $^ -o $@

.PHONY: clean

clean:
	rm -f *.o p1 p2 p3 *.a *.so p4-v1 p4-v2 p4-v3
