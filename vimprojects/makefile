main : main.o OS_process_communicate.o headers.h 
	g++ main.o OS_process_communicate.o  -o main
OS_process_communicate.o : OS_process_communicate.cpp headers.h
	g++ -c OS_process_communicate.cpp  -o OS_process_communicate.o
main.o : main.cpp headers.h
	g++ -c main.cpp  -o main.o
clean :
	rm *.o
