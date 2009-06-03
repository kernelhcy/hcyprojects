main : main.o OS2_process.o headers.h 
	g++ main.o OS2_process.o  -o main
OS_process_communicate.o : OS_process.cpp headers.h
	g++ -c OS2_process.cpp  -o OS2_process.o
main.o : main.cpp headers.h
	g++ -c main.cpp  -o main.o
clean :
	rm *.o main
