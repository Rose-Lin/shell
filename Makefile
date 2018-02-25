all:

%.o: %.c
	gcc -g -c $^
	
dlist.o: dlist.h dlist_node.h
