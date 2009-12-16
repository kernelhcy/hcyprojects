HD = libemail/headers.h
OBJECTS = main.o libemail/buffer.o libemail/log.o \
	libemail/libemail.o libemail/base64/base64.o libemail/html.o
DEBUG = -g

main : ${OBJECTS} ${HD}
	gcc ${DEBUG} ${OBJECTS} -o main

main.o : main.c 
	gcc -c ${DEBUG} main.c -o main.o

buffer.o : libemail/buffer.c ${HD} libemail/buffer.h
	gcc -c ${DEBUG} libemail/buffer.c -o libemail/buffer.o

log.o : libemail/log.c libemail/log.h ${HD}
	gcc -c ${DEBUG} libemail/log.c -o libemail/log.o

libemail.o : libemail/libemail.c libemail/libemail.h ${HD}
	gcc -c ${DEBUG} libemail/libemail.c -o libemail/libemail.o

base64.o : libemail/base64/base64.c libemail/base64/base64.h ${HD}
	gcc -c ${DEBUG} libemail/base64/base64.c -o  libemail/base64/base64.o

html.o: libemail/html.c ${HD} libemail/html.h
	gcc -c ${DEBUG} libemail/html.c -o libemail/html.o

.PHONY : clean
clean :
	-rm *.o libemail/*.o libemail/base64/*.o
	-rm main
