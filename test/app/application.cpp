/*
 * Test framework
 *
 * See SmingTest library for details
 *
 */

#include <ConfigDBTest.h>
#include <LittleFS.h>
#include <modules.h>

TestConfig database("out/test-config");

void resetDatabase()
{
	database.openStoreForUpdate(0)->clear();
}

#define XX(t) extern void REGISTER_TEST(t);
TEST_MAP(XX)
#undef XX

static void registerTests()
{
#define XX(t)                                                                                                          \
	REGISTER_TEST(t);                                                                                                  \
	debug_i("Test '" #t "' registered");
	TEST_MAP(XX)
#undef XX
}

static void testsComplete()
{
#if RESTART_DELAY == 0
	System.restart();
#else
	SmingTest::runner.execute(testsComplete, RESTART_DELAY);
#endif
}

void init()
{
	Serial.setTxBufferSize(1024);
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true);

	debug_e("WELCOME to SMING! Host Tests application running.");

	registerTests();

#ifdef ARCH_HOST
	fileSetFileSystem(&IFS::Host::getFileSystem());
#else
	lfs_mount();
#endif

	SmingTest::runner.setGroupIntervalMs(TEST_GROUP_INTERVAL);
	System.onReady([]() { SmingTest::runner.execute(testsComplete); });
}
