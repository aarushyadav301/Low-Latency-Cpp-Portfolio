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
             *
             *  
             * 
             * 
             * 
             * 
             * 
             * 
             */ 
            array<bool, MAX_ORDERS> pendingOrders{};
            array<bool, MAX_ORDERS> processedOrders{};
            array<bool, MAX_ORDERS> buyOrders{};
    
            // Index (Id) -> Heap location
            array<int, MAX_ORDERS> sellHeapMap{};
            array<int, MAX_ORDERS> buyHeapMap{};

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
            int sellLimitSize = 0;
            int buyLimitSize = 0;
    
            inline
            int findHeapParent(int child) {
                return ((child - 1) / 2);
            }

            inline
            void sellLimitInsert(orderStruct oS) {
                int cur = sellLimitSize++;

                //cout << "order struct " << oS.id << " inserted in sell heap at " << cur << endl;

                while (cur > 0 && sellLimitHeap[findHeapParent(cur)].price > oS.price) {
                    sellLimitHeap[cur] = sellLimitHeap[findHeapParent(cur)];
                    sellHeapMap[sellLimitHeap[cur].id] = cur;
                    cur = findHeapParent(cur);

                    //cout << "order struct " << sellLimitHeap[cur].id << " shifted to " << cur << endl;
                }

                sellLimitHeap[cur] = oS;
                sellHeapMap[oS.id] = cur;
            }

            inline
            void buyLimitInsert(orderStruct oS) {
                int cur = buyLimitSize++;

                //cout << "order struct " << oS.id << " inserted in buy heap at " << cur << endl;

                while (cur > 0 && buyLimitHeap[findHeapParent(cur)].price < oS.price) {
                    buyLimitHeap[cur] = buyLimitHeap[findHeapParent(cur)];
                    buyHeapMap[buyLimitHeap[cur].id] = cur;
                    cur = findHeapParent(cur);

                    //cout << "order struct " << buyLimitHeap[cur].id << " shifted to " << cur << endl;
                }

                buyLimitHeap[cur] = oS;
                buyHeapMap[oS.id] = cur;
            }

            inline
            const orderStruct& getMinAsk() {
                return (sellLimitHeap[0]);
            }

            inline
            const orderStruct& getMaxBid() {
                return (buyLimitHeap[0]);
            }

            inline
            void removeAsk(int heapLoc) {
                sellLimitSize--;
                sellHeapMap[sellLimitHeap[heapLoc].id] = -1;

                if (heapLoc == sellLimitSize) {
                    sellLimitHeap[heapLoc].id = -1;
                    return;
                }

                orderStruct moving = sellLimitHeap[sellLimitSize];
                sellLimitHeap[sellLimitSize].id = -1;

                int cur = heapLoc;

                while (cur > 0 && moving.price < sellLimitHeap[findHeapParent(cur)].price) {
                    sellLimitHeap[cur] = sellLimitHeap[findHeapParent(cur)];
                    sellHeapMap[sellLimitHeap[cur].id] = cur;
                    cur = findHeapParent(cur);
                }

                while (true) {
                    int left = 2 * cur + 1;
                    int right = 2 * cur + 2;
                    int smallest = -1;

                    if (left < sellLimitSize) {
                        smallest = left;
                        if (right < sellLimitSize && sellLimitHeap[right].price < sellLimitHeap[left].price) {
                            smallest = right;
                        }
                    }

                    if (smallest == -1 || moving.price <= sellLimitHeap[smallest].price) {
                        break;
                    }

                    sellLimitHeap[cur] = sellLimitHeap[smallest];
                    sellHeapMap[sellLimitHeap[cur].id] = cur;
                    cur = smallest;
                }

                sellLimitHeap[cur] = moving;
                sellHeapMap[moving.id] = cur;
            }

            inline
            void removeBid(int heapLoc) {
                buyLimitSize--;
                buyHeapMap[buyLimitHeap[heapLoc].id] = -1;

                if (heapLoc == buyLimitSize) {
                    buyLimitHeap[heapLoc].id = -1;
                    return;
                }

                orderStruct moving = buyLimitHeap[buyLimitSize];
                buyLimitHeap[buyLimitSize].id = -1;

                int cur = heapLoc;

                while (cur > 0 && moving.price > buyLimitHeap[findHeapParent(cur)].price) {
                    buyLimitHeap[cur] = buyLimitHeap[findHeapParent(cur)];
                    buyHeapMap[buyLimitHeap[cur].id] = cur;
                    cur = findHeapParent(cur);
                }

                while (true) {
                    int left = 2 * cur + 1;
                    int right = 2 * cur + 2;
                    int largest = -1;

                    if (left < buyLimitSize) {
                        largest = left;
                        if (right < buyLimitSize && buyLimitHeap[right].price > buyLimitHeap[left].price) {
                            largest = right;
                        }
                    }

                    if (largest == -1 || moving.price >= buyLimitHeap[largest].price) {
                        break;
                    }

                    buyLimitHeap[cur] = buyLimitHeap[largest];
                    buyHeapMap[buyLimitHeap[cur].id] = cur;
                    cur = largest;
                }

                buyLimitHeap[cur] = moving;
                buyHeapMap[moving.id] = cur;
            }

            int lastTradedPrice;
    };    
}

#endif