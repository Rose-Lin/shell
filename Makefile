all: shell

shell: shell.o dlist.o job_node.o tokenizer.o parser.o
	gcc -o shell shell.o dlist.o job_node.o tokenizer.o parser.o -lpthread -lrt

shell.o: shell.c shell.h dlist.h job_node.h tokenizer.h parser.h
	gcc -c -Wall shell.c

dlist.o: dlist.c dlist.h job_node.h
	gcc -c -Wall dlist.c

job_node.o: job_node.c job_node.h
	gcc -c -Wall job_node.c

tokenizer.o: tokenizer.c tokenizer.h
	gcc -c -Wall tokenizer.c

parser.o: parser.c parser.h
	gcc -c -Wall parser.c

clean:
	rm *.o shell
