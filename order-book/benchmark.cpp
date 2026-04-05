#include <benchmark/benchmark.h>
#include "OrderBook.hpp"

#include <random>
#include <iostream>

using namespace std;
using namespace Queue;
using namespace Order;

void BM_shallowLatency(benchmark::State& state) {
    OrderBook oB;

    for (auto _ : state) {
        state.PauseTiming();
        oB.reset();
        int id = 1;
        default_random_engine generator(random_device{}());

        while (oB.getSize() <= 100) {
            uniform_int_distribution<int> actDistr(0, 1);

            int numAction = actDistr(generator);
            orderAction action;
            if (numAction == 0) {
                action = BUY;
            }
            else {
                action = SELL;
            }

            orderType type = LIMIT;

            uniform_int_distribution<int> sharesDistr(1, 20);
            int numShares = sharesDistr(generator);

            // For 100 orders, the two means are 2.5 std devs apart
            double limit;
            if (numAction == 0) {
                normal_distribution<double> buyDistr(10.0, 4.0);
                limit = max(0.0, buyDistr(generator));
            }   
            else {
                normal_distribution<double> sellDistr(20.0, 4.0);
                limit = max(0.0, sellDistr(generator));
            }

            orderStruct oS(action, type, numShares, limit, id++);
            oB.processOrder(oS);
        }

        uniform_int_distribution<int> actDistr(1, 5);
        int act = actDistr(generator);
        
        orderAction action;
        orderType type = LIMIT;

        switch (act) {
            case 1:
                action = BUY;
                type = LIMIT;
                break;
            case 2:
                action = SELL;
                type = LIMIT;
                break;
            case 3:
                action = BUY;
                type = MARKET;
                break;
            case 4:
                action = SELL;
                type = MARKET;
                break;
            case 5:
                action = CANCEL;
        }

        uniform_int_distribution<int> sharesDistr(1, 20);
        int shares = sharesDistr(generator);

        double price = 0.0;

        if (action == BUY) {
            normal_distribution<double> buyDistr(10.0, 4.0);
            price = buyDistr(generator);
        }
        else if (action == SELL) {
            normal_distribution<double> sellDistr(20.0, 4.0);
            price = sellDistr(generator);
        }

        orderStruct oS;

        if (action == CANCEL) {
            oS.action = CANCEL;

            uniform_int_distribution<int> cancelDistr(1, id);
            oS.id = cancelDistr(generator);
        }
        else {
            oS.action = action;
            oS.type = type;
            oS.shares = shares;
            oS.price = price;
            oS.id = id++;
        }
        state.ResumeTiming();

        // Single order latency
        oB.processOrder(oS);
    }
}


BENCHMARK(BM_shallowLatency);
