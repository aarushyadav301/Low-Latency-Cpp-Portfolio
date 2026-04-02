#include "Queue.hpp"

#ifndef ORDERBOOK
#define ORDERBOOK

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

using namespace std;
using namespace Queue;

namespace Order {
    class OrderBook {
        public:
            OrderBook();
            string processOrder(orderStruct oS);
    
            // Market info methods
            double getSpread();
    
        private:
            /* 
             * Matching Engine Methods: 
             *
             *  
             * 
             * 
             * 
             * 
             * 
             * 
             */ 
            unordered_map<int, orderStruct> pendingOrders;
            unordered_map<int, orderStruct> processedOrders;
            unordered_set<int> buyOrders;
    
            // Index (Id) -> Heap location
            vector<int> sellHeapMap{1000};
            vector<int> buyHeapMap{1000};

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
             * Both heaps are declared with size 200 to avoid resizing. 
             * 
             */ 
            
            vector <orderStruct> sellLimitHeap{1000};
            vector <orderStruct> buyLimitHeap{1000};
            int sellLimitSize = 0;
            int buyLimitSize = 0;
    
            inline
            int findHeapParent(int child);
    
            void sellLimitInsert (orderStruct oS);
            void buyLimitInsert (orderStruct oS);
    
            orderStruct getMinAsk();
            orderStruct getMaxBid();
    
            void removeAsk(int heapLoc);
            void removeBid(int heapLoc);
    
    
            // Market Info Methods: 
            double bestAsk;
            double bestBid;
    };    
}

#endif