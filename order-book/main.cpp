#include <iostream>
#include <thread>
#include "OrderBook.hpp"
#include "Queue.hpp"

using namespace std;
using namespace Queue;
using namespace Order;

int main() {

    OrderBook oB;
    int id = 1;

    // Testing loop
    while (true) {
        string input;
        getline(cin, input);

        if (input == "q") {
            break;
        }

        int len = input.length();

        orderStruct order;
        if (input[0] == 'B') {
            order.action = "BUY";
            string nums;

            if (input[4] == 'L') {
                order.type = "LIMIT";
                nums = input.substr(10, len);
            }
            else {
                order.type = "MARKET";
                nums = input.substr(11, len);
            }

            int shares = stoi(nums.substr(0, nums.find_first_of(" ")));
            double price = stod(nums.substr(nums.find_first_of("$") + 1, len));

            order.shares = shares;
            order.price = price;
        }
        else {
            order.action = "SELL";
            string nums;

            if (input[5] == 'L') {
                order.type = "LIMIT";
                nums = input.substr(11, len);
            }
            else {
                order.type = "MARKET";
                nums = input.substr(12, len);
            }

            int shares = stoi(nums.substr(0, nums.find_first_of(" ")));
            double price = stod(nums.substr(nums.find_first_of("$") + 1, len));
            order.shares = shares;
            order.price = price;
        }

        if (order.type == "MARKET") {
            order.price = 0.0;
        }

        order.id = id;

        cout << "ACTION: " << order.action << " TYPE: " << order.type << " SHARES: " << order.shares <<  " PRICE: " << order.price << " ID: " << order.id << endl << endl;
        oB.processOrder(order);
        id++;
    }
}