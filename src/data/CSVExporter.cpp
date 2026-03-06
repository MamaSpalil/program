#include "CSVExporter.h"
#include <fstream>
#include <ctime>
#include <cstring>
#include <filesystem>
#include <algorithm>

namespace crypto {

// Escape a CSV field: wrap in quotes if it contains comma, quote, or newline
static std::string escapeCsvField(const std::string& field) {
    if (field.find_first_of(",\"\n\r") != std::string::npos) {
        std::string escaped;
        escaped.reserve(field.size() + 4);
        escaped += '"';
        for (char c : field) {
            if (c == '"') escaped += '"';
            escaped += c;
        }
        escaped += '"';
        return escaped;
    }
    return field;
}

// Validate filename to prevent path traversal
static bool isValidFilename(const std::string& filename) {
    // Reject empty filenames
    if (filename.empty()) return false;
    // Reject path traversal
    if (filename.find("..") != std::string::npos) return false;
    // Only allow relative filenames in the current directory
    namespace fs = std::filesystem;
    fs::path p(filename);
    if (p.is_absolute()) return false;
    return true;
}

static std::string formatTime(int64_t epochMs) {
    time_t t = static_cast<time_t>(epochMs / 1000);
    char buf[32];
    std::memset(buf, 0, sizeof(buf));
#ifdef _WIN32
    struct tm tmBuf;
    gmtime_s(&tmBuf, &t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmBuf);
#else
    struct tm tmBuf;
    gmtime_r(&t, &tmBuf);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmBuf);
#endif
    return std::string(buf);
}

bool CSVExporter::exportBars(const std::vector<Candle>& bars,
                              const std::string& filename)
{
    if (!isValidFilename(filename)) return false;
    std::ofstream f(filename);
    if (!f.is_open()) return false;
    f << "datetime,open,high,low,close,volume\n";
    for (auto& b : bars) {
        f << formatTime(b.openTime) << ","
          << b.open << "," << b.high << ","
          << b.low << "," << b.close << ","
          << b.volume << "\n";
    }
    return true;
}

bool CSVExporter::exportTrades(const std::vector<HistoricalTrade>& trades,
                                const std::string& filename)
{
    if (!isValidFilename(filename)) return false;
    std::ofstream f(filename);
    if (!f.is_open()) return false;
    f << "id,datetime_entry,datetime_exit,symbol,side,type,qty,entry_price,exit_price,pnl,commission,is_paper\n";
    for (auto& t : trades) {
        f << escapeCsvField(t.id) << ","
          << escapeCsvField(formatTime(t.entryTime)) << ","
          << escapeCsvField(formatTime(t.exitTime)) << ","
          << escapeCsvField(t.symbol) << "," << escapeCsvField(t.side) << "," << escapeCsvField(t.type) << ","
          << t.qty << "," << t.entryPrice << "," << t.exitPrice << ","
          << t.pnl << "," << t.commission << ","
          << (t.isPaper ? "paper" : "live") << "\n";
    }
    return true;
}

bool CSVExporter::exportBacktestTrades(const std::vector<BacktestTrade>& trades,
                                        const std::string& filename)
{
    if (!isValidFilename(filename)) return false;
    std::ofstream f(filename);
    if (!f.is_open()) return false;
    f << "side,entry_time,exit_time,entry_price,exit_price,qty,pnl,pnl_pct\n";
    for (auto& t : trades) {
        f << t.side << ","
          << formatTime(t.entryTime) << ","
          << formatTime(t.exitTime) << ","
          << t.entryPrice << "," << t.exitPrice << ","
          << t.qty << "," << t.pnl << "," << t.pnlPct << "\n";
    }
    return true;
}

bool CSVExporter::exportEquityCurve(const std::vector<double>& equity,
                                     const std::string& filename)
{
    if (!isValidFilename(filename)) return false;
    std::ofstream f(filename);
    if (!f.is_open()) return false;
    f << "index,equity\n";
    for (size_t i = 0; i < equity.size(); ++i)
        f << i << "," << equity[i] << "\n";
    return true;
}

} // namespace crypto
