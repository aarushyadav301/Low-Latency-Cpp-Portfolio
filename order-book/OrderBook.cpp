#include "OrderBook.hpp"
#include "Queue.hpp"

#include <string>
#include <iostream>

using namespace Order;
using namespace std;
using namespace Queue;

OrderBook::OrderBook() {}

// Public Interface
string OrderBook::processOrder(orderStruct oS) {
    if (oS.action == "CANCEL") {
        return (cancelOrder(oS.id));
    }
    else {
        if (oS.action == "BUY") {
            if (oS.type == "MARKET") {
                return (buyMarketOrder(oS));
            }
            else {
                return (buyLimitOrder(oS));
            }
        }
        else {
            if (oS.type == "MARKET") {
                return (sellMarketOrder(oS));
            }
            else {
                return (sellLimitOrder(oS));
            }
        }
    }
}

// Matching Engine Methods
// ====================================================================================================================================
string OrderBook::cancelOrder(int id) {
    if (processedOrders.count(id)) {
        return ("FAILED");
    }

    // TODO: Actually cancel order
    return (" ");
}


string OrderBook::buyMarketOrder(orderStruct oS) {
    int filledShares = 0;
    int reqShares = oS.shares;
    double totalCost = 0.0;

    orderStruct bestAsk = getMinAsk();

    while (bestAsk.id != -1) {
        cout << "BEST SHARES " << bestAsk.shares << endl;
        cout << "BEST PRICE " << bestAsk.price << endl;

        int bestShares = bestAsk.shares;
        double bestPrice = bestAsk.price;
        int addedShares = min(reqShares - filledShares, bestShares);
        filledShares += addedShares;
        totalCost += (addedShares * bestPrice);
        removeMinAsk();

        if (filledShares == reqShares) {
            if (bestShares - addedShares) {
                Queue::orderStruct adding("SELL", "LIMIT", bestShares - addedShares, bestPrice, bestAsk.id);
                sellLimitInsert(adding);
            }
            break;
        }

        bestAsk = getMinAsk();
    }

    double averageCost = totalCost / filledShares;
    string output = "FILLED " + to_string(filledShares) + " SHARES AT AN AVERAGE COST OF $" + to_string(averageCost);
    cout << output << endl;
    
    if (filledShares < reqShares) {
        return ("PARTIAL");
    }

    return ("FILLED");
}


string OrderBook::sellMarketOrder(orderStruct oS) {
    // TODO

    return (" ");
}



string OrderBook::buyLimitOrder(orderStruct oS) {
    buyLimitInsert(oS);
    cout << "BUY INSERTED" << endl;
    return ("Inserted");
}



string OrderBook::sellLimitOrder(orderStruct oS) {
    sellLimitInsert(oS);
    cout << "SELL INSERTED" << endl;
    return ("Inserted");
}



// Heap Method Implementations
// ====================================================================================================================================
int OrderBook::findHeapParent(int child) {
    return ((child - 1) / 2);
}

void OrderBook::sellLimitInsert(orderStruct oS) {
    int cur = sellLimitSize;
    sellLimitHeap[sellLimitSize++] = oS;

    while (cur != 0 && sellLimitHeap[findHeapParent(cur)].price > sellLimitHeap[cur].price) {
        swap(sellLimitHeap[findHeapParent(cur)], sellLimitHeap[cur]);
        cur = findHeapParent(cur);
    }

    cout << sellLimitHeap[cur].shares << endl;
}

void OrderBook::buyLimitInsert(orderStruct oS) {
    int cur = buyLimitSize;
    buyLimitHeap[buyLimitSize++] = oS;

    while (cur != 0 && buyLimitHeap[findHeapParent(cur)].price < buyLimitHeap[cur].price) {
        swap(buyLimitHeap[findHeapParent(cur)], buyLimitHeap[cur]);
        cur = findHeapParent(cur);
    }
}

orderStruct OrderBook::getMinAsk() {
    return (sellLimitHeap[0]);
}

orderStruct OrderBook::getMaxBid() {
    return (buyLimitHeap[0]);
}

void OrderBook::removeMinAsk() {
    sellLimitSize--;
    sellLimitHeap[0] = sellLimitHeap[sellLimitSize];
    sellLimitHeap[sellLimitSize].id = -1;

    int cur = 0;

    while (sellLimitHeap[cur].id != -1) {
        int smallest = cur;
        int left = 2 * cur + 1;
        int right = 2 * cur + 2;

        if (left <= sellLimitSize && sellLimitHeap[left].price < sellLimitHeap[smallest].price) {
            smallest = left;
        }

        if (right <= sellLimitSize && sellLimitHeap[right].price < sellLimitHeap[smallest].price) {
            smallest = right;
        }

        if (smallest == cur) {
            break;
        }

        std::swap(sellLimitHeap[cur], sellLimitHeap[smallest]);
        cur = smallest;
    }
}

void OrderBook::removeMaxBid() {
    sellLimitSize--;
    sellLimitHeap[0] = sellLimitHeap[sellLimitSize];
    sellLimitHeap[sellLimitSize].id = -1;

    int cur = 0;

    while (sellLimitHeap[cur].id != -1) {
        int largest = cur;
        int left = 2 * cur + 1;
        int right = 2 * cur + 2;

        if (left <= sellLimitSize && sellLimitHeap[left].price > sellLimitHeap[largest].price) {
            largest = left;
        }

        if (right <= sellLimitSize && sellLimitHeap[right].price > sellLimitHeap[largest].price) {
            largest = right;
        }

        if (largest == cur) {
            break;
        }

        std::swap(sellLimitHeap[cur], sellLimitHeap[largest]);
        cur = largest;
    }
}

// Market Info Methods
// ====================================================================================================================================
double OrderBook::getSpread() {
    return (bestAsk - bestBid);
}