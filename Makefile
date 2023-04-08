CC = gcc
CC_ARGS = -std=c99 -lm

main: main.o
	$(CC) -o main main.o $(CC_ARGS)

main.o: main.c
	$(CC) -c main.c $(CC_ARGS)

clean:
	rm *.o
	rm ./main

run: main
	./main
