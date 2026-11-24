/*
 * TradeSim India - Virtual Stock Market Simulator
 * Intermediate C project - live-style simulated market
 *
 * IMPORTANT:
 * - Prices are RANDOMLY SIMULATED, not live NSE prices.
 * - All trades use virtual money. No real orders are placed.
 * - The Live Market screen updates every second for 20 seconds.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static void waitOneSecond(void) { Sleep(1000); }
static void clearScreen(void) { system("cls"); }
#else
#include <unistd.h>
static void waitOneSecond(void) { sleep(1); }
static void clearScreen(void) { system("clear"); }
#endif

#define MAX_STOCKS 8
#define MAX_TRANSACTIONS 300
#define STARTING_BALANCE 100000.0
#define LIVE_TICKS 20
#define BROKERAGE_RATE 0.001  /* 0.10% simulated brokerage */

typedef struct {
    char company[50];
    char symbol[16];
    double price;
    double previousClose;
} Stock;

typedef struct {
    int quantity;
    double averageBuyPrice;
} Holding;

typedef struct {
    char type[8];
    char symbol[16];
    int quantity;
    double price;
    double total;
    char timestamp[24];
} Transaction;

static Stock stocks[MAX_STOCKS] = {
    {"Reliance Industries", "RELIANCE", 1250.00, 1220.00},
    {"Tata Consultancy Services", "TCS", 3800.00, 3850.00},
    {"Infosys", "INFY", 1600.00, 1570.00},
    {"HDFC Bank", "HDFCBANK", 1700.00, 1680.00},
    {"ICICI Bank", "ICICIBANK", 1200.00, 1190.00},
    {"ITC Limited", "ITC", 450.00, 460.00},
    {"Larsen & Toubro", "LT", 3500.00, 3460.00},
    {"State Bank of India", "SBIN", 800.00, 790.00}
};

static Holding holdings[MAX_STOCKS] = {0};
static Transaction transactions[MAX_TRANSACTIONS];
static int transactionCount = 0;
static double cashBalance = STARTING_BALANCE;
static double realizedProfit = 0.0;

static void printLine(void) {
    printf("-------------------------------------------------------------------------------\n");
}

static void waitForEnter(void) {
    char line[8];
    printf("\nPress ENTER to continue...");
    (void)fgets(line, sizeof(line), stdin);
}

static int readInt(const char *prompt, int minValue, int maxValue) {
    char line[100], *end;
    long value;

    for (;;) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) return minValue - 1;
        value = strtol(line, &end, 10);
        while (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r') end++;
        if (end != line && *end == '\0' && value >= minValue && value <= maxValue)
            return (int)value;
        printf("Invalid input. Enter a number from %d to %d.\n", minValue, maxValue);
    }
}

static double portfolioMarketValue(void) {
    int i;
    double value = 0.0;
    for (i = 0; i < MAX_STOCKS; i++)
        value += holdings[i].quantity * stocks[i].price;
    return value;
}

static double investedValue(void) {
    int i;
    double value = 0.0;
    for (i = 0; i < MAX_STOCKS; i++)
        value += holdings[i].quantity * holdings[i].averageBuyPrice;
    return value;
}

static double unrealizedProfit(void) {
    return portfolioMarketValue() - investedValue();
}

static void displayHeader(void) {
    printf("\n");
    printLine();
    printf("                              TRADESIM INDIA\n");
    printf("                         VIRTUAL STOCK EXCHANGE\n");
    printLine();
    printf("  MARKET MODE: SIMULATED LIVE FEED (NOT REAL NSE DATA)\n");
    printf("  Virtual Cash: Rs. %12.2f | Total P/L: Rs. %+10.2f\n",
           cashBalance, realizedProfit + unrealizedProfit());
    printLine();
}

