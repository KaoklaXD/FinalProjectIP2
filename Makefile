CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

COMMON_SRCS = graph.c storage.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

all: test staff visitor

test: test.o graph.o recommendation.o storage.o map_visualizer.o
	$(CC) $(CFLAGS) -o test test.o graph.o recommendation.o storage.o map_visualizer.o

staff: Staff_input.o graph.o storage.o
	$(CC) $(CFLAGS) -o staff Staff_input.o graph.o storage.o

visitor: Visitor_input.o graph.o recommendation.o storage.o map_visualizer.o
	$(CC) $(CFLAGS) -o visitor Visitor_input.o graph.o recommendation.o storage.o map_visualizer.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o test staff visitor

.PHONY: all clean

