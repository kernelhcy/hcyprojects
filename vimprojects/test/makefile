main : data_string.o data_count.o data_array.o buffer.o array.o array.h
	gcc data_string.o data_count.o data_array.o  buffer.o array.o -o main
data_string.o : data_string.c array.h
	gcc -c data_string.c -o data_string.o
data_count.o : data_count.c array.h
	gcc -c data_count.c -o data_count.o
data_array.o : data_array.c array.h
	gcc -c data_array.c -o data_array.o
buffer.o : buffer.c buffer.h
	gcc -c buffer.c -o buffer.o
array.o : array.c array.h
	gcc -c array.c -o array.o
clean : 
	rm *.o
