main : main.o OS4_fs.o headers.h 
	#g++ main.o OS2_process.o  -o main
	#g++ main.o OS3_memory.o  -o main
	g++ -g main.o OS4_fs.o  -o main
#OS2_process.o : OS2_process.cpp headers.h
#	g++ -c OS2_process.cpp  -o OS2_process.o

#OS3_memory.o : OS3_memory.cpp headers.h
#	g++ -c  OS3_memory.cpp  -o OS3_memory.o
OS4_fs.o : OS4_fs.cpp headers.h fs.h
	g++ -g -c  OS4_fs.cpp  -o OS4_fs.o
main.o : main.cpp headers.h
	g++ -g -c  main.cpp  -o main.o
clean :
	rm *.o main
