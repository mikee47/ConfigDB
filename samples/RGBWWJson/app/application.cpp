#include <SmingCore.h>
#include "jsonrpc.h"

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
template <typename Object> void printMessage(const Object& object)
{
	Serial << object << _F("\r\n");
}

[[maybe_unused]] bool generateRawColorRequest(Jsonrpc& db, int id)
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto colorRequest = update.toColorRequest();
			colorRequest.setId(id);
			colorRequest.setMethod("color");

			auto rawRequest = colorRequest.params.toRawColor(); // make this a color request
			rawRequest.raw.setR(1023);
			rawRequest.raw.setG(512);
			rawRequest.raw.setB(128);
			rawRequest.raw.setWw(64);
			rawRequest.raw.setCw(32);
			printMessage(colorRequest);
			colorRequest.clearDirty();
		} else 
			return false;	
	}
	return true;
}
[[maybe_unused]] bool generateRawColorResponse(Jsonrpc& db, int id)
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto colorResponse = update.toColorResponse(); // make this a color response
			int id_ = colorResponse.getId();
			auto rawResponse = colorResponse.result.toRawColor();
			rawResponse.raw.setR(1023);
			rawResponse.raw.setG(512);
			rawResponse.raw.setB(128);
			rawResponse.raw.setWw(64);
			rawResponse.raw.setCw(32);
			printMessage(colorResponse);
			root.clearDirty();
		} else 
			return false;	
	}
	return true;
}
[[maybe_unused]] bool generateRawColorNotification(Jsonrpc& db)
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto notification = update.toColorEvent(); // make this a color notification
			notification.setMethod("color");
			auto rawEvent = notification.params.toRawColor();
			rawEvent.raw.setR(1023);
			rawEvent.raw.setG(512);
			rawEvent.raw.setB(128);
			rawEvent.raw.setWw(64);
			rawEvent.raw.setCw(32);
			printMessage(notification);
			root.clearDirty();
		} else 
			return false;	
	}
	return true;
}
[[maybe_unused]] bool generateHsvColorRequest(Jsonrpc& db, int id)
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto colorRequest = update.toColorRequest(); // make this a color request
			colorRequest.setId(id);
			colorRequest.setMethod("color");
			auto hsvRequest = colorRequest.params.toHsvColor();
			hsvRequest.hsv.setH(210);
			hsvRequest.hsv.setS(75);
			hsvRequest.hsv.setV(60);
			hsvRequest.hsv.setCt(3500);
			printMessage(colorRequest);
			root.clearDirty();
		} else 
			return false;	
	}
	return true;
}
[[maybe_unused]] bool generateHsvColorResponse(Jsonrpc& db, int id)
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto colorResponse = update.asColorResponse(); // make this a color response
			int id_ = colorResponse.getId();
			auto hsvResponse = colorResponse.result.toHsvColor();
			hsvResponse.hsv.setH(210);
			hsvResponse.hsv.setS(75);
			hsvResponse.hsv.setV(60);
			hsvResponse.hsv.setCt(3500);
			printMessage(colorResponse);
			root.clearDirty();
		} else 
			return false;	
	}
	return true;
}
[[maybe_unused]] bool generateHsvColorNotification(Jsonrpc& db)
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto notification = update.toColorEvent(); // make this a color notification
			notification.setMethod("color");

			auto hsv = notification.params.toHsvColor();
			hsv.hsv.setH(210);
			hsv.hsv.setS(75);
			hsv.hsv.setV(60);
			hsv.hsv.setCt(3500);
			printMessage(notification);
			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
}

[[maybe_unused]] bool generateInfoRequest(Jsonrpc& db, int id)
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto request = update.toInfoRequest();
			request.setId(id);
			request.setMethod("info");
			printMessage(request);
			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
}

[[maybe_unused]] bool generateInfoV1Response(Jsonrpc& db, int id)
{
	int free=system_get_free_heap_size();	
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto message = update.asInfoResponse();
			message.setId(id);

			auto result = message.result.toInfoV1Params();
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
			Serial << "heap used: " << (free - system_get_free_heap_size()) << " bytes" << endl;
			printMessage(message);
			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
}

[[maybe_unused]] bool generateInfoV1Notification(Jsonrpc& db)
{
	int free=system_get_free_heap_size();
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto notification = update.toInfoEvent();
			notification.setMethod("info");

			auto params = notification.params.toInfoV1Params();
			params.setDeviceid(10964360);
			params.setSoc("esp8266");
			params.setCurrentRom("rom0");
			params.setGitVersion("V5.0.0-965-experimental");
			params.setBuildType("debug");
			params.setGitDate("2026-08-06");
			params.setWebappVersion("V5.0-365-experimental");
			params.setSming("6.2.0");
			params.setEventNumClients(1);
			params.setUptime(358620);
			params.setHeapFree(16912);
			params.rgbww.setVersion("0.10.0");
			params.rgbww.setQueuesize(20);
			params.connection.setConnected(true);
			params.connection.setSsid("IoT");
			params.connection.setDhcp(true);
			params.connection.setIp("192.168.29.101");
			params.connection.setNetmask("255.255.255.0");
			params.connection.setGateway("192.168.29.1");
			params.connection.setMac("840d8ea74d88");
			Serial << "heap used: " << (free - system_get_free_heap_size()) << " bytes" << endl;
			
			printMessage(notification);
			root.clearDirty();
		}
	}
	return true;
}

