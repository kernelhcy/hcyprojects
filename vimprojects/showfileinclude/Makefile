main: main.o log.o digraph.o out.o parser.o
	gcc -g main.o digraph.o log.o out.o parser.o  -o main
main.o: main.c digraph.h headers.h base.h out.h
	gcc -g -c main.c -o main.o
digraph.o: digraph.c digraph.h headers.h base.h
	gcc -g -c digraph.c -o digraph.o
log.o: log.c log.h
	gcc -g -c log.c -o log.o
out.o: out.c out.h headers.h
	gcc -g -c out.c -o out.o
parser.o: parser.c parser.h headers.h digraph.h
	gcc -g -c parser.c -o parser.o
clean:
	rm -rf *.o
#	rm -rf main
