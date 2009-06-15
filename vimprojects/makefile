OS4: main4.o OS4_fs.o headers.h 
	g++  main4.o OS4_fs.o  -o main
OS4_fs.o : OS4_fs.cpp headers.h fs.h
	g++  -c  OS4_fs.cpp  -o OS4_fs.o
main4.o : main.cpp headers.h
	g++  -c  main.cpp  -o main4.o

OS3: main3.o OS3_memory.o headers.h 
	g++ main3.o OS3_memory.o  -o main
OS3_memory.o : OS3_memory.cpp headers.h
	g++ -c  OS3_memory.cpp  -o OS3_memory.o
main3.o : main.cpp headers.h
	g++  -c  main.cpp  -o main3.o

OS2: main2.o OS2_process.o headers.h 
	g++ main2.o OS2_process.o  -o main
OS2_process.o : OS2_process.cpp headers.h
	g++ -c OS2_process.cpp  -o OS2_process.o
main2.o : main.cpp headers.h
	g++  -c  main.cpp  -o main2.o


debug : maind.o OS4_fsd.o headers.h 
	#g++ -g maind.o OS2_processd.o  -o main
	#g++ -g  maind.o OS3_memoryd.o  -o main
	g++ -g maind.o OS4_fsd.o  -o main
#OS2_processd.o : OS2_process.cpp headers.h
#	g++ -c -g OS2_process.cpp  -o OS2_processd.o

#OS3_memoryd.o : OS3_memory.cpp headers.h
#	g++ -c -g OS3_memory.cpp  -o OS3_memoryd.o
OS4_fsd.o : OS4_fs.cpp headers.h fs.h
	g++ -g -c  OS4_fs.cpp  -o OS4_fsd.o
main.o : main.cpp headers.h
	g++ -g -c  main.cpp  -o maind.o


clean :
	rm *.o main fsdata
