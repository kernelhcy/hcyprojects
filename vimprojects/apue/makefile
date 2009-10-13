main : main.o apue.o ch11.o
	gcc main.o apue.o ch11.o -o main -lpthread

main.o : main.c apue.h
	gcc -c  main.c  -o main.o

apue.o : apue.c apue.h
	gcc -c  apue.c -o apue.o

ch1.o : ch1.c ch1.h
	gcc -c  ch1.c -o ch1.o
	
ch2.o : ch2.c ch2.h
	gcc -c  ch2.c -o ch2.o
	
ch3.o : ch3.c ch3.h
	gcc -c  ch3.c -o ch3.o

ch10.o : ch10.c ch10.h
	gcc -c  ch10.c -o ch10.o
	
ch11.o : ch11.c ch11.h
	gcc -c  ch11.c -o ch11.o 

clean:
	rm *.o main