static void displayMarketWatch(void) {
    int i;
    printf("\n                                  MARKET WATCH\n");
    printLine();
    printf("%-4s %-23s %-12s %12s %12s %10s\n",
           "No.", "COMPANY", "SYMBOL", "PRICE (Rs.)", "PREV CLOSE", "CHANGE");
    printLine();

    for (i = 0; i < MAX_STOCKS; i++) {
        double change = stocks[i].price - stocks[i].previousClose;
        double percent = stocks[i].previousClose > 0
                       ? (change / stocks[i].previousClose) * 100.0 : 0.0;
        printf("%-4d %-23.23s %-12s %12.2f %12.2f %+9.2f%%\n",
               i + 1, stocks[i].company, stocks[i].symbol,
               stocks[i].price, stocks[i].previousClose, percent);
    }
    printLine();
}

static int findStockBySymbol(const char *symbol) {
    int i;
    for (i = 0; i < MAX_STOCKS; i++)
        if (strcmp(stocks[i].symbol, symbol) == 0) return i;
    return -1;
}

static int selectStock(void) {
    char symbol[64];
    int i;

    displayMarketWatch();
    printf("Enter stock symbol (example: TCS): ");
    if (!fgets(symbol, sizeof(symbol), stdin)) return -1;
    symbol[strcspn(symbol, "\r\n")] = '\0';

    for (i = 0; symbol[i]; i++)
        if (symbol[i] >= 'a' && symbol[i] <= 'z')
            symbol[i] = (char)(symbol[i] - 'a' + 'A');

    i = findStockBySymbol(symbol);
    if (i == -1) printf("Stock symbol not found.\n");
    return i;
}

/* Random walk: each stock changes by roughly -0.30% to +0.30% per tick. */
static void updateStockPrices(void) {
    int i;
    for (i = 0; i < MAX_STOCKS; i++) {
        int randomStep = (rand() % 601) - 300; /* -300 to +300 */
        double percentChange = randomStep / 100000.0;
        stocks[i].price *= (1.0 + percentChange);
        if (stocks[i].price < 1.0) stocks[i].price = 1.0;
    }
}

static void addTransaction(const char *type, int stockIndex, int quantity,
                           double price, double total) {
    Transaction *t;
    time_t now;
    struct tm *tmInfo;

    if (transactionCount >= MAX_TRANSACTIONS) {
        printf("Transaction history is full; trade completed but not recorded.\n");
        return;
    }

    t = &transactions[transactionCount++];
    snprintf(t->type, sizeof(t->type), "%s", type);
    snprintf(t->symbol, sizeof(t->symbol), "%s", stocks[stockIndex].symbol);
    t->quantity = quantity;
    t->price = price;
    t->total = total;

    now = time(NULL);
    tmInfo = localtime(&now);
    if (tmInfo)
        strftime(t->timestamp, sizeof(t->timestamp), "%d-%m-%Y %H:%M:%S", tmInfo);
    else
        snprintf(t->timestamp, sizeof(t->timestamp), "Time unavailable");
}

static void liveMarketScreen(void) {
    int tick;
    printf("\nStarting simulated live market. Prices refresh every second.\n");
    printf("This is generated demonstration data, NOT live NSE prices.\n");
    waitOneSecond();

    for (tick = 1; tick <= LIVE_TICKS; tick++) {
        updateStockPrices();
        clearScreen();
        displayHeader();
        printf("                 LIVE MARKET + PORTFOLIO P/L  |  UPDATE %02d/%02d\n",
               tick, LIVE_TICKS);
        displayMarketWatch();

        printf("\nPORTFOLIO SNAPSHOT\n");
        printLine();
        printf("Cash balance       : Rs. %12.2f\n", cashBalance);
        printf("Holdings value     : Rs. %12.2f\n", portfolioMarketValue());
        printf("Unrealized P/L     : Rs. %+12.2f\n", unrealizedProfit());
        printf("Realized P/L       : Rs. %+12.2f\n", realizedProfit);
        printf("TOTAL P/L          : Rs. %+12.2f\n",
               realizedProfit + unrealizedProfit());
        printf("Total account value: Rs. %12.2f\n",
               cashBalance + portfolioMarketValue());
        printLine();
        printf("Refreshing in 1 second... (screen ends after %d updates)\n",
               LIVE_TICKS);
        fflush(stdout);
        waitOneSecond();
    }

    printf("\nLive simulation finished. Returning to main menu.\n");
}

