CC=gcc
OBJS= main.o fs.o log.o bitset.o
main : ${OBJS}
	${CC}  ${OBJS}  -o main
fs.o : fs.c fs.h log.h
	${CC}  -c  fs.c  -o fs.o
main.o : main.c
	${CC}  -c  main.c  -o main.o
log.o : log.c log.h
	${CC} -c log.c -o log.o
bitset.o : bitset.c bitset.h
	${CC} -c bitset.c -o bitset.o

clean :
	rm -f *.o fsdata
