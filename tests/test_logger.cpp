#include <gtest/gtest.h>
#include "../src/core/Logger.h"

using namespace crypto;

TEST(Logger, DoubleInitDoesNotThrow) {
    // First init
    EXPECT_NO_THROW(Logger::init());
    // Second init should be a no-op (not throw "logger already exists")
    EXPECT_NO_THROW(Logger::init());
    // Logger should be usable
    EXPECT_NE(Logger::get(), nullptr);
}

TEST(Logger, GetAutoInits) {
    auto logger = Logger::get();
    EXPECT_NE(logger, nullptr);
}

TEST(Logger, DebugModeToggle) {
    Logger::init();
    EXPECT_FALSE(Logger::isDebugMode());

    Logger::setDebugMode(true);
    EXPECT_TRUE(Logger::isDebugMode());

    Logger::setDebugMode(false);
    EXPECT_FALSE(Logger::isDebugMode());
}