static void buyStock(void) {
    int index, quantity;
    double gross, fee, total;
    char prompt[100];

    printf("\n                                   BUY STOCKS\n");
    index = selectStock();
    if (index < 0) return;

    snprintf(prompt, sizeof(prompt), "Enter quantity of %s to buy: ",
             stocks[index].symbol);
    quantity = readInt(prompt, 1, 1000000);
    if (quantity < 1) return;

    gross = stocks[index].price * quantity;
    fee = gross * BROKERAGE_RATE;
    total = gross + fee;

    if (total > cashBalance) {
        printf("\nOrder rejected: insufficient virtual cash.\n");
        printf("Required incl. simulated fee: Rs. %.2f | Available: Rs. %.2f\n",
               total, cashBalance);
        return;
    }

    if (holdings[index].quantity == 0) {
        holdings[index].averageBuyPrice = total / quantity;
    } else {
        double oldCost = holdings[index].averageBuyPrice * holdings[index].quantity;
        holdings[index].averageBuyPrice =
            (oldCost + total) / (holdings[index].quantity + quantity);
    }

    holdings[index].quantity += quantity;
    cashBalance -= total;
    addTransaction("BUY", index, quantity, stocks[index].price, total);

    printf("\nBUY ORDER SUCCESSFUL!\n");
    printf("%d shares of %s bought at Rs. %.2f each.\n",
           quantity, stocks[index].symbol, stocks[index].price);
    printf("Simulated brokerage (0.10%%): Rs. %.2f\n", fee);
    printf("Total debit: Rs. %.2f\n", total);
    printf("Remaining cash: Rs. %.2f\n", cashBalance);
}

static void sellStock(void) {
    int index, quantity;
    double gross, fee, proceeds, costValue, realized;

    printf("\n                                   SELL STOCKS\n");
    index = selectStock();
    if (index < 0) return;

    if (holdings[index].quantity == 0) {
        printf("You do not own any shares of %s.\n", stocks[index].symbol);
        return;
    }

    printf("You own %d shares of %s.\n",
           holdings[index].quantity, stocks[index].symbol);
    quantity = readInt("Enter quantity to sell: ", 1, holdings[index].quantity);
    if (quantity < 1) return;

    gross = stocks[index].price * quantity;
    fee = gross * BROKERAGE_RATE;
    proceeds = gross - fee;
    costValue = holdings[index].averageBuyPrice * quantity;
    realized = proceeds - costValue;

    holdings[index].quantity -= quantity;
    cashBalance += proceeds;
    realizedProfit += realized;
    if (holdings[index].quantity == 0)
        holdings[index].averageBuyPrice = 0.0;

    addTransaction("SELL", index, quantity, stocks[index].price, proceeds);

    printf("\nSELL ORDER SUCCESSFUL!\n");
    printf("Sale value before fee: Rs. %.2f\n", gross);
    printf("Simulated brokerage (0.10%%): Rs. %.2f\n", fee);
    printf("Net proceeds: Rs. %.2f\n", proceeds);
    printf("Realized P/L on this sale: Rs. %+.2f\n", realized);
    printf("Updated cash balance: Rs. %.2f\n", cashBalance);
}

static void displayPortfolio(void) {
    int i, found = 0;
    printf("\n                                   MY PORTFOLIO\n");
    printLine();
    printf("%-12s %8s %13s %13s %13s\n",
           "SYMBOL", "QTY", "AVG COST", "LTP", "UNREAL. P/L");
    printLine();

    for (i = 0; i < MAX_STOCKS; i++) {
        if (holdings[i].quantity > 0) {
            double cost = holdings[i].averageBuyPrice * holdings[i].quantity;
            double value = stocks[i].price * holdings[i].quantity;
            double pnl = value - cost;
            printf("%-12s %8d %13.2f %13.2f %+13.2f\n",
                   stocks[i].symbol, holdings[i].quantity,
                   holdings[i].averageBuyPrice, stocks[i].price, pnl);
            found = 1;
        }
    }

    if (!found) printf("No shares in portfolio yet.\n");
    printLine();
    printf("Cash balance       : Rs. %12.2f\n", cashBalance);
    printf("Holdings market val: Rs. %12.2f\n", portfolioMarketValue());
    printf("Unrealized P/L     : Rs. %+12.2f\n", unrealizedProfit());
    printf("Realized P/L       : Rs. %+12.2f\n", realizedProfit);
    printf("TOTAL P/L           : Rs. %+12.2f\n",
           realizedProfit + unrealizedProfit());
    printf("Total account value: Rs. %12.2f\n",
           cashBalance + portfolioMarketValue());
    printLine();
}

