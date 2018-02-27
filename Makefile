all: shell

shell: shell.o dlist.o job_node.o tokenizer.o
	gcc -o shell shell.o dlist.o job_node.o tokenizer.o -lpthread -lrt

shell.o: shell.c shell.h dlist.h job_node.h tokenizer.h
	gcc -c -Wall shell.c

dlist.o: dlist.c dlist.h job_node.h
	gcc -c -Wall dlist.c

job_node.o: job_node.c job_node.h
	gcc -c -Wall job_node.c

tokenizer.o: tokenizer.c tokenizer.h
	gcc -c -Wall tokenizer.c


clean:
	rm *.o shell
