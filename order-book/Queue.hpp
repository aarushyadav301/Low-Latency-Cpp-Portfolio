#ifndef QUEUE
#define QUEUE

#include <string>

using namespace std;

namespace Queue {
    enum orderAction {
        BUY,
        SELL,
        CANCEL
    };

    enum orderType {
        LIMIT, 
        MARKET
    };

    // orderStruct
    struct orderStruct {
        orderAction action;
        orderType type;
        int shares;
        double price;
        int id;

        orderStruct () {
            id = -1;
        }

        orderStruct (orderAction a, orderType t, int s, double p, int i) {
            action = a;
            type = t;
            shares = s;
            price = p;
            id = i;
        }
    };
}


#endif