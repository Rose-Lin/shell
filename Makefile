all: shell

shell: shell.o dlist.o job_node.o tokenizer.o
	gcc -o shell shell.o dlist.o job_node.o tokenizer.o

shell.o: shell.c shell.h dlist.h job_node.h tokenizer.h
	gcc -c shell.c

dlist.o: dlist.c dlist.h job_node.h
	gcc -c dlist.c

job_node.o: job_node.c job_node.h
	gcc -c job_node.c

tokenizer.o: tokenizer.c tokenizer.h
	gcc -c tokenizer.c


clean:
	rm *.o shell
