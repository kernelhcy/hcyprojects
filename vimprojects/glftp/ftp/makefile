test: test.o ftp.o log.o  ftp.h
	gcc test.o ftp.o log.o  -o test
test.o: test.c ftp.h log.h
	gcc -c test.c -o test.o
ftp.o: ftp.c ftp.h log.h
	gcc -c ftp.c -o ftp.o
log.o: log.c log.h
	gcc -c log.c -o log.o
clean:
	rm -rf *.o
