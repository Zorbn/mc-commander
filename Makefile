CC = gcc
CC_ARGS = -std=c99

main: main.o
	$(CC) $(CC_ARGS) -o main main.o

main.o: main.c
	$(CC) $(CC_ARGS) -c main.c

clean:
	rm *.o
	rm ./main

run: main
	./main
