#include "Dashboard.h"
#include "../core/Logger.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef USE_NCURSES
#include <ncurses.h>
#endif

namespace crypto {

Dashboard::Dashboard() = default;

Dashboard::~Dashboard() {
    stop();
}

void Dashboard::start() {
    running_ = true;
    renderThread_ = std::thread(&Dashboard::renderLoop, this);
}

void Dashboard::stop() {
    running_ = false;
    if (renderThread_.joinable()) renderThread_.join();
    cleanupCurses();
}

void Dashboard::update(const Candle& c,
                        const IndicatorEngine& ind,
                        const RiskManager& risk,
                        const Signal& sig) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    lastCandle_ = c;
    ema_fast_   = ind.emaFast();
    ema_slow_   = ind.emaSlow();
    rsi_        = ind.rsi();
    atr_        = ind.atr();
    equity_     = risk.equity();
    lastSignal_ = sig;
}

void Dashboard::initCurses() {
#ifdef USE_NCURSES
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN,  COLOR_BLACK);
        init_pair(2, COLOR_RED,    COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_CYAN,   COLOR_BLACK);
    }
#endif
}

void Dashboard::cleanupCurses() {
#ifdef USE_NCURSES
    endwin();
#endif
}

void Dashboard::draw() {
#ifdef USE_NCURSES
    std::lock_guard<std::mutex> lock(dataMutex_);
    clear();

    int row = 0;
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(row++, 0, "╔══════════════════════════════════════════╗");
    mvprintw(row++, 0, "║         Crypto ML Trader Dashboard       ║");
    mvprintw(row++, 0, "╚══════════════════════════════════════════╝");
    attroff(A_BOLD | COLOR_PAIR(4));

    mvprintw(row++, 0, "Price:   %.4f  Vol: %.2f", lastCandle_.close, lastCandle_.volume);
    mvprintw(row++, 0, "EMA9:    %.4f  EMA21: %.4f", ema_fast_, ema_slow_);
    mvprintw(row++, 0, "RSI:     %.2f   ATR: %.4f", rsi_, atr_);

    // Equity
    attron(COLOR_PAIR(3));
    mvprintw(row++, 0, "Equity:  $%.2f", equity_);
    attroff(COLOR_PAIR(3));

    // Signal
    int colorPair = 1;
    const char* sigStr = "HOLD";
    if (lastSignal_.type == Signal::Type::BUY)  { sigStr = "BUY";  colorPair = 1; }
    if (lastSignal_.type == Signal::Type::SELL) { sigStr = "SELL"; colorPair = 2; }
    attron(COLOR_PAIR(colorPair) | A_BOLD);
    mvprintw(row++, 0, "Signal:  %-6s  Conf: %.2f%%", sigStr, lastSignal_.confidence * 100.0);
    attroff(COLOR_PAIR(colorPair) | A_BOLD);

    if (!lastSignal_.reason.empty()) {
        mvprintw(row++, 0, "Reason:  %s", lastSignal_.reason.c_str());
    }

    mvprintw(row++, 0, "SL: %.4f  TP: %.4f", lastSignal_.stopLoss, lastSignal_.takeProfit);
    mvprintw(row++, 0, "Press Ctrl+C to exit.");
    refresh();
#else
    // Fallback: plain text
    std::lock_guard<std::mutex> lock(dataMutex_);
    std::ostringstream oss;
    oss << "\r[Price=" << std::fixed << std::setprecision(2) << lastCandle_.close
        << " RSI=" << std::setprecision(1) << rsi_
        << " EMA9=" << std::setprecision(2) << ema_fast_
        << " Equity=" << equity_
        << " Sig=";
    switch (lastSignal_.type) {
        case Signal::Type::BUY:  oss << "BUY"; break;
        case Signal::Type::SELL: oss << "SELL"; break;
        default:                 oss << "HOLD"; break;
    }
    oss << " Conf=" << std::setprecision(1) << lastSignal_.confidence * 100.0 << "%]   ";
    std::cout << oss.str() << std::flush;
#endif
}

void Dashboard::renderLoop() {
    initCurses();
    while (running_) {
        draw();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace crypto
