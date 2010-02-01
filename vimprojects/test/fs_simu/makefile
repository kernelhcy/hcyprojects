main: main.o fs.o 
	g++  main.o fs.o  -o main
fs.o : fs.cpp fs.h
	g++  -c  fs.cpp  -o fs.o
main.o : main.cpp
	g++  -c  main.cpp  -o main.o

clean :
	rm *.o main fsdata
