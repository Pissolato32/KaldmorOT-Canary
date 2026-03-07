/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "test_pch.hpp"

#include "lib/di/container.hpp"
#include "lib/logging/in_memory_logger.hpp"
#include "lib/logging/logger.hpp"
#include "security/argon.hpp"

class Argon2Test : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		previousContainer = DI::getTestContainer();
		InMemoryLogger::install(injector);
		DI::setTestContainer(&injector);
		logger = &dynamic_cast<InMemoryLogger &>(DI::get<Logger>());
	}

	static void TearDownTestSuite() {
		DI::setTestContainer(previousContainer);
	}

	void SetUp() override {
		logger->reset();
	}

	static InMemoryLogger &testLogger() {
		return *logger;
	}

	uint32_t parseBitShift(const std::string &bitShiftStr) {
		Argon2 argon;
		return argon.parseBitShift(bitShiftStr);
	}

private:
	inline static di::extension::injector<> injector {};
	inline static di::extension::injector<>* previousContainer { nullptr };
	inline static InMemoryLogger* logger { nullptr };
};

TEST_F(Argon2Test, ParseBitShift_ValidInputs) {
	EXPECT_EQ(parseBitShift("1 << 12"), 1u << 12);
	EXPECT_EQ(parseBitShift("2 << 10"), 2u << 10);
	EXPECT_EQ(parseBitShift(" 16 << 1 "), 16u << 1);
	EXPECT_EQ(parseBitShift("1<<0"), 1u << 0);
	EXPECT_EQ(parseBitShift("  32  <<  5  "), 32u << 5);
}

TEST_F(Argon2Test, ParseBitShift_InvalidFormats) {
	EXPECT_EQ(parseBitShift(""), 0u);
	EXPECT_EQ(parseBitShift("1 < 12"), 0u);
	EXPECT_EQ(parseBitShift("1 <<"), 0u);
	EXPECT_EQ(parseBitShift("<< 12"), 0u);
	EXPECT_EQ(parseBitShift("invalid"), 0u);
	EXPECT_EQ(parseBitShift("1 << 12 << 1"), 0u);
	EXPECT_EQ(parseBitShift("1 << -1"), 0u);

	auto &logger = testLogger();
	EXPECT_FALSE(logger.logs.empty());
	EXPECT_EQ(logger.logs[0].level, "warning");
	EXPECT_TRUE(logger.logs[0].message.find("Invalid bit shift string format") != std::string::npos);
}

TEST_F(Argon2Test, ParseBitShift_OutOfBounds) {
	EXPECT_EQ(parseBitShift("1 << 32"), 0u);
	EXPECT_EQ(parseBitShift("1 << 64"), 0u);

	auto &logger = testLogger();
	EXPECT_FALSE(logger.logs.empty());
	EXPECT_EQ(logger.logs[0].level, "warning");
	EXPECT_TRUE(logger.logs[0].message.find("Shift value out of bounds") != std::string::npos);
}

TEST_F(Argon2Test, ParseBitShift_StoiException) {
	// 99999999999999999999 is larger than int max
	EXPECT_EQ(parseBitShift("99999999999999999999 << 1"), 0u);

	auto &logger = testLogger();
	EXPECT_FALSE(logger.logs.empty());
	EXPECT_EQ(logger.logs[0].level, "warning");
	EXPECT_TRUE(logger.logs[0].message.find("Error parsing bit shift string") != std::string::npos);
}