[[maybe_unused]] bool generateInfoV2Message(Jsonrpc& db, int id)
{
	int free=system_get_free_heap_size();
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto message = update.toInfoResponse();
			message.setId(id);
			
			auto result = message.result.toInfoV2Params();
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
			Serial << "heap used: " << (free - system_get_free_heap_size()) << " bytes" << endl;

			printMessage(message);
			
			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
}
[[maybe_unused]] bool generateInfoV2Notification(Jsonrpc& db)
{
	int free=system_get_free_heap_size();
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto notification = update.toInfoEvent();
			notification.setMethod("info");

			auto params = notification.params.toInfoV2Params();
			params.device.setDeviceid(10964360);
			params.device.setSoc("esp8266");
			params.device.setCurrentRom("rom0");
			params.app.setWebappVersion("V5.0-365-experimental");
			params.app.setGitVersion("V5.0.0-965-experimental");
			params.app.setBuildType("debug");
			params.app.setGitDate("2026-08-06");
			params.sming.setVersion("6.2.0");
			params.filesystem.setTotalBytes(1015808);
			params.filesystem.setFreeBytes(516096);
			params.filesystem.setUsedBytes(499712);
			params.rgbww.setVersion("0.10.0");
			params.rgbww.setQueuesize(20);
			params.connection.setConnected(true);
			params.connection.setSsid("IoT");
			params.connection.setDhcp(true);
			params.connection.setIp("192.168.29.101");
			params.connection.setNetmask("255.255.255.0");
			params.connection.setGateway("192.168.29.1");
			params.connection.setMac("840d8ea74d88");
			params.connection.setRssi(-63);
			params.mqtt.setStatus("disabled");
			params.mqtt.setEnabled(false);
			params.mqtt.setBroker("mqtt.local");
			params.mqtt.setTopic("home/");
			params.homeassistant.setEnabled(true);
			params.homeassistant.setDiscoveryPrefix("homeassistant");
			params.homeassistant.setNodeID("");
			params.runtime.setUptime(941760);
			params.runtime.setHeapFree(system_get_free_heap_size());
			params.runtime.setMinimumfreeHeapRuntime(5528);
			params.runtime.setMinimumfreeHeap10min(19216);
			params.runtime.setHeapLowErrUptime(0);
			params.runtime.setHeapLowErr10min(0);
			Serial << "heap used: " << (free - system_get_free_heap_size()) << " bytes" << endl;

			printMessage(notification);

			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
}
[[maybe_unused]] bool generateErrorResponse(Jsonrpc& db, int id, const ErrorType error, String data="")
{
	{
		Jsonrpc::Root root(db);
		if(auto update = root.update()) {
			auto message = update.toErrorResponse();
			message.setId(id);
			auto result = message.error;
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
					if(data!="")
						result.setData(data);
					break;
				case ApplicationError2:
					result.setCode(-32001);
					result.setMessage("Application error 2");
					if(data!="")
						result.setData(data);
					break;
				case ApplicationError3:
					result.setCode(-32002);
					result.setMessage("Application error 3");
					if(data!="")
						result.setData(data);
					break;
			}
			printMessage(message);
			root.clearDirty();
		} else {
			return false;
		}
	}
	return true;
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

	if(!generateRawColorResponse(db, 1)) {
		Serial << "Failed to update color database" << endl;
	}
	if(!generateInfoV2Notification(db)) {
		Serial << "Failed to generate info v2 notification" << endl;
	}

	if(!generateInfoV2Notification(db)) {
		Serial << "Failed to generate info v2 notification" << endl;
	}

	if(!generateInfoRequest(db, 1)) {
		Serial << "Failed to generate info request" << endl;
	}

	if(!generateErrorResponse(db, 1, ErrorType::InvalidRequest)) {
		Serial << "Failed to generate error response" << endl;
	}
	
	if(!generateErrorResponse(db, 2, ErrorType::ApplicationError2, "something went horribly wrong")) {
		Serial << "Failed to generate error response" << endl;
	}
	if(!generateErrorResponse(db, 2, ErrorType::MethodNotFound)) {
		Serial << "Failed to generate error response" << endl;
	}
	Serial << endl << endl;
}
