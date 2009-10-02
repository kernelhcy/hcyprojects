main : main.o apue.o ch1.o
	gcc main.o apue.o ch1.o -o main
	rm *.o
main.o : main.c apue.h
	gcc -c  main.c  -o main.o

apue.o : apue.c apue.h
	gcc -c  apue.c -o apue.o

ch1.o : ch1.c ch1.h
	gcc -c  ch1.c -o ch1.o
