.PHONY: clean

clean: 
	rm *.o

OrderBook.o: OrderBook.cpp OrderBook.hpp
	g++ -c -O2 OrderBook.cpp

Queue.o: Queue.cpp Queue.hpp
	g++ -c -O2 Queue.cpp

main.o: main.cpp
	g++ -c -O2 main.cpp 

program: OrderBook.o Queue.o main.o 
	g++ OrderBook.o Queue.o main.o -o program



