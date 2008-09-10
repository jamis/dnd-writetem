all: libwritetem.a

clean:
	rm -f src/*.o src/*.a

libwritetem.a: src/writetem.o src/wtstream.o
	ar -rc src/libwritetem.a src/writetem.o src/wtstream.o
	ranlib src/libwritetem.a

src/writetem.o: src/writetem.c include/*.h
	gcc -c -Iinclude -o src/writetem.o src/writetem.c

src/wtstream.o: src/wtstream.c include/wtstream.h
	gcc -c -Iinclude -o src/wtstream.o src/wtstream.c

