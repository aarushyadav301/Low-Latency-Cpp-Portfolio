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
    //cout << "INCOMING ORDER " << oS.id << " " << oS.action << " " << oS.type << " " << oS.shares << " at " << oS.price << endl;

    if (oS.action == 2) {
        return (cancelOrder(oS.id));
    }
    else {
        if (oS.action == 0) {
            if (oS.type == 1) {
                return (buyMarketOrder(oS));
            }
            else {
                buyOrders.insert(oS.id);
                return (buyLimitOrder(oS));
            }
        }
        else {
            if (oS.type == 1) {
                return (sellMarketOrder(oS));
            }
            else {
                return (sellLimitOrder(oS));
            }
        }
    }
}

void OrderBook::reset() {
    pendingOrders.clear();
    processedOrders.clear();
    buyOrders.clear();

    sellHeapMap.clear();
    sellHeapMap.resize(10005);
    buyHeapMap.clear();
    buyHeapMap.resize(10005);
    
    sellLimitHeap.clear();
    sellLimitHeap.resize(10005);
    buyLimitHeap.clear();
    buyLimitHeap.resize(10005);
    sellLimitSize = 0;
    buyLimitSize = 0;
}

// Matching Engine Methods
// ====================================================================================================================================
string OrderBook::cancelOrder(int id) {
    // Prolly need to change .count to some other detect->exit feature (possible waste of time here)
    if (processedOrders.count(id)) {
        //cout << "CANCELLATION FAILED ORDER " << id << " ALREADY EXECUTED" << endl;
        return ("");
    } 

    if (buyOrders.count(id)) {
        removeBid(buyHeapMap[id]);
    }
    else {
        removeAsk(sellHeapMap[id]);
    }

    //cout << "SUCCESSFUL CANCELLATION OF ORDER " << id << endl;
    return ("");
}


string OrderBook::buyMarketOrder(orderStruct oS) {
    processedOrders[oS.id] = oS;
    int filledShares = 0;
    int reqShares = oS.shares;
    int totalCost = 0;

    orderStruct bestAsk = getMinAsk();

    while (bestAsk.id != -1) {
        int bestShares = bestAsk.shares;
        int bestPrice = bestAsk.price;
        lastTradedPrice = bestPrice;
        int addedShares = min(reqShares - filledShares, bestShares);

        if (addedShares == bestShares) {
            processedOrders[bestAsk.id] = bestAsk;
        }

        filledShares += addedShares;
        totalCost += (addedShares * bestPrice);
        removeAsk(0);

        if (filledShares == reqShares) {
            if (bestShares - addedShares) {
                orderStruct adding(SELL, LIMIT, bestShares - addedShares, bestPrice, bestAsk.id);
                sellLimitInsert(adding);
            }
            break;
        }

        bestAsk = getMinAsk();
    }

    if (filledShares == 0) {
        //cout << "Buy market order " << oS.id << " failed with 0 shares filled" << endl;
        return ("");
    }

    //double averageCost = totalCost / (100.0 * filledShares);
    //string output = "FILLED " + to_string(filledShares) + " SHARES AT AN AVERAGE COST OF $" + to_string(averageCost);
    //cout << output << endl;
    return ("");
}


string OrderBook::sellMarketOrder(orderStruct oS) {
    processedOrders[oS.id] = oS;
    int filledShares = 0;
    int reqShares = oS.shares;
    int totalGain = 0;

    orderStruct bestBid = getMaxBid();

    while (bestBid.id != -1) {
        int bestShares = bestBid.shares;
        int bestPrice = bestBid.price;
        lastTradedPrice = bestPrice;
        int addedShares = min(reqShares - filledShares, bestShares);

        if (addedShares == bestShares) {
            processedOrders[bestBid.id] = bestBid;
        }

        filledShares += addedShares;
        totalGain += (addedShares * bestPrice);
        removeBid(0);

        if (filledShares == reqShares) {
            if (bestShares - addedShares) {
                orderStruct adding(SELL, LIMIT, bestShares - addedShares, bestPrice, bestBid.id);
                sellLimitInsert(adding);
            }
            break;
        }

        bestBid = getMaxBid();
    }

    //string output = "SOLD " + to_string(filledShares) + " shares for a total gain of $" + to_string(totalGain);
    //cout << output << endl;
    return ("");
}

/*
 * TODO Logic: 
 *  
 * If there exist multiple cheapest sell limit prices, execute them in chronological order (better heap sorting methods required)
 */

string OrderBook::buyLimitOrder(orderStruct oS) {
    int filledShares = 0;
    int totalCost = 0;
    orderStruct cheapestAsk = getMinAsk();
    while (cheapestAsk.id != -1 && cheapestAsk.price <= oS.price) {
        lastTradedPrice = cheapestAsk.price;
        if (cheapestAsk.shares + filledShares <= oS.shares) {
            totalCost += (cheapestAsk.shares * cheapestAsk.price);
            filledShares += cheapestAsk.shares;
            processedOrders[cheapestAsk.id] = cheapestAsk;

            //cout << "Sell limit order " << cheapestAsk.id << " has been fully filled (" << cheapestAsk.shares << " shares at an average price of $" << cheapestAsk.price << ")" << endl;
            removeAsk(0);
            cheapestAsk = getMinAsk();
        }
        else {
            int adding = oS.shares - filledShares;
            totalCost += (adding * cheapestAsk.price);
            filledShares = oS.shares;
            
            orderStruct insertingOrder = cheapestAsk;
            insertingOrder.shares -= adding;

            //cout << "Sell limit order " << cheapestAsk.id << " has been partially filled (" << adding << " shares at an average price of $" << cheapestAsk.price << ")" << endl;
            removeAsk(0);
            sellLimitInsert(insertingOrder);
            break;
        } 
    }

    if (filledShares == oS.shares) {
        double avgCost = totalCost / (100.0 * filledShares);
        //cout << "Buy limit order " << oS.id << " has fully filled (" << filledShares << " shares at an average cost of $" << avgCost << ")" << endl;
        processedOrders[oS.id] = oS;
    }
    else if (filledShares > 0) {
        double avgCost = totalCost / (100.0 * filledShares);
        //cout << "Buy limit order " << oS.id << " has partially filled (" << filledShares << " shares at an average cost of $" << avgCost << ")" << endl;
        oS.shares -= filledShares;
        buyLimitInsert(oS);
        //cout << "Buy limit order " << oS.id << " inserted with " << oS.shares << " shares waiting to be bought at price $" << oS.price << " or better" << endl;
    }
    else {
        //cout << "Buy limit order " << oS.id << " inserted with " << oS.shares << " shares waiting to be bought at price $" << oS.price << " or better" << endl;
        buyLimitInsert(oS);
    }
    return("");
}


/*
 * TODO Logic: 
 *  
 * If there exist multiple most expensive buy limit orders, execute them in chronological order (better heap sorting methods required)
 */
string OrderBook::sellLimitOrder(orderStruct oS) {
    int filledShares = 0;
    int totalProfit = 0;
    orderStruct expensiveBid = getMaxBid();

    while (expensiveBid.id != -1 && expensiveBid.price >= oS.price) {
        lastTradedPrice = expensiveBid.price;
        if (filledShares + expensiveBid.shares <= oS.shares) {
            totalProfit += (expensiveBid.shares * expensiveBid.price);
            filledShares += expensiveBid.shares;
            processedOrders[expensiveBid.id] = expensiveBid;

            //cout << "Buy limit order " << expensiveBid.id << " has been fully filled (" << expensiveBid.shares << " shares at an average price of $" << expensiveBid.price << ")" << endl;
            removeBid(0);
            expensiveBid = getMaxBid();
        }
        else {
            int adding = oS.shares - filledShares;
            totalProfit += (adding * expensiveBid.price);
            filledShares = oS.shares;
            
            orderStruct insertingOrder = expensiveBid;
            insertingOrder.shares -= adding;

            //cout << "Buy limit order " << expensiveBid.id << " has been partially filled (" << adding << " shares at an average price of $" << expensiveBid.price << ")" << endl;
            removeBid(0);
            buyLimitInsert(insertingOrder);
            break;
        }
    }

    if (filledShares == oS.shares) {
        double avgProfit = totalProfit / (100.0 * filledShares);
        //cout << "Sell limit order " << oS.id << " has fully filled (" << filledShares << " shares at an average price of $" << avgProfit << ")" << endl;
        processedOrders[oS.id] = oS;
    }
    else if (filledShares > 0) {
        double avgProfit = totalProfit / (100.0 * filledShares);
        //cout << "Sell limit order " << oS.id << " has partially filled (" << filledShares << " shares at an average price of $" << avgProfit << ")" << endl;
        oS.shares -= filledShares;
        sellLimitInsert(oS);
        //cout << "Sell limit order " << oS.id << " inserted with " << oS.shares << " shares waiting to be sold at price $" << oS.price << " or better" << endl;
    }
    else {
        //cout << "Sell limit order " << oS.id << " inserted with " << oS.shares << " shares waiting to be sold at price $" << oS.price << " or better" << endl;
        sellLimitInsert(oS);
    }
    return ("");
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

    //cout << "order struct " << oS.id << " inserted in sell heap at " << sellHeapMap[oS.id] << endl;

    while (cur != 0 && sellLimitHeap[findHeapParent(cur)].price > sellLimitHeap[cur].price) {
        orderStruct pS = sellLimitHeap[findHeapParent(cur)];
        sellHeapMap[pS.id] = cur;
        sellHeapMap[oS.id] = findHeapParent(cur);
        swap(sellLimitHeap[findHeapParent(cur)], sellLimitHeap[cur]);
        cur = findHeapParent(cur);

        //cout << "order struct " << pS.id << " swapped to " << sellHeapMap[pS.id] << endl;
        //cout << "order struct " << oS.id << " swapped to " << sellHeapMap[oS.id] << endl;
    }
}

