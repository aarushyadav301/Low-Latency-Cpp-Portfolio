#include "Queue.hpp"

#ifndef ORDERBOOK
#define ORDERBOOK

#include <array>
#include <string>

using namespace std;
using namespace Queue;

namespace Order {

    static constexpr int MAX_ORDERS = 10005;

    class OrderBook {
        public:
            OrderBook();
            string processOrder(orderStruct oS);
            void reset();
    
            // Market info methods
            int getSpread();
            int getLastTradedPrice();
            int getSize();
    
        private:
            /* 
             * Matching Engine Methods: 
             */ 
            array<bool, MAX_ORDERS> pendingOrders{};
            array<bool, MAX_ORDERS> processedOrders{};
            array<bool, MAX_ORDERS> buyOrders{};

            string cancelOrder(int id);
    
            string buyMarketOrder(orderStruct oS);
            string sellMarketOrder(orderStruct oS);
    
            string buyLimitOrder(orderStruct oS);
            string sellLimitOrder(orderStruct oS);
    
            /* 
             * Heap Methods: 
             * Sell limit orders are sorted in ascending order (min heap) 
             * so market buy orders can walk up easily through the ladder 
             * and complete transactions. 
             * 
             * 
             * Buy limit orders are sorted in descending orders (max heap)
             * market sell orders can walk down the ladder.
             * 
             * Both heaps use std::array for inline storage (no pointer 
             * indirection). All heap methods defined inline so the compiler 
             * can fold them into the matching engine hot path.
             *
             * Insert and remove use single-pass sift: the displaced element
             * is held aside while parents/children shift in place, then 
             * written once at its final position (halves writes vs swap).
             * 
             */ 
            
            array<orderStruct, MAX_ORDERS> sellLimitHeap;
            array<orderStruct, MAX_ORDERS> buyLimitHeap;

            // Index (Id) -> Heap location
            array<int, MAX_ORDERS> sellHeapMap{};
            array<int, MAX_ORDERS> buyHeapMap{};

            int sellLimitSize = 0;
            int buyLimitSize = 0;
    
            int findHeapParent(int child);
            const orderStruct& getMinAsk();
            const orderStruct& getMaxBid();

            void sellLimitInsert(orderStruct oS);
            void buyLimitInsert(orderStruct oS);
            void removeAsk(int heapLoc);
            void removeBid(int heapLoc);

            int lastTradedPrice;
    };    
}

#endif