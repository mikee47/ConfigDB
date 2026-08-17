#include <SmingCore.h>
#include "jsonrpc.h"

#ifdef ENABLE_MALLOC_COUNT
#include <malloc_count.h>
#endif

namespace
{
template <typename Object> void printMessage(const Object& object)
{
	Serial << object << _F("\r\n");
}

bool generateRawColorRequest(Jsonrpc& db, int id)
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto request = update.toColor();
			request.setId(id);
			request.setMethod("color");

			auto raw = request.params.toRawColor();
			raw.raw.setR(1023);
			raw.raw.setG(512);
			raw.raw.setB(128);
			raw.raw.setWw(64);
			raw.raw.setCw(32);
			printMessage(request);
			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
}

bool generateHsvColorRequest(Jsonrpc& db, int id)
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto request = update.toColor();
			request.setId(id);
			request.setMethod("color");

			auto hsv = request.params.toHsvColor();
			hsv.hsv.setH(210);
			hsv.hsv.setS(75);
			hsv.hsv.setV(60);
			hsv.hsv.setCt(350);
			printMessage(request);
			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
}

bool generateInfoV1Message(Jsonrpc& db, int id)
{
	int free=system_get_free_heap_size();	
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto message = update.toInfov1();
			message.setId(id);
			message.setMethod("infov1");

			auto& result = message.params;
			result.setDeviceid(10964360);
			result.setSoc("esp8266");
			result.setCurrentRom("rom0");
			result.setGitVersion("V5.0.0-965-experimental");
			result.setBuildType("debug");
			result.setGitDate("2026-08-06");
			result.setWebappVersion("V5.0-365-experimental");
			result.setSming("6.2.0");
			result.setEventNumClients(1);
			result.setUptime(358620);
			result.setHeapFree(16912);
			result.rgbww.setVersion("0.10.0");
			result.rgbww.setQueuesize(20);
			result.connection.setConnected(true);
			result.connection.setSsid("IoT");
			result.connection.setDhcp(true);
			result.connection.setIp("192.168.29.101");
			result.connection.setNetmask("255.255.255.0");
			result.connection.setGateway("192.168.29.1");
			result.connection.setMac("840d8ea74d88");
			Serial << "heap used: " << (free-system_get_free_heap_size()) << " bytes" << endl;
			printMessage(message);
			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
}

bool generateInfoV2Message(Jsonrpc& db, int id)
{
	int free=system_get_free_heap_size();
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto message = update.toInfov2();
			message.setId(id);
			message.setMethod("infov2");

			auto& result = message.params;
			result.device.setDeviceid(10964360);
			result.device.setSoc("esp8266");
			result.device.setCurrentRom("rom0");
			result.app.setWebappVersion("V5.0-365-experimental");
			result.app.setGitVersion("V5.0.0-965-experimental");
			result.app.setBuildType("debug");
			result.app.setGitDate("2026-08-06");
			result.sming.setVersion("6.2.0");
			result.filesystem.setTotalBytes(1015808);
			result.filesystem.setFreeBytes(516096);
			result.filesystem.setUsedBytes(499712);
			result.rgbww.setVersion("0.10.0");
			result.rgbww.setQueuesize(20);
			result.connection.setConnected(true);
			result.connection.setSsid("IoT");
			result.connection.setDhcp(true);
			result.connection.setIp("192.168.29.101");
			result.connection.setNetmask("255.255.255.0");
			result.connection.setGateway("192.168.29.1");
			result.connection.setMac("840d8ea74d88");
			result.connection.setRssi(-63);
			result.mqtt.setStatus("disabled");
			result.mqtt.setEnabled(false);
			result.mqtt.setBroker("mqtt.local");
			result.mqtt.setTopic("home/");
			result.homeassistant.setEnabled(true);
			result.homeassistant.setDiscoveryPrefix("homeassistant");
			result.homeassistant.setNodeID("");
			result.runtime.setUptime(941760);
			result.runtime.setHeapFree(system_get_free_heap_size());
			result.runtime.setMinimumfreeHeapRuntime(5528);
			result.runtime.setMinimumfreeHeap10min(19216);
			result.runtime.setHeapLowErrUptime(0);
			result.runtime.setHeapLowErr10min(0);
			Serial << "heap used: " << (free-system_get_free_heap_size()) << " bytes" << endl;

			printMessage(message);
			
			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
}

bool generateInfoMessage(Jsonrpc& db, int version, int id)
{
	if(version == 1) {
		return generateInfoV1Message(db, id);
	}
	if(version == 2) {
		return generateInfoV2Message(db, id);
	}
	return false;
}

} // namespace

void init()
{
	Serial.begin(COM_SPEED_SERIAL);
	Serial.systemDebugOutput(true);

#ifdef ARCH_HOST
	fileSetFileSystem(&IFS::Host::getFileSystem());
#else
	spiffs_mount();
#endif

	Jsonrpc db("jsonrpc");

	Serial << endl << "RAW COLOR REQUEST" << endl;
	if(!generateRawColorRequest(db, 1)) {
		Serial << "Failed to update color database" << endl;
	}

	Serial << endl << "HSV COLOR REQUEST" << endl;
	if(!generateHsvColorRequest(db, 2)) {
		Serial << "Failed to update color database" << endl;
	}

	Serial << endl << "INFO V1 MESSAGE" << endl;
	if(!generateInfoMessage(db, 1, 3)) {
		Serial << "Failed to generate info v1 message" << endl;
	}

	Serial << endl << "INFO V2 MESSAGE" << endl;
	if(!generateInfoMessage(db, 2, 4)) {
		Serial << "Failed to generate info v2 message" << endl;
	}

	Serial << endl << endl;
}
