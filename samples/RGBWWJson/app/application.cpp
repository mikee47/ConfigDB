#include <SmingCore.h>
#include "jsonrpc.h"
#include <ConfigDB/JsonRPC/JsonRPC.h>

#ifdef ENABLE_MALLOC_COUNT
#include <malloc_count.h>
#endif

enum ErrorType {
	ParseError = -32700,
	InvalidRequest = -32600,
	MethodNotFound = -32601,
	InvalidParams = -32602,
	InternalError = -32603,
	ApplicationError1 = -32000,
	ApplicationError2 = -32001,
	ApplicationError3 = -32002,
};

namespace
{
using JsonRPC::Message;

Jsonrpc database("jsonrpc");

void printMessage(const Message& msg, const ConfigDB::ObjectRef& body)
{
	JsonRPC::exportMessage(msg, body.object, Serial);
	Serial << endl;
}

Message generateRawColorRequest(int id)
{
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		// make this a raw color request
		auto colorObject = update.toColor();
		auto req = colorObject.params.toRaw();
		req.setR(1023);
		req.setG(512);
		req.setB(128);
		req.setWw(64);
		req.setCw(32);

		root.clearDirty();

		return {id, Message::Kind::request, root.getTagString()};
	}

	return {};
}

Message generateRawColorResponse(int id)
{
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		// make this a color response
		auto colorObject = update.toColor();
		auto rsp = colorObject.result.toRaw();
		rsp.setR(1023);
		rsp.setG(512);
		rsp.setB(128);
		rsp.setWw(64);
		rsp.setCw(32);

		root.clearDirty();

		return {id, Message::Kind::result, root.getTagString()};
	}

	return {};
}

Message generateRawColorNotification(int)
{
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		auto colorObject = update.toColor();
		auto raw = colorObject.params.toRaw(); // make this a color notification
		raw.setR(1023);
		raw.setG(512);
		raw.setB(128);
		raw.setWw(64);
		raw.setCw(32);

		root.clearDirty();

		return {0, Message::Kind::notification, root.getTagString()};
	}

	return {};
}

Message generateHsvColorRequest(int id)
{
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		// make this a color request
		auto colorObject = update.toColor();
		auto hsv = colorObject.params.toHsv();
		hsv.setH(210);
		hsv.setS(75);
		hsv.setV(60);
		hsv.setCt(3500);

		root.clearDirty();

		return {id, Message::Kind::request, root.getTagString()};
	}

	return {};
}

Message generateHsvColorResponse(int id)
{
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		// make this a color response
		auto colorObject = update.toColor();
		auto hsv = colorObject.result.toHsv();
		hsv.setH(210);
		hsv.setS(75);
		hsv.setV(60);
		hsv.setCt(3500);

		root.clearDirty();

		return {id, Message::Kind::result, root.getTagString()};
	}

	return {};
}

Message generateHsvColorNotification(int)
{
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		// make this a color notification
		auto colorObject = update.toColor();
		auto hsv = colorObject.params.toHsv();
		hsv.setH(210);
		hsv.setS(75);
		hsv.setV(60);
		hsv.setCt(3500);

		root.clearDirty();

		return {0, Message::Kind::notification, root.getTagString()};
	}

	return {};
}

Message generateInfoRequest(int id)
{
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		update.toInfo();

		root.clearDirty();

		return {id, Message::Kind::request, root.getTagString()};
	}

	return {};
}

Message generateInfoV1Response(int id)
{
	int free = system_get_free_heap_size();

	Serial << "heap free before generating info v1 response: " << free << " bytes" << endl;
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		auto infoObject = update.toInfo();
		auto info = infoObject.result.toInfoV1Params();
		info.setDeviceid(10964360);
		info.setSoc("esp8266");
		info.setCurrentRom("rom0");
		info.setGitVersion("V5.0.0-965-experimental");
		info.setBuildType("debug");
		info.setGitDate("2026-08-06");
		info.setWebappVersion("V5.0-365-experimental");
		info.setSming("6.2.0");
		info.setEventNumClients(1);
		info.setUptime(358620);
		info.setHeapFree(free);
		info.rgbww.setVersion("0.10.0");
		info.rgbww.setQueuesize(20);
		info.connection.setConnected(true);
		info.connection.setSsid("IoT");
		info.connection.setDhcp(true);
		info.connection.setIp("192.168.29.101");
		info.connection.setNetmask("255.255.255.0");
		info.connection.setGateway("192.168.29.1");
		info.connection.setMac("840d8ea74d88");
		Serial << "heap used: " << (free - system_get_free_heap_size()) << " bytes" << endl;

		root.clearDirty();

		return {id, Message::Kind::result, root.getTagString()};
	}

	return {};
}

