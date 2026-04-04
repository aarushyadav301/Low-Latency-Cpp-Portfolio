.PHONY: clean

clean: 
	rm *.o *.txt

benchmark: benchmark.cpp OrderBook.cpp OrderBook.hpp Queue.cpp Queue.hpp
	g++ -O3 \
	-I/opt/homebrew/opt/google-benchmark/include/ \
	-L/opt/homebrew/opt/google-benchmark/lib/ \
	benchmark.cpp OrderBook.cpp Queue.cpp -lbenchmark -lbenchmark_main -o benchmark_runner

OrderBook.o: OrderBook.cpp OrderBook.hpp
	g++ -c -O3 OrderBook.cpp

Queue.o: Queue.cpp Queue.hpp
	g++ -c -O3 Queue.cpp

main.o: main.cpp
	g++ -c -O3 main.cpp 

program: OrderBook.o Queue.o main.o 
	g++ OrderBook.o Queue.o main.o -o program



