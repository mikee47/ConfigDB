#include <SmingCore.h>
#include <ConfigDB/Json/Store.h>
#include <ConfigDB/Database.h>
#include <ConfigDB/DataStream.h>
#include <LittleFS.h>

namespace
{
class BasicConfig : public ConfigDB::Database
{
public:
	DEFINE_FSTR_LOCAL(fstr_empty, "")
	DEFINE_FSTR_LOCAL(fstr_general, "general")
	DEFINE_FSTR_LOCAL(fstr_color, "color")
	DEFINE_FSTR_LOCAL(fstr_events, "events")
	DEFINE_FSTR_LOCAL(fstr_pin, "pin")
	DEFINE_FSTR_LOCAL(fstr_name, "name")
	DEFINE_FSTR_LOCAL(fstr_red, "red")
	DEFINE_FSTR_LOCAL(fstr_13, "13")
	DEFINE_FSTR_LOCAL(fstr_general_channels, "general.channels")
	DEFINE_FSTR_LOCAL(fstr_RGB, "RGB")
	DEFINE_FSTR_LOCAL(fstr_general_supported_color_models, "general.supported_color_models")
	DEFINE_FSTR_LOCAL(fstr_device_name, "device_name")
	DEFINE_FSTR_LOCAL(fstr_pin_config_url, "pin_config_url")
	DEFINE_FSTR_LOCAL(fstr_https_raw_githubusercontent_com_,
					  "https://raw.githubusercontent.com/pljakobs/esp_rgb_webapp2/devel/public/config/pinconfig.json")
	DEFINE_FSTR_LOCAL(fstr_current_pin_config_name, "current_pin_config_name")
	DEFINE_FSTR_LOCAL(fstr_mrpj, "mrpj")
	DEFINE_FSTR_LOCAL(fstr_buttons_debounce_ms, "buttons_debounce_ms")
	DEFINE_FSTR_LOCAL(fstr_pin_config, "pin_config")
	DEFINE_FSTR_LOCAL(fstr_13_12_14_5_4, "13,12,14,5,4")
	DEFINE_FSTR_LOCAL(fstr_buttons_config, "buttons_config")
	DEFINE_FSTR_LOCAL(fstr_50, "50")
	DEFINE_FSTR_LOCAL(fstr_channels, "channels")
	DEFINE_FSTR_LOCAL(fstr_supported_color_models, "supported_color_models")
	DEFINE_FSTR_LOCAL(fstr_api_secured, "api_secured")
	DEFINE_FSTR_LOCAL(fstr_False, "False")
	DEFINE_FSTR_LOCAL(fstr_security, "security")
	DEFINE_FSTR_LOCAL(fstr_ww, "ww")
	DEFINE_FSTR_LOCAL(fstr_green, "green")
	DEFINE_FSTR_LOCAL(fstr_blue, "blue")
	DEFINE_FSTR_LOCAL(fstr_cw, "cw")
	DEFINE_FSTR_LOCAL(fstr_100, "100")
	DEFINE_FSTR_LOCAL(fstr_color_brightness, "color.brightness")
	DEFINE_FSTR_LOCAL(fstr_2700, "2700")
	DEFINE_FSTR_LOCAL(fstr_6000, "6000")
	DEFINE_FSTR_LOCAL(fstr_color_colortemp, "color.colortemp")
	DEFINE_FSTR_LOCAL(fstr_magenta, "magenta")
	DEFINE_FSTR_LOCAL(fstr_yellow, "yellow")
	DEFINE_FSTR_LOCAL(fstr_model, "model")
	DEFINE_FSTR_LOCAL(fstr_cyan, "cyan")
	DEFINE_FSTR_LOCAL(fstr_0, "0")
	DEFINE_FSTR_LOCAL(fstr_color_hsv, "color.hsv")
	DEFINE_FSTR_LOCAL(fstr_startup_color, "startup_color")
	DEFINE_FSTR_LOCAL(fstr_last, "last")
	DEFINE_FSTR_LOCAL(fstr_outputmode, "outputmode")
	DEFINE_FSTR_LOCAL(fstr_brightness, "brightness")
	DEFINE_FSTR_LOCAL(fstr_colortemp, "colortemp")
	DEFINE_FSTR_LOCAL(fstr_hsv, "hsv")
	DEFINE_FSTR_LOCAL(fstr_url, "url")
	DEFINE_FSTR_LOCAL(fstr_https_lightinator_de_version_jso, "https://lightinator.de/version.json")
	DEFINE_FSTR_LOCAL(fstr_ota, "ota")
	DEFINE_FSTR_LOCAL(fstr_cmd_master_enabled, "cmd_master_enabled")
	DEFINE_FSTR_LOCAL(fstr_color_slave_enabled, "color_slave_enabled")
	DEFINE_FSTR_LOCAL(fstr_color_slave_topic, "color_slave_topic")
	DEFINE_FSTR_LOCAL(fstr_home_led1_color, "home/led1/color")
	DEFINE_FSTR_LOCAL(fstr_clock_master_enabled, "clock_master_enabled")
	DEFINE_FSTR_LOCAL(fstr_color_master_interval_ms, "color_master_interval_ms")
	DEFINE_FSTR_LOCAL(fstr_clock_slave_enabled, "clock_slave_enabled")
	DEFINE_FSTR_LOCAL(fstr_cmd_slave_enabled, "cmd_slave_enabled")
	DEFINE_FSTR_LOCAL(fstr_clock_master_interval, "clock_master_interval")
	DEFINE_FSTR_LOCAL(fstr_clock_slave_topic, "clock_slave_topic")
	DEFINE_FSTR_LOCAL(fstr_home_led1_clock, "home/led1/clock")
	DEFINE_FSTR_LOCAL(fstr_cmd_slave_topic, "cmd_slave_topic")
	DEFINE_FSTR_LOCAL(fstr_home_led1_command, "home/led1/command")
	DEFINE_FSTR_LOCAL(fstr_color_master_enabled, "color_master_enabled")
	DEFINE_FSTR_LOCAL(fstr_30, "30")
	DEFINE_FSTR_LOCAL(fstr_sync, "sync")
	DEFINE_FSTR_LOCAL(fstr_color_interval_ms, "color_interval_ms")
	DEFINE_FSTR_LOCAL(fstr_color_mininterval_ms, "color_mininterval_ms")
	DEFINE_FSTR_LOCAL(fstr_transfin_interval_ms, "transfin_interval_ms")
	DEFINE_FSTR_LOCAL(fstr_server_enabled, "server_enabled")
	DEFINE_FSTR_LOCAL(fstr_500, "500")
	DEFINE_FSTR_LOCAL(fstr_1000, "1000")
	DEFINE_FSTR_LOCAL(fstr_True, "True")
	DEFINE_FSTR_LOCAL(fstr_server, "server")
	DEFINE_FSTR_LOCAL(fstr_mqtt_local, "mqtt.local")
	DEFINE_FSTR_LOCAL(fstr_password, "password")
	DEFINE_FSTR_LOCAL(fstr_port, "port")
	DEFINE_FSTR_LOCAL(fstr_topic_base, "topic_base")
	DEFINE_FSTR_LOCAL(fstr_home_, "home/")
	DEFINE_FSTR_LOCAL(fstr_enabled, "enabled")
	DEFINE_FSTR_LOCAL(fstr_username, "username")
	DEFINE_FSTR_LOCAL(fstr_1883, "1883")
	DEFINE_FSTR_LOCAL(fstr_network_mqtt, "network.mqtt")
	DEFINE_FSTR_LOCAL(fstr_netmask, "netmask")
	DEFINE_FSTR_LOCAL(fstr_0_0_0_0, "0.0.0.0")
	DEFINE_FSTR_LOCAL(fstr_ip, "ip")
	DEFINE_FSTR_LOCAL(fstr_dhcp, "dhcp")
	DEFINE_FSTR_LOCAL(fstr_gateway, "gateway")
	DEFINE_FSTR_LOCAL(fstr_network_connection, "network.connection")
	DEFINE_FSTR_LOCAL(fstr_rgbwwctrl, "rgbwwctrl")
	DEFINE_FSTR_LOCAL(fstr_secured, "secured")
	DEFINE_FSTR_LOCAL(fstr_ssid, "ssid")
	DEFINE_FSTR_LOCAL(fstr_network_ap, "network.ap")
	DEFINE_FSTR_LOCAL(fstr_mqtt, "mqtt")
	DEFINE_FSTR_LOCAL(fstr_connection, "connection")
	DEFINE_FSTR_LOCAL(fstr_ap, "ap")
	DEFINE_FSTR_LOCAL(fstr_network, "network")

	class RootStore : public ConfigDB::Json::StoreTemplate<RootStore>
	{
	public:
		RootStore(ConfigDB::Database& db) : StoreTemplate(db, nullptr)
		{
		}
	};

	class Channels;

	class ChannelsItem : public ConfigDB::Json::Object
	{
	public:
		using Object::Object;

		int getPin() const
		{
			return getValue<int>(fstr_pin, 13);
		}

		bool setPin(const int& value)
		{
			return setValue(fstr_pin, value);
		}

		String getName() const
		{
			return getValue<String>(fstr_name, fstr_red);
		}

		bool setName(const String& value)
		{
			return setValue(fstr_name, value);
		}

		unsigned getObjectCount() const override
		{
			return 0;
		}

		std::unique_ptr<ConfigDB::Object> getObject(const String& name) override
		{
			return nullptr;
		}

		std::unique_ptr<ConfigDB::Object> getObject(unsigned index) override
		{
			return nullptr;
		}

		unsigned getPropertyCount() const override
		{
			return 2;
		}

		ConfigDB::Property getProperty(unsigned index) override
		{
			using namespace ConfigDB;
			switch(index) {
			case 0:
				return {*this, fstr_pin, Property::Type::Integer, &fstr_13};
			case 1:
				return {*this, fstr_name, Property::Type::String, &fstr_red};
			default:
				return {};
			}
		}
	};

