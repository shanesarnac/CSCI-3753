CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -Wall -Wextra -pthread

multi_lookup: multi-lookup.o queue.o util.o
	$(CC) $(LFLAGS) $^ -o $@
	
multi_lookup.o: multi-lookup.c multi_lookup.h
	$(CC) $(CFLAGS) $<

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) $<

util.o: util.c util.h
	$(CC) $(CFLAGS) $<

clean:
	rm -f multi-lookup
	rm -f *.o
	rm -f *~
	rm -f results.txt

#multi_lookup: main.c queue.c util.c multi_lookup.c
#	gcc -Wall -Wextra -pthread -std=c++11 -g main.c multi_lookup.c queue.c util.c -lm -o numerical_analysis

#clean: 
#	rm *.o
