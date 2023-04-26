FLAGS:=-m32 -Wall -g

myshell: bin/myshell.o bin/LineParser.o
	gcc $(FLAGS) bin/myshell.o bin/LineParser.o -o bin/myshell

bin/myshell.o: src/myshell.c
	gcc $(FLAGS) -c src/myshell.c -o bin/myshell.o

mypipe: src/mypipe.c
	gcc $(FLAGS) src/mypipe.c -o bin/mypipe

bin/LineParser.o: src/LineParser.c
	gcc $(FLAGS) -c src/LineParser.c -o bin/LineParser.o

looper: src/looper.c
	gcc $(FLAGS) src/looper.c -o bin/looper

mypipeline: src/mypipeline.c
	gcc $(FLAGS) src/mypipeline.c -o bin/mypipeline

.PHONY: cleanshell cleanpipe cleanlooper

cleanshell:
	rm -f bin/myshell.o bin/myshell

cleanpipe:
	rm -f bin/mypipe.o bin/mypipe

cleanlooper:
	rm -f bin/looper.o bin/looper

#TODO: understand what's causing the "Circular..." warning!