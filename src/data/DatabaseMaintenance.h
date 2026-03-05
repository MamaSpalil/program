#pragma once
#include "TradingDatabase.h"
#include "EquityRepository.h"
#include "OrderRepository.h"
#include "AuxRepository.h"
#include <thread>
#include <atomic>

namespace crypto {

class DatabaseMaintenance {
public:
    DatabaseMaintenance(TradingDatabase& db,
                        EquityRepository& equityRepo,
                        OrderRepository& orderRepo,
                        AuxRepository& auxRepo);
    ~DatabaseMaintenance();

    void start();
    void stop();
    void runCleanup();

private:
    void loop();

    TradingDatabase&   db_;
    EquityRepository&  equityRepo_;
    OrderRepository&   orderRepo_;
    AuxRepository&     auxRepo_;
    std::thread        thread_;
    std::atomic<bool>  running_{false};
};

} // namespace crypto