Message generateInfoV1Notification(int)
{
	int free = system_get_free_heap_size();
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		auto infoObject = update.toInfo();
		auto info = infoObject.params.toInfoV1Params();
		info.setDeviceid(10964360);
		info.setSoc("esp8266");
		info.setCurrentRom("rom0");
		info.setGitVersion("V5.0.0-965-experimental");
		info.setBuildType("debug");
		info.setGitDate("2026-08-06");
		info.setWebappVersion("V5.0-365-experimental");
		info.setSming("6.2.0");
		info.setEventNumClients(1);
		info.setUptime(358620);
		info.setHeapFree(16912);
		info.rgbww.setVersion("0.10.0");
		info.rgbww.setQueuesize(20);
		info.connection.setConnected(true);
		info.connection.setSsid("IoT");
		info.connection.setDhcp(true);
		info.connection.setIp("192.168.29.101");
		info.connection.setNetmask("255.255.255.0");
		info.connection.setGateway("192.168.29.1");
		info.connection.setMac("840d8ea74d88");
		Serial << "heap used: " << (free - system_get_free_heap_size()) << " bytes" << endl;

		root.clearDirty();

		return {0, Message::Kind::notification, root.getTagString()};
	}

	return {};
}

Message generateInfoV2Response(int id)
{
	int free = system_get_free_heap_size();

	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		auto infoObject = update.toInfo();
		auto info = infoObject.result.toInfoV2Params();
		info.device.setDeviceid(10964360);
		info.device.setSoc("esp8266");
		info.device.setCurrentRom("rom0");
		info.app.setWebappVersion("V5.0-365-experimental");
		info.app.setGitVersion("V5.0.0-965-experimental");
		info.app.setBuildType("debug");
		info.app.setGitDate("2026-08-06");
		info.sming.setVersion("6.2.0");
		info.filesystem.setTotalBytes(1015808);
		info.filesystem.setFreeBytes(516096);
		info.filesystem.setUsedBytes(499712);
		info.rgbww.setVersion("0.10.0");
		info.rgbww.setQueuesize(20);
		info.connection.setConnected(true);
		info.connection.setSsid("IoT");
		info.connection.setDhcp(true);
		info.connection.setIp("192.168.29.101");
		info.connection.setNetmask("255.255.255.0");
		info.connection.setGateway("192.168.29.1");
		info.connection.setMac("840d8ea74d88");
		info.connection.setRssi(-63);
		info.mqtt.setStatus("disabled");
		info.mqtt.setEnabled(false);
		info.mqtt.setBroker("mqtt.local");
		info.mqtt.setTopic("home/");
		info.homeassistant.setEnabled(true);
		info.homeassistant.setDiscoveryPrefix("homeassistant");
		info.homeassistant.setNodeID("");
		info.runtime.setUptime(941760);
		info.runtime.setHeapFree(system_get_free_heap_size());
		info.runtime.setMinimumfreeHeapRuntime(5528);
		info.runtime.setMinimumfreeHeap10min(19216);
		info.runtime.setHeapLowErrUptime(0);
		info.runtime.setHeapLowErr10min(0);
		Serial << "heap used: " << (free - system_get_free_heap_size()) << " bytes" << endl;

		root.clearDirty();

		return {id, Message::Kind::result, root.getTagString()};
	}

	return {};
}