static void displayTransactions(void) {
    int i;
    printf("\n                              TRANSACTION HISTORY\n");
    printLine();
    if (transactionCount == 0) {
        printf("No transactions recorded yet.\n");
        printLine();
        return;
    }

    printf("%-5s %-6s %-12s %7s %12s %13s\n",
           "No.", "TYPE", "SYMBOL", "QTY", "PRICE", "TOTAL");
    printLine();

    for (i = 0; i < transactionCount; i++) {
        Transaction *t = &transactions[i];
        printf("%-5d %-6s %-12s %7d %12.2f %13.2f\n",
               i + 1, t->type, t->symbol, t->quantity, t->price, t->total);
        printf("      Timestamp: %s\n", t->timestamp);
    }
    printLine();
}

static void searchStock(void) {
    int index = selectStock();
    if (index < 0) return;

    printf("\n                                  STOCK DETAILS\n");
    printLine();
    printf("Company        : %s\n", stocks[index].company);
    printf("Symbol         : %s\n", stocks[index].symbol);
    printf("Current price  : Rs. %.2f\n", stocks[index].price);
    printf("Previous close : Rs. %.2f\n", stocks[index].previousClose);
    printf("Day change     : Rs. %+.2f\n",
           stocks[index].price - stocks[index].previousClose);
    printf("Your quantity  : %d\n", holdings[index].quantity);
    printLine();
}

static void resetAccount(void) {
    int confirm = readInt("Reset virtual account? Enter 1 for YES, 0 for NO: ", 0, 1);
    if (confirm == 1) {
        int i;
        for (i = 0; i < MAX_STOCKS; i++) {
            holdings[i].quantity = 0;
            holdings[i].averageBuyPrice = 0.0;
            stocks[i].price = stocks[i].previousClose;
        }
        cashBalance = STARTING_BALANCE;
        realizedProfit = 0.0;
        transactionCount = 0;
        printf("Virtual account and simulated prices have been reset.\n");
    } else {
        printf("Reset cancelled.\n");
    }
}

static void displayMenu(void) {
    printf("\n                                   MAIN MENU\n");
    printLine();
    printf("  1. Market Watch\n");
    printf("  2. Buy Stocks\n");
    printf("  3. Sell Stocks\n");
    printf("  4. My Portfolio / P&L\n");
    printf("  5. Transaction History\n");
    printf("  6. Search Stock / Stock Details\n");
    printf("  7. Reset Virtual Account\n");
    printf("  8. Live Market (auto-refresh every second)\n");
    printf("  0. Exit\n");
    printLine();
}

int main(void) {
    int choice;
    srand((unsigned int)time(NULL));

    printf("\nWelcome to TradeSim India!\n");
    printf("Paper-trading project: all prices are simulated; no real money is used.\n");

    for (;;) {
        displayHeader();
        displayMenu();
        choice = readInt("Enter your choice: ", 0, 8);

        switch (choice) {
            case 1: displayMarketWatch(); break;
            case 2: buyStock(); break;
            case 3: sellStock(); break;
            case 4: displayPortfolio(); break;
            case 5: displayTransactions(); break;
            case 6: searchStock(); break;
            case 7: resetAccount(); break;
            case 8: liveMarketScreen(); break;
            case 0:
                printf("\nThank you for using TradeSim India. Happy learning!\n");
                return 0;
        }
        waitForEnter();
    }
}
