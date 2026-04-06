#include "OrderBook.hpp"
#include "Queue.hpp"

#include <string>
#include <iostream>
#include <algorithm>

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
                buyOrders[oS.id] = true;
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
    fill(pendingOrders.begin(), pendingOrders.end(), false);
    fill(processedOrders.begin(), processedOrders.end(), false);
    fill(buyOrders.begin(), buyOrders.end(), false);

    sellLimitHeap[0].id = -1;
    buyLimitHeap[0].id = -1;
    sellLimitSize = 0;
    buyLimitSize = 0;
}

// Matching Engine Methods
// ====================================================================================================================================
string OrderBook::cancelOrder(int id) {
    // Prolly need to change .count to some other detect->exit feature (possible waste of time here)
    if (processedOrders[id]) {
        //cout << "CANCELLATION FAILED ORDER " << id << " ALREADY EXECUTED" << endl;
        return ("");
    } 

    if (buyOrders[id]) {
        removeBid(buyHeapMap[id]);
    }
    else {
        removeAsk(sellHeapMap[id]);
    }

    //cout << "SUCCESSFUL CANCELLATION OF ORDER " << id << endl;
    return ("");
}


string OrderBook::buyMarketOrder(orderStruct oS) {
    processedOrders[oS.id] = true;
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
            processedOrders[bestAsk.id] = true;
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
    processedOrders[oS.id] = true;
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
            processedOrders[bestBid.id] = true;
        }

        filledShares += addedShares;
        totalGain += (addedShares * bestPrice);
        removeBid(0);

        if (filledShares == reqShares) {
            if (bestShares - addedShares) {
                orderStruct adding(BUY, LIMIT, bestShares - addedShares, bestPrice, bestBid.id);
                buyLimitInsert(adding);
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
            processedOrders[cheapestAsk.id] = true;

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
        processedOrders[oS.id] = true;
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
            processedOrders[expensiveBid.id] = true;

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
        processedOrders[oS.id] = true;
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