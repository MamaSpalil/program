#pragma once
#include "TradingDatabase.h"
#include "../scanner/MarketScanner.h"
#include "../tax/TaxReporter.h"
#include <string>
#include <vector>

namespace crypto {

struct FundingRateRecord {
    std::string symbol;
    std::string exchange;
    double      rate            = 0;
    long long   nextFundingTime = 0;
    long long   recordedAt      = 0;
};

struct WebhookLogEntry {
    long long   receivedAt = 0;
    std::string action;
    std::string symbol;
    double      qty        = 0;
    double      price      = 0;
    std::string status     = "ok";
    std::string errorMsg;
    std::string clientIp;
};

class AuxRepository {
public:
    explicit AuxRepository(TradingDatabase& db);

    // Funding Rate
    bool saveFundingRate(const std::string& symbol, const std::string& exchange,
                         double rate, long long nextFundingTime);
    std::vector<FundingRateRecord> getFundingHistory(const std::string& symbol,
                                                      const std::string& exchange,
                                                      int limit = 100) const;
    void pruneFundingRates(int keepDays = 30);

    // Scanner Cache
    bool upsertScanResult(const ScanResult& r, const std::string& exchange);
    std::vector<ScanResult> getScanCache(const std::string& exchange) const;
    void clearStaleScanCache(int olderThanSec = 60);

    // Webhook Log
    bool logWebhook(const WebhookLogEntry& entry);
    std::vector<WebhookLogEntry> getWebhookLog(int limit = 500) const;
    void pruneWebhookLog(int keepCount = 500);

    // Tax Events
    bool saveTaxEvents(const std::vector<TaxReporter::TaxEvent>& events,
                       int taxYear, const std::string& method);
    std::vector<TaxReporter::TaxEvent> getTaxEvents(int taxYear,
                                                      const std::string& symbol = "") const;
    void clearTaxEvents(int taxYear);

private:
    TradingDatabase& db_;
};

} // namespace crypto