void OrderBook::buyLimitInsert(orderStruct oS) {
    int cur = buyLimitSize;
    buyLimitHeap[buyLimitSize] = oS;
    buyHeapMap[oS.id] = buyLimitSize++;

    //cout << "order struct " << oS.id << " inserted in buy heap at " << buyHeapMap[oS.id] << endl;

    while (cur != 0 && buyLimitHeap[findHeapParent(cur)].price < buyLimitHeap[cur].price) {
        orderStruct pS = buyLimitHeap[findHeapParent(cur)];
        buyHeapMap[pS.id] = cur;
        buyHeapMap[oS.id] = findHeapParent(cur);
        swap(buyLimitHeap[findHeapParent(cur)], buyLimitHeap[cur]);
        cur = findHeapParent(cur);

        //cout << "order struct " << pS.id << " swapped to " << buyHeapMap[pS.id] << endl;
        //cout << "order struct " << oS.id << " swapped to " << buyHeapMap[oS.id] << endl;
    }
}

orderStruct OrderBook::getMinAsk() {
    return (sellLimitHeap[0]);
}

orderStruct OrderBook::getMaxBid() {
    return (buyLimitHeap[0]);
}

void OrderBook::removeAsk(int heapLoc) {
    sellLimitSize--;

    orderStruct heapLocoS = sellLimitHeap[heapLoc];
    orderStruct sLSoS = sellLimitHeap[sellLimitSize];
    sellHeapMap[sLSoS.id] = heapLoc;
    sellHeapMap[heapLocoS.id] = -1;

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

        orderStruct curoS = sellLimitHeap[cur];
        orderStruct smallestoS = sellLimitHeap[smallest];
        sellHeapMap[curoS.id] = smallest;
        sellHeapMap[smallestoS.id] = cur;

        swap(sellLimitHeap[cur], sellLimitHeap[smallest]);
        cur = smallest;
    }
}

void OrderBook::removeBid(int heapLoc) {
    buyLimitSize--;

    orderStruct heapLocoS = buyLimitHeap[heapLoc];
    orderStruct bLSoS = buyLimitHeap[buyLimitSize];
    buyHeapMap[bLSoS.id] = heapLoc;
    buyHeapMap[heapLocoS.id] = -1;

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

        orderStruct curoS = buyLimitHeap[cur];
        orderStruct largestoS = buyLimitHeap[largest];
        buyHeapMap[largestoS.id] = cur;
        buyHeapMap[curoS.id] = largest;

        swap(buyLimitHeap[cur], buyLimitHeap[largest]);
        cur = largest;
    }
}

// Market Info Methods
// ====================================================================================================================================
int OrderBook::getSpread() {
    return (getMinAsk().price - getMaxBid().price);
}

int OrderBook::getLastTradedPrice() {
    return (lastTradedPrice);
}

int OrderBook::getSize() {
    return (buyLimitSize + sellLimitSize);
}