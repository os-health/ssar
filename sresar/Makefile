default:
	gcc -g -std=gnu99 -rdynamic -c toml.c        -o toml.o
	gcc -g -std=gnu99 -rdynamic -c utils.c       -o utils.o
	gcc -g -std=gnu99 -rdynamic -c collection.c  -o collection.o
	gcc -g -std=gnu99 -rdynamic -c readprocess.c -o readprocess.o
	gcc -g -std=gnu99 -rdynamic -c sresar.c      -o sresar.o
	gcc -g -std=gnu99 -rdynamic toml.o utils.o collection.o readprocess.o sresar.o -o sresar -lpthread -lm -lz
clean:
	rm -fr sresar
	rm -fr toml.o utils.o collection.o readprocess.o sresar.o
