main : main.o OS3_memory.o headers.h 
	#g++ main.o OS2_process.o  -o main
	g++   main.o OS3_memory.o  -o main
#OS2_process.o : OS2_process.cpp headers.h
#	g++ -c OS2_process.cpp  -o OS2_process.o

OS3_memory.o : OS3_memory.cpp headers.h
	g++ -c  OS3_memory.cpp  -o OS3_memory.o
main.o : main.cpp headers.h
	g++ -c  main.cpp  -o main.o
clean :
	rm *.o main
