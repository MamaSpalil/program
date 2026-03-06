#include "TaxReporter.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace crypto {

std::vector<TaxReporter::TaxEvent> TaxReporter::calculate(
    const std::vector<HistoricalTrade>& trades,
    Method method,
    int taxYear) const
{
    std::vector<TaxEvent> events;
    // Group lots by symbol
    std::map<std::string, std::deque<TaxLot>> lots;

    for (auto& t : trades) {
        // Filter by year: entryTime is in milliseconds
        std::time_t sec = static_cast<std::time_t>(t.entryTime / 1000);
        std::tm tm_val;
#ifdef _WIN32
        gmtime_s(&tm_val, &sec);
#else
        gmtime_r(&sec, &tm_val);
#endif
        int year = 1900 + tm_val.tm_year;
        if (year != taxYear) continue;

        if (t.side == "BUY") {
            TaxLot lot;
            lot.qty = t.qty;
            lot.unitCost = t.entryPrice;
            lot.costBasis = t.entryPrice * t.qty;
            lot.acquireTime = t.entryTime;
            lots[t.symbol].push_back(lot);
        } else if (t.side == "SELL") {
            double remaining = t.qty;
            auto& q = lots[t.symbol];
            while (remaining > 1e-12 && !q.empty()) {
                auto& lot = (method == Method::FIFO) ? q.front() : q.back();
                double used = std::min(remaining, lot.qty);

                TaxEvent ev;
                ev.symbol    = t.symbol;
                ev.qty       = used;
                ev.proceeds  = used * t.exitPrice;
                ev.costBasis = used * lot.unitCost; // use per-unit cost for precision
                ev.gainLoss  = ev.proceeds - ev.costBasis;
                ev.openTime  = lot.acquireTime;
                ev.closeTime = t.exitTime;
                // Long-term = held > 365 days (times in milliseconds)
                ev.isLongTerm = (t.exitTime - lot.acquireTime)
                              > 365LL * 86400LL * 1000LL;
                events.push_back(ev);

                lot.qty       -= used;
                lot.costBasis -= ev.costBasis;
                remaining     -= used;

                if (lot.qty <= 1e-12) {
                    if (method == Method::FIFO)
                        q.pop_front();
                    else
                        q.pop_back();
                }
            }
        }
    }

    return events;
}

bool TaxReporter::exportCSV(
    const std::vector<TaxEvent>& events,
    const std::string& filename)
{
    std::ofstream f(filename);
    if (!f.is_open()) return false;

    f << "Symbol,Qty,Proceeds,CostBasis,GainLoss,OpenTime,CloseTime,LongTerm\n";
    for (auto& ev : events) {
        f << ev.symbol << ","
          << std::fixed << std::setprecision(8) << ev.qty << ","
          << std::setprecision(2) << ev.proceeds << ","
          << ev.costBasis << ","
          << ev.gainLoss << ","
          << ev.openTime << ","
          << ev.closeTime << ","
          << (ev.isLongTerm ? "Yes" : "No") << "\n";
    }
    return true;
}

} // namespace crypto