Message generateInfoV2Notification(int)
{
	int free = system_get_free_heap_size();
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		auto infoObject = update.toInfo();
		auto info = infoObject.params.toInfoV2Params();

		info.device.setDeviceid(10964360);
		info.device.setSoc("esp8266");
		info.device.setCurrentRom("rom0");
		info.app.setWebappVersion("V5.0-365-experimental");
		info.app.setGitVersion("V5.0.0-965-experimental");
		info.app.setBuildType("debug");
		info.app.setGitDate("2026-08-06");
		info.sming.setVersion("6.2.0");
		info.filesystem.setTotalBytes(1015808);
		info.filesystem.setFreeBytes(516096);
		info.filesystem.setUsedBytes(499712);
		info.rgbww.setVersion("0.10.0");
		info.rgbww.setQueuesize(20);
		info.connection.setConnected(true);
		info.connection.setSsid("IoT");
		info.connection.setDhcp(true);
		info.connection.setIp("192.168.29.101");
		info.connection.setNetmask("255.255.255.0");
		info.connection.setGateway("192.168.29.1");
		info.connection.setMac("840d8ea74d88");
		info.connection.setRssi(-63);
		info.mqtt.setStatus("disabled");
		info.mqtt.setEnabled(false);
		info.mqtt.setBroker("mqtt.local");
		info.mqtt.setTopic("home/");
		info.homeassistant.setEnabled(true);
		info.homeassistant.setDiscoveryPrefix("homeassistant");
		info.homeassistant.setNodeID("");
		info.runtime.setUptime(941760);
		info.runtime.setHeapFree(system_get_free_heap_size());
		info.runtime.setMinimumfreeHeapRuntime(5528);
		info.runtime.setMinimumfreeHeap10min(19216);
		info.runtime.setHeapLowErrUptime(0);
		info.runtime.setHeapLowErr10min(0);
		Serial << "heap used: " << (free - system_get_free_heap_size()) << " bytes" << endl;

		root.clearDirty();

		return {0, Message::Kind::notification, root.getTagString()};
	}

	return {};
}

Message generateErrorResponse(int id, const ErrorType error, String data = "")
{
	Jsonrpc::Root root(database);
	if(auto update = root.update()) {
		auto errorObject = update.toError();
		auto result = errorObject.result;
		switch(error) {
		case ParseError:
			result.setCode(-32700);
			result.setMessage("Parse error");
			break;
		case InvalidRequest:
			result.setCode(-32600);
			result.setMessage("Invalid Request");
			break;
		case MethodNotFound:
			result.setCode(-32601);
			result.setMessage("Method not found");
			break;
		case InvalidParams:
			result.setCode(-32602);
			result.setMessage("Invalid params");
			break;
		case InternalError:
			result.setCode(-32603);
			result.setMessage("Internal error");
			break;
		case ApplicationError1:
			result.setCode(-32000);
			result.setMessage("Application error 1");
			if(data != "")
				result.setData(data);
			break;
		case ApplicationError2:
			result.setCode(-32001);
			result.setMessage("Application error 2");
			if(data != "")
				result.setData(data);
			break;
		case ApplicationError3:
			result.setCode(-32002);
			result.setMessage("Application error 3");
			if(data != "")
				result.setData(data);
			break;
		}

		root.clearDirty();

		return {id, Message::Kind::result, root.getTagString()};
	}

	return {};
}

struct Generator {
	Message (*func)(int id);
	const char* desc;
};

Generator generators[]{
	{generateRawColorRequest, "RAW Color Request"},
	{generateRawColorResponse, "RAW Color Response"},
	{generateRawColorNotification, "Raw Color Notification"},
	{generateHsvColorRequest, "HSV Color Request"},
	{generateHsvColorResponse, "HSV Color Response"},
	{generateHsvColorNotification, "HSV Color Notification"},
	{generateInfoRequest, "Info Request"},
	{generateInfoV1Response, "Info V1 Response"},
	{generateInfoV1Notification, "Info V1 Notification"},
	{generateInfoV2Response, "Info V2 Response"},
	{generateInfoV2Notification, "Info V2 Notification"},
};

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

	for(auto& gen : generators) {
		Serial << gen.desc << endl;
		auto msg = gen.func(1);
		assert(msg);
		Jsonrpc::Root root(database);
		auto method = root.findObject(msg.method);
		auto body = method.findObject((msg.kind == Message::Kind::result) ? "result" : "params");
		printMessage(msg, body);
		Serial << endl << endl;
	}

	Serial << endl << "ERROR RESPONSES" << endl;

	auto printError = [](const Message& msg) {
		Jsonrpc::Root root(database);
		printMessage(msg, root.asError().result);
	};

	auto msg = generateErrorResponse(1, ErrorType::InvalidRequest);
	assert(msg);
	printError(msg);

	msg = generateErrorResponse(2, ErrorType::ApplicationError2, "something went horribly wrong");
	assert(msg);
	printError(msg);

	msg = generateErrorResponse(2, ErrorType::MethodNotFound);
	assert(msg);
	printError(msg);

	Serial << endl << endl;
}
