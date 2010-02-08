CC=gcc
OBJS=test.o snake.o snake_base.o log.o snake_snake.o keyboard_ctrl.o snake_food.o
LIBS=-lcurses -lpthread
DEBUG=-g

test : ${OBJS}
	${CC} ${DEBUG} ${OBJS} -o test ${LIBS}

test.o : test.c snake.h
	${CC} ${DEBUG} -c test.c -o test.o

snake.o : snake.c snake.h log.h
	${CC} ${DEBUG} -c snake.c -o snake.o

snake_base.o : snake_base.c snake_base.h
	${CC} ${DEBUG} -c snake_base.c -o snake_base.o

snake_snake.o : snake_snake.c snake_snake.h
	${CC} ${DEBUG} -c snake_snake.c -o snake_snake.o

key_board.o : keyboard_ctrl.c keyboard_ctrl.h
	${CC} ${DEBUG} -c keyboard_ctrl.c -o keyboard_ctrl.o

snake_food.o : snake_food.c snake_food.h
	${CC} ${DEBUG} -c snake_food.c -o snake_food.o

log.o : ../log.c ../log.h
	${CC} ${DEBUG} -c ../log.c -o log.o

clean:
	rm -rf *.o
