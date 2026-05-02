#include <iostream>
#include <thread>
#include "OrderBook.hpp"
#include "Queue.hpp"

using namespace std;
using namespace Queue;
using namespace Order;

typedef std::atomic<int> atomInt;

#define RINGBUFFERSIZE 1024

void handleInput(array<orderStruct, RINGBUFFERSIZE> &ringBuf, atomInt &writeProcessed, atomInt &readProcessed, atomInt &finished) {
    int id = 1;

    // Interactive loop
    while (true) {
        string input;
        getline(cin, input);

        if (input == "q") {
            finished++;
            break;
        }

        int len = input.length();
        orderStruct order;

        if (input[0] == 'B') {
            order.action = BUY;
            string nums;

            if (input[4] == 'L') {
                order.type = LIMIT;
                nums = input.substr(10, len);
            }
            else {
                order.type = MARKET;
                nums = input.substr(11, len);
            }

            int shares = stoi(nums.substr(0, nums.find_first_of(" ")));
            double priceD = stod(nums.substr(nums.find_first_of("$") + 1, len));
            int price = (int)(priceD * 100 + 0.5);

            order.shares = shares;
            order.price = price;
        }
        else if (input[0] == 'S') {
            order.action = SELL;
            string nums;

            if (input[5] == 'L') {
                order.type = LIMIT;
                nums = input.substr(11, len);
            }
            else {
                order.type = MARKET;
                nums = input.substr(12, len);
            }

            int shares = stoi(nums.substr(0, nums.find_first_of(" ")));
            double priceD = stod(nums.substr(nums.find_first_of("$") + 1, len));
            int price = (int)(priceD * 100 + 0.5);

            order.shares = shares;
            order.price = price;
        }
        else {
            // Cancel order
            int cancelId = stoi(input.substr(7, len));
            order.action = CANCEL;
            order.id = cancelId;
            cout << "CANCELLING ORDER " << order.id << endl;

            int wp = writeProcessed.load(memory_order_relaxed); // lazily get writeProcessed (okay because writer is only thread updating it)

            while (wp == readProcessed.load(memory_order_acquire) + RINGBUFFERSIZE) {} // aggressively get the latest value of readProcessed from L3
                                                                                       // needed because consumer is constantly updating readProcessed to L3

            ringBuf[wp % RINGBUFFERSIZE] = order;
            writeProcessed.store(wp + 1, memory_order_release); // flush all L1/2 writes to shared L3 
                                                                // consumer will read writeProcessed from L3 with acquire

            continue;
        }

        if (order.type == MARKET) {
            order.price = 0;
        }

        order.id = id;
        id++;

        int wp = writeProcessed.load(memory_order_relaxed);

        while (wp == readProcessed.load(memory_order_acquire) + RINGBUFFERSIZE) {}

        ringBuf[wp % RINGBUFFERSIZE] = order;
        writeProcessed.store(wp + 1, memory_order_release); // before updating writeProcessed, flush all L1/2 writes to L3
    }
}

void processInput(array<orderStruct, RINGBUFFERSIZE> &ringBuf, atomInt &writeProcessed, atomInt &readProcessed, atomInt &finished) {
    OrderBook oB;

    // outer loop determines if work is still possible (cannot leave for then thread will exit)
    // if finished not marked by writer, more writes are possible -> possibly more work for reader to do
    // if readProcessed < writeProcessed, definitely more work for reader to do
    while (!finished || readProcessed < writeProcessed) {
        // If finished has not been marked and readProcessed != writeProcessed, nothing to do at the moment (keep spinning)
        // If finished and readProcessed == writeProcessed, all work is done and thread will safely exit
        // If not finished and readProcessed < writeProcessed, more work to do
        while (!finished && readProcessed == writeProcessed) {}

        // Only do work if readProcessed < writeProcessed
        int rp = readProcessed.load(memory_order_relaxed);
        if (rp < writeProcessed) {
            oB.processOrder(ringBuf[rp % RINGBUFFERSIZE]);
            readProcessed.store(rp + 1, memory_order_release);
        }
    }
}

int main() {
    // lock-free, shared state
    array<orderStruct, RINGBUFFERSIZE> ringBuffer;

    // Launch the two threads with std::thread
    atomInt writerProcessed{0};
    atomInt readerProcessed{0};
    atomInt finished{0};
    thread writer(handleInput, ref(ringBuffer), ref(writerProcessed), ref(readerProcessed), ref(finished));
    thread reader(processInput, ref(ringBuffer), ref(writerProcessed), ref(readerProcessed), ref(finished));

    // Join after
    writer.join(); // wait for writer thread to finish
    reader.join(); // wait for reader thread to finish

    std::cout << "DONE PROCESSING" << std::endl;
    return 0;
}