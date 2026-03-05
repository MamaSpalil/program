#include <gtest/gtest.h>
#include "../src/automation/Scheduler.h"
#include "../src/automation/GridBot.h"
#include "../src/integration/WebhookServer.h"
#include "../src/integration/TelegramBot.h"
#include "../src/tax/TaxReporter.h"
#include "../src/external/NewsFeed.h"
#include "../src/external/FearGreed.h"

using namespace crypto;

// ── T08: Scheduler cron parser ───────────────────────────────────────────

TEST(Scheduler, CronEveryMinute) {
    Scheduler s;
    std::tm t{};
    t.tm_min = 0; t.tm_sec = 0;
    t.tm_hour = 12; t.tm_mday = 1; t.tm_mon = 0; t.tm_wday = 1;
    EXPECT_TRUE(s.shouldRun("* * * * *", t));
}

TEST(Scheduler, CronSpecificHour) {
    Scheduler s;
    std::tm t{};
    t.tm_hour = 9; t.tm_min = 0;
    t.tm_mday = 1; t.tm_mon = 0;
    t.tm_wday = 1; // Monday
    EXPECT_TRUE(s.shouldRun("0 9 * * 1-5", t));
    t.tm_wday = 6; // Saturday
    EXPECT_FALSE(s.shouldRun("0 9 * * 1-5", t));
}

TEST(Scheduler, CronSpecificMinuteHour) {
    Scheduler s;
    std::tm t{};
    t.tm_hour = 14; t.tm_min = 30;
    t.tm_mday = 15; t.tm_mon = 5; t.tm_wday = 3;
    EXPECT_TRUE(s.shouldRun("30 14 * * *", t));
    EXPECT_FALSE(s.shouldRun("0 14 * * *", t));
}

TEST(Scheduler, AddAndRemoveJobs) {
    Scheduler s;
    ScheduledJob j;
    j.name = "Test Job";
    j.cronExpr = "* * * * *";
    j.symbol = "BTCUSDT";
    s.addJob(j);
    auto jobs = s.getJobs();
    EXPECT_EQ(jobs.size(), 1u);
    s.removeJob(jobs[0].id);
    EXPECT_TRUE(s.getJobs().empty());
}

TEST(Scheduler, SaveAndLoad) {
    const std::string path = "/tmp/test_scheduler.json";
    {
        Scheduler s;
        ScheduledJob j;
        j.name = "Morning Run";
        j.cronExpr = "0 9 * * 1-5";
        j.symbol = "ETHUSDT";
        s.addJob(j);
        s.save(path);
    }
    {
        Scheduler s;
        s.load(path);
        auto jobs = s.getJobs();
        EXPECT_EQ(jobs.size(), 1u);
        EXPECT_EQ(jobs[0].name, "Morning Run");
        EXPECT_EQ(jobs[0].symbol, "ETHUSDT");
    }
}

// ── T09: Grid Bot levels ─────────────────────────────────────────────────

TEST(GridBot, ArithmeticLevels) {
    GridBot bot;
    GridConfig cfg;
    cfg.lowerPrice = 60000;
    cfg.upperPrice = 70000;
    cfg.gridCount  = 10;
    cfg.arithmetic = true;
    bot.configure(cfg);
    EXPECT_EQ(bot.levelCount(), 11);
    EXPECT_NEAR(bot.levelAt(0),  60000, 0.01);
    EXPECT_NEAR(bot.levelAt(5),  65000, 0.01);
    EXPECT_NEAR(bot.levelAt(10), 70000, 0.01);
}

TEST(GridBot, GeometricLevels) {
    GridBot bot;
    GridConfig cfg;
    cfg.lowerPrice = 10000;
    cfg.upperPrice = 20000;
    cfg.gridCount  = 4;
    cfg.arithmetic = false;
    bot.configure(cfg);
    EXPECT_EQ(bot.levelCount(), 5);
    EXPECT_NEAR(bot.levelAt(0), 10000, 0.01);
    EXPECT_NEAR(bot.levelAt(4), 20000, 1.0);  // geometric may have rounding
}

TEST(GridBot, StartStop) {
    GridBot bot;
    GridConfig cfg;
    cfg.lowerPrice = 60000;
    cfg.upperPrice = 70000;
    cfg.gridCount = 5;
    bot.start(cfg);
    EXPECT_TRUE(bot.isRunning());
    bot.stop();
    EXPECT_FALSE(bot.isRunning());
}

// ── T10: Webhook security ────────────────────────────────────────────────

TEST(Webhook, RejectsWrongSecret) {
    WebhookServer srv;
    srv.setSecret("correct-secret");
    auto res = srv.simulateRequest(
        "wrong-secret",
        "{\"action\":\"buy\",\"symbol\":\"BTCUSDT\",\"qty\":0.01}");
    EXPECT_EQ(res.status, 401);
}

TEST(Webhook, AcceptsCorrectSecret) {
    WebhookServer srv;
    srv.setSecret("test-secret-123");
    auto res = srv.simulateRequest(
        "test-secret-123",
        "{\"action\":\"buy\",\"symbol\":\"BTCUSDT\",\"qty\":0.01}");
    EXPECT_EQ(res.status, 200);
}

TEST(Webhook, RateLimitBlocks) {
    WebhookServer srv;
    srv.setSecret("s");
    for (int i = 0; i < 10; ++i) {
        srv.simulateRequest("s", "{}", "1.2.3.4");
    }
    auto res = srv.simulateRequest("s", "{}", "1.2.3.4");
    EXPECT_EQ(res.status, 429);
}