	class Channels : public ConfigDB::Json::ObjectArrayTemplate<Channels, ChannelsItem>
	{
	public:
		Channels(BasicConfig& db)
			: ObjectArrayTemplate(*RootStore::open(db), fstr_general_channels), store(RootStore::open(db))
		{
		}

		ConfigDB::Store& getStore() override
		{
			return *store;
		}

	private:
		std::shared_ptr<ConfigDB::Json::Store> store;
	};

	// class General : public ConfigDB::Json::Object
	// {
	// public:
	// 	General(std::shared_ptr<ConfigDB::Json::Store&> store)
	// 		: Object(*store, fstr_general), : Object(root),
	// 										  store(RootStore::open(db)),
	// 										  root(*store),
	// 										  channels(*this, fstr_channels),
	// 										  supportedColorModels(*this, fstr_supported_color_models)
	// 	{
	// 		init(root, fstr_general);
	// 	}

	// 	General(BasicConfig& db)
	// 		: Object(root), store(RootStore::open(db)), root(*store), channels(*this, fstr_channels),
	// 		  supportedColorModels(*this, fstr_supported_color_models)
	// 	{
	// 		init(root, fstr_general);
	// 	}

	// 	using ContainedChannels = ConfigDB::Json::ObjectArrayTemplate<Channels, ChannelsItem>;

	// 	class ContainedSupportedColorModels : public ConfigDB::Json::Array
	// 	{
	// 	public:
	// 		using Array::Array;

	// 		String getItem(unsigned index) const
	// 		{
	// 			return Array::getItem<String>(index, fstr_RGB);
	// 		}

	// 		bool setItem(unsigned index, const String& value)
	// 		{
	// 			return Array::setItem(index, value);
	// 		}

	// 		ConfigDB::Property getProperty(unsigned index) override
	// 		{
	// 			return getArrayProperty(index, ConfigDB::Property::Type::String, &fstr_RGB);
	// 		}

	// 	private:
	// 		std::shared_ptr<ConfigDB::Json::Store> store;
	// 		ConfigDB::Json::SimpleObject root;
	// 		ConfigDB::Json::SimpleObject general;
	// 	};

	// 	String getDeviceName() const
	// 	{
	// 		return getValue<String>(fstr_device_name, fstr_3);
	// 	}

	// 	bool setDeviceName(const String& value)
	// 	{
	// 		return setValue(fstr_device_name, value);
	// 	}

