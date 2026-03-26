#ifndef QUEUE
#define QUEUE

#include <string>

using namespace std;

namespace Queue {


    // orderStruct
    struct orderStruct {
        string action;
        string type;
        int shares;
        double price;
        int id;

        orderStruct () {
            id = -1;
        }

        orderStruct (string a, string t, int s, double p, int i) {
            action = a;
            type = t;
            shares = s;
            price = p;
            id = i;
        }
    };
}


#endif