main: main.o textquery.o textquery.h
	g++ main.o textquery.o -o main
textquery.o: textquery.cpp textquery.h
	g++ -c textquery.cpp -o textquery.o
main.o: main.cpp textquery.h
	g++ -c main.cpp -o main.o
clean: 
	rm -rf *.o
	rm -rf main
