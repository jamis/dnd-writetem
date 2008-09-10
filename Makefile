all: libtemplates.a

clean:
	rm -f src/*.o src/*.a

libtemplates.a: src/writetem.o src/wtstream.o
	ar -rc src/libtemplates.a src/writetem.o src/wtstream.o
	ranlib src/libtemplates.a

src/writetem.o: src/writetem.c include/*.h
	gcc -c -Iinclude -o src/writetem.o src/writetem.c

src/wtstream.o: src/wtstream.c include/wtstream.h
	gcc -c -Iinclude -o src/wtstream.o src/wtstream.c