	// 	String getPinConfigUrl() const
	// 	{
	// 		return getValue<String>(fstr_pin_config_url, fstr_https_raw_githubusercontent_com_);
	// 	}

	// 	bool setPinConfigUrl(const String& value)
	// 	{
	// 		return setValue(fstr_pin_config_url, value);
	// 	}

	// 	String getCurrentPinConfigName() const
	// 	{
	// 		return getValue<String>(fstr_current_pin_config_name, fstr_mrpj);
	// 	}

	// 	bool setCurrentPinConfigName(const String& value)
	// 	{
	// 		return setValue(fstr_current_pin_config_name, value);
	// 	}

	// 	int getButtonsDebounceMs() const
	// 	{
	// 		return getValue<int>(fstr_buttons_debounce_ms, 50);
	// 	}

	// 	bool setButtonsDebounceMs(const int& value)
	// 	{
	// 		return setValue(fstr_buttons_debounce_ms, value);
	// 	}

	// 	String getPinConfig() const
	// 	{
	// 		return getValue<String>(fstr_pin_config, fstr_13_12_14_5_4);
	// 	}

	// 	bool setPinConfig(const String& value)
	// 	{
	// 		return setValue(fstr_pin_config, value);
	// 	}

	// 	String getButtonsConfig() const
	// 	{
	// 		return getValue<String>(fstr_buttons_config, fstr_3);
	// 	}

	// 	bool setButtonsConfig(const String& value)
	// 	{
	// 		return setValue(fstr_buttons_config, value);
	// 	}

	// 	unsigned getObjectCount() const override
	// 	{
	// 		return 2;
	// 	}

	// 	std::unique_ptr<ConfigDB::Object> getObject(const String& name) override
	// 	{
	// 		return nullptr;
	// 	}

	// 	std::unique_ptr<ConfigDB::Object> getObject(unsigned index) override
	// 	{
	// 		return nullptr;
	// 	}

	// 	unsigned getPropertyCount() const override
	// 	{
	// 		return 8;
	// 	}

	// 	ConfigDB::Property getProperty(unsigned index) override
	// 	{
	// 		return ConfigDB::Property{};
	// 	}

	// 	ConfigDB::Store& getStore() override
	// 	{
	// 		return *store;
	// 	}

	// 	ContainedChannels channels;
	// 	ContainedSupportedColorModels supportedColorModels;

	// private:
	// 	std::shared_ptr<ConfigDB::Json::Store> store;
	// 	ConfigDB::Json::SimpleObject root;
	// };

	using Database::Database;

	std::shared_ptr<ConfigDB::Store> getStore(unsigned index) override
	{
		switch(index) {
		case 0:
			return RootStore::open(*this);
		default:
			return nullptr;
		}
	}
};

} // namespace

void init()
{
	Serial.begin(COM_SPEED_SERIAL);
	Serial.systemDebugOutput(true);

#ifdef ARCH_HOST
	fileSetFileSystem(&IFS::Host::getFileSystem());
#else
	lfs_mount();
#endif

	createDirectory("test");
	BasicConfig db("test");
	db.setFormat(ConfigDB::Format::Pretty);

	IFS::Profiler profiler;
	getFileSystem()->setProfiler(&profiler);

	// BasicConfig::General gen(db);
	// gen.setButtonsConfig("Here lies buttons config");
	// gen.supportedColorModels.addItem("New model");
	// gen.commit();

	BasicConfig::Channels channels(db);
	auto item = channels.addItem();
	item.setName("My channel item");
	// item.setPin(123);
	item.commit();
	Serial << profiler << endl;

	Serial << "new item = " << item << endl;
	for(unsigned i = 0; auto item = channels.getItem(i); ++i) {
		Serial << "item.name = " << item.getName() << endl;
		Serial << "item.pin = " << item.getPin() << endl;
	}
	Serial << "channels = " << channels << endl;
	Serial << "store = " << channels.getStore() << endl;
	channels.commit();

	Serial << profiler << endl;

	// Serial << endl << _F("Database:") << endl;

	// ConfigDB::DataStream stream(db);
	// Serial.copyFrom(&stream);

#ifdef ARCH_HOST
	System.restart();
#endif
}