TEST(Webhook, DifferentIPsNotAffected) {
    WebhookServer srv;
    srv.setSecret("s");
    for (int i = 0; i < 10; ++i) {
        srv.simulateRequest("s", "{}", "1.2.3.4");
    }
    // Different IP should still be allowed
    auto res = srv.simulateRequest("s", "{}", "5.6.7.8");
    EXPECT_EQ(res.status, 200);
}

// ── T11: Telegram Bot commands ───────────────────────────────────────────

TEST(TelegramBot, HelpCommand) {
    TelegramBot bot;
    bot.setAuthorizedChat("12345");
    auto reply = bot.processCommand("/help", "12345");
    EXPECT_FALSE(reply.empty());
    EXPECT_NE(reply.find("/balance"), std::string::npos);
}

TEST(TelegramBot, UnauthorizedChat) {
    TelegramBot bot;
    bot.setAuthorizedChat("12345");
    auto reply = bot.processCommand("/help", "99999");
    EXPECT_EQ(reply, "Unauthorized");
}

TEST(TelegramBot, StatusCommand) {
    TelegramBot bot;
    bot.setAuthorizedChat("12345");
    auto reply = bot.processCommand("/status", "12345");
    EXPECT_FALSE(reply.empty());
}

TEST(TelegramBot, AvailableCommands) {
    auto cmds = TelegramBot::availableCommands();
    EXPECT_GT(cmds.size(), 5u);
}

// ── T12: Tax Reporter FIFO vs LIFO ──────────────────────────────────────

static HistoricalTrade makeTaxTrade(const std::string& side,
                                     const std::string& symbol,
                                     double qty,
                                     double entryPrice,
                                     double exitPrice,
                                     int64_t entryTime,
                                     int64_t exitTime) {
    HistoricalTrade t;
    t.side = side;
    t.symbol = symbol;
    t.qty = qty;
    t.entryPrice = entryPrice;
    t.exitPrice = exitPrice;
    t.entryTime = entryTime;
    t.exitTime = exitTime;
    return t;
}

TEST(TaxReporter, FIFOGainCalc) {
    TaxReporter reporter;
    // Use timestamps in year 2024 (ms since epoch)
    // 2024-01-15 ~= 1705276800000 ms
    int64_t t1 = 1705276800000LL;  // Jan 15, 2024
    int64_t t2 = 1705363200000LL;  // Jan 16, 2024
    int64_t t3 = 1705449600000LL;  // Jan 17, 2024

    std::vector<HistoricalTrade> trades = {
        makeTaxTrade("BUY",  "BTC", 1.0, 50000, 0,     t1, 0),
        makeTaxTrade("BUY",  "BTC", 1.0, 60000, 0,     t2, 0),
        makeTaxTrade("SELL", "BTC", 1.0, 0,     70000, t3, t3)
    };

    auto events = reporter.calculate(trades, TaxReporter::Method::FIFO, 2024);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_NEAR(events[0].costBasis, 50000.0, 0.01);
    EXPECT_NEAR(events[0].gainLoss,  20000.0, 0.01);
}

TEST(TaxReporter, LIFOGainCalc) {
    TaxReporter reporter;
    int64_t t1 = 1705276800000LL;
    int64_t t2 = 1705363200000LL;
    int64_t t3 = 1705449600000LL;

    std::vector<HistoricalTrade> trades = {
        makeTaxTrade("BUY",  "BTC", 1.0, 50000, 0,     t1, 0),
        makeTaxTrade("BUY",  "BTC", 1.0, 60000, 0,     t2, 0),
        makeTaxTrade("SELL", "BTC", 1.0, 0,     70000, t3, t3)
    };

    auto events = reporter.calculate(trades, TaxReporter::Method::LIFO, 2024);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_NEAR(events[0].costBasis, 60000.0, 0.01);
    EXPECT_NEAR(events[0].gainLoss,  10000.0, 0.01);
}

TEST(TaxReporter, ExportCSV) {
    TaxReporter reporter;
    (void)reporter; // reporter instance validates construction; export is static
    TaxReporter::TaxEvent ev;
    ev.symbol = "BTC";
    ev.qty = 1.0;
    ev.proceeds = 70000.0;
    ev.costBasis = 50000.0;
    ev.gainLoss = 20000.0;

    std::string path = "/tmp/test_tax_export.csv";
    EXPECT_TRUE(TaxReporter::exportCSV({ev}, path));
}

// ── NewsFeed ─────────────────────────────────────────────────────────────

TEST(NewsFeed, AddAndGet) {
    NewsFeed feed;
    NewsItem item;
    item.title = "Bitcoin hits new high";
    item.source = "CoinDesk";
    item.sentiment = "positive";
    item.pubTime = 1705276800;
    feed.addItem(item);
    auto items = feed.getItems(10);
    EXPECT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].title, "Bitcoin hits new high");
}

TEST(NewsFeed, MaxItems) {
    NewsFeed feed;
    for (int i = 0; i < 250; ++i) {
        NewsItem item;
        item.title = "News " + std::to_string(i);
        feed.addItem(item);
    }
    EXPECT_LE(feed.itemCount(), 200);
}

// ── FearGreed ────────────────────────────────────────────────────────────

TEST(FearGreed, SetAndGet) {
    FearGreed fg;
    FearGreedData d;
    d.value = 25;
    d.classification = "Extreme Fear";
    d.timestamp = 1705276800;
    fg.setData(d);
    auto got = fg.getData();
    EXPECT_EQ(got.value, 25);
    EXPECT_EQ(got.classification, "Extreme Fear");
}
