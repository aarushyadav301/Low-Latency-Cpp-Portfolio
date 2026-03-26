.PHONY: clean

clean: 
	rm *.o

OrderBook.o: OrderBook.cpp
	g++ -c OrderBook.cpp

Queue.o: Queue.cpp
	g++ -c Queue.cpp

main.o: main.cpp
	g++ -c main.cpp 

program: OrderBook.o Queue.o main.o 
	g++ OrderBook.o Queue.o main.o -o program



