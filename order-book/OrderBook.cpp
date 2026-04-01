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
/*
 * Three scenarios: 
 * 
 * 1) Order has already been fully processed -> cancel order fails (easy return)
 *    - Use the processedOrders data structure to tell
 * 
 * 2) Order has not been processed yet 
 *    - Run through appropriate heap and delete node 
 * 
 * 3) Order has been partially processed
 *    - Run through appropriate heap and delete node
 * 
 *
 */


string OrderBook::cancelOrder(int id) {
    // Prolly need to change .count to some other detect->exit feature (possible waste of time here)
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
        int bestShares = bestAsk.shares;
        double bestPrice = bestAsk.price;
        int addedShares = min(reqShares - filledShares, bestShares);
        filledShares += addedShares;
        totalCost += (addedShares * bestPrice);
        removeAsk(0);

        if (filledShares == reqShares) {
            if (bestShares - addedShares) {
                orderStruct adding("SELL", "LIMIT", bestShares - addedShares, bestPrice, bestAsk.id);
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

        // Partial market orders must be stored in some pending list with ids
    }

    return ("FILLED");
}


string OrderBook::sellMarketOrder(orderStruct oS) {
    int filledShares = 0;
    int reqShares = oS.shares;
    double totalGain = 0.0;

    orderStruct bestBid = getMaxBid();

    while (bestBid.id != -1) {
        int bestShares = bestBid.shares;
        double bestPrice = bestBid.price;
        int addedShares = min(reqShares - filledShares, bestShares);
        filledShares += addedShares;
        totalGain += (addedShares * bestPrice);
        removeBid(0);

        if (filledShares == reqShares) {
            if (bestShares - addedShares) {
                orderStruct adding("SELL", "LIMIT", bestShares - addedShares, bestPrice, bestBid.id);
                sellLimitInsert(adding);
            }
            break;
        }

        bestBid = getMaxBid();
    }

    string output = "SOLD " + to_string(filledShares) + " shares for a total gain of $" + to_string(totalGain);
    cout << output << endl;
    
    if (filledShares < reqShares) {
        return ("PARTIAL");

        // Partial market orders must be stored in some pending list with ids
    }

    return ("FILLED");
}

/*
 * TODO Logic: 
 * Buy limits follow specific logic flow: 
 * 
 * If there exists sell limit orders with prices below the buy limit price, 
 *   - Execute at the cheapest sell limit price (repeatedly until satisfied)
 *     - If there exist multiple cheapest sell limit prices, execute them in chronological order (better heap sorting methods required)
 *   - If all sell limits below buy price have been executed and order is still not fulfilled, store onto buyLimitHeap
 * 
 * Else,
 *   - Store buy limit order onto buyLimitHeap
 * 
 */

string OrderBook::buyLimitOrder(orderStruct oS) {
    buyLimitInsert(oS);
    cout << "BUY INSERTED" << endl;
    return ("Inserted");
}


/*
 * TODO Logic: 
 * Sell limits follow specific logic flow: 
 * 
 * If there exists buy limit orders with prices above the sell limit price, 
 *   - Execute at the most expensive buy limit price (repeatedly until satisfied)
 *     - If there exist multiple expensive buy limit prices, execute them in chronological order (better heap sorting methods required)
 *   - If all buy limits above buy price have been executed and order is still not fulfilled, store onto sellLimitHeap
 * 
 * Else,
 *   - Store sell limit order onto sellLimitHeap
 * 
 */
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
    sellLimitHeap[sellLimitSize] = oS;
    sellHeapMap[oS.id] = sellLimitSize++;

    cout << "order struct " << oS.id << " inserted in sell heap at " << sellHeapMap[oS.id] << endl;

    while (cur != 0 && sellLimitHeap[findHeapParent(cur)].price > sellLimitHeap[cur].price) {
        orderStruct pS = sellLimitHeap[findHeapParent(cur)];
        sellHeapMap[pS.id] = cur;
        sellHeapMap[oS.id] = findHeapParent(cur);
        swap(sellLimitHeap[findHeapParent(cur)], sellLimitHeap[cur]);
        cur = findHeapParent(cur);

        cout << "order struct " << pS.id << " swapped to " << sellHeapMap[pS.id] << endl;
        cout << "order struct " << oS.id << " swapped to " << sellHeapMap[oS.id] << endl;
    }
}

void OrderBook::buyLimitInsert(orderStruct oS) {
    int cur = buyLimitSize;
    buyLimitHeap[buyLimitSize] = oS;
    buyHeapMap[oS.id] = buyLimitSize++;

    cout << "order struct " << oS.id << " inserted in buy heap at " << buyHeapMap[oS.id] << endl;

    while (cur != 0 && buyLimitHeap[findHeapParent(cur)].price < buyLimitHeap[cur].price) {
        orderStruct pS = buyLimitHeap[findHeapParent(cur)];
        buyHeapMap[pS.id] = cur;
        buyHeapMap[oS.id] = findHeapParent(cur);
        swap(buyLimitHeap[findHeapParent(cur)], buyLimitHeap[cur]);
        cur = findHeapParent(cur);

        cout << "order struct " << pS.id << " swapped to " << buyHeapMap[pS.id] << endl;
        cout << "order struct " << oS.id << " swapped to " << buyHeapMap[oS.id] << endl;
    }
}

orderStruct OrderBook::getMinAsk() {
    return (sellLimitHeap[0]);
}

orderStruct OrderBook::getMaxBid() {
    return (buyLimitHeap[0]);
}

/*
 * TODO: 
 * Refactor removeMinAsk so it accepts orderId paramter
 * orderId parameter will be used for targeted node deletions 
 * beyond just the minimum/maximum heap elements. 
 *
 *
 * This extension will be needed for the cancelOrder buildout.
 *
 */

void OrderBook::removeAsk(int heapLoc) {
    sellLimitSize--;
    sellLimitHeap[heapLoc] = sellLimitHeap[sellLimitSize];
    sellLimitHeap[sellLimitSize].id = -1;

    int cur = heapLoc;

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

        swap(sellLimitHeap[cur], sellLimitHeap[smallest]);
        cur = smallest;
    }
}

/*
 * TODO: 
 * Refactor removeMaxBid so it accepts orderId paramter
 * orderId parameter will be used for targeted node deletions 
 * beyond just the minimum/maximum heap elements. 
 *
 *
 * This extension will be needed for the cancelOrder buildout.
 *
 */

void OrderBook::removeBid(int heapLoc) {
    buyLimitSize--;
    buyLimitHeap[heapLoc] = buyLimitHeap[buyLimitSize];
    buyLimitHeap[buyLimitSize].id = -1;

    int cur = heapLoc;

    while (buyLimitHeap[cur].id != -1) {
        int largest = cur;
        int left = 2 * cur + 1;
        int right = 2 * cur + 2;

        if (left <= buyLimitSize && buyLimitHeap[left].price > buyLimitHeap[largest].price) {
            largest = left;
        }

        if (right <= buyLimitSize && buyLimitHeap[right].price > buyLimitHeap[largest].price) {
            largest = right;
        }

        if (largest == cur) {
            break;
        }

        swap(buyLimitHeap[cur], buyLimitHeap[largest]);
        cur = largest;
    }
}

// Market Info Methods
// ====================================================================================================================================
double OrderBook::getSpread() {
    return (bestAsk - bestBid);
}