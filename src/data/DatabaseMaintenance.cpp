#include "DatabaseMaintenance.h"
#include "../core/Logger.h"

namespace crypto {

DatabaseMaintenance::DatabaseMaintenance(TradingDatabase& db,
                                         EquityRepository& equityRepo,
                                         OrderRepository& orderRepo,
                                         AuxRepository& auxRepo)
    : db_(db), equityRepo_(equityRepo), orderRepo_(orderRepo), auxRepo_(auxRepo) {}

DatabaseMaintenance::~DatabaseMaintenance() {
    stop();
}

void DatabaseMaintenance::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&DatabaseMaintenance::loop, this);
}

void DatabaseMaintenance::stop() {
    running_ = false;
    if (thread_.joinable())
        thread_.join();
}

void DatabaseMaintenance::runCleanup() {
    Logger::get()->info("[DB] Running maintenance...");

    equityRepo_.pruneOld(365);
    auxRepo_.pruneFundingRates(30);
    auxRepo_.pruneWebhookLog(500);
    orderRepo_.deleteOld(30);
    db_.execute("PRAGMA wal_checkpoint(TRUNCATE)");

    Logger::get()->info("[DB] Maintenance complete");
}

void DatabaseMaintenance::loop() {
    while (running_) {
        // Sleep in small intervals so stop() is responsive
        for (int i = 0; i < 86400 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (running_) {
            runCleanup();
        }
    }
}

} // namespace crypto
