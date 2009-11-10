main: main.o log.o digraph.o out.o parser.o buffer.o path.o pathtree.o
	gcc -g main.o digraph.o log.o out.o parser.o buffer.o path.o pathtree.o  -o main
main.o: main.c digraph.h headers.h base.h out.h
	gcc -g -c main.c -o main.o
digraph.o: digraph.c digraph.h headers.h base.h
	gcc -g -c digraph.c -o digraph.o
log.o: log.c log.h
	gcc -g -c log.c -o log.o
out.o: out.c out.h headers.h buffer.h
	gcc -g -c out.c -o out.o
parser.o: parser.c parser.h headers.h digraph.h
	gcc -g -c parser.c -o parser.o
buffer.o: buffer.c buffer.h log.h headers.h
	gcc -g -c buffer.c -o buffer.o
path.o: path.c path.h log.h buffer.c headers.h
	gcc -g -c path.c -o path.o
pathtree.o: pathtree.c pathtree.h buffer.h
	gcc -g -c pathtree.c -o pathtree.o
clean:
	rm -rf *.o
#	rm -rf main
