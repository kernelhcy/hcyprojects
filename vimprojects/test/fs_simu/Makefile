CC=gcc
OBJS= main.o fs.o log.o bitset.o dir.o fs_base.o user.o
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
dir.o : dir.c dir.h
	${CC} dir.c -o dir.o

fs_base.o : fs_base.c fs_base.h
	${CC} fs_base.c -o fs_base.o

user.o : user.c user.h
	${CC} user.c -c user.o

clean :
	rm -f *.o fsdata
