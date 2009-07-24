main : main.o ourhdr.o ourhdr.h
	gcc main.o ourhdr.o -o main

main.o : main.c ourhdr.h
	gcc -c  main.c  -o main.o

ourhdr.o : ourhdr.c ourhdr.h
	gcc -c  ourhdr.c -o ourhdr.o
