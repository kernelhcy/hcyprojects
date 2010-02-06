CC=gcc
OBJS=test.o snake.o snake_base.o log.o snake_snake.o

test : ${OBJS}
	${CC} ${OBJS} -o test -lcurses

test.o : test.c snake.h
	${CC} -c test.c -o test.o

snake.o : snake.c snake.h log.h
	${CC} -c snake.c -o snake.o

snake_base.o : snake_base.c snake_base.h
	${CC} -c snake_base.c -o snake_base.o

snake_snake.o : snake_snake.c snake_snake.h
	${CC} -c snake_snake.c -o snake_snake.o

log.o : ../log.c ../log.h
	${CC} -c ../log.c -o log.o

clean:
	rm -rf *.o
