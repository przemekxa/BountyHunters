#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <charconv>
#include <fstream>

using namespace std;

struct Config {
	uint8_t shopSize = 5;
	uint8_t minOrders = 2;
	uint8_t maxOrders = 5;
    int64_t hunterMin = 1;
    int64_t hunterMax = 2;
    uint8_t storeWaitMin = 1;
    uint8_t storeWaitMax = 10;
    uint8_t missionWaitMin = 1;
    uint8_t missionWaitMax = 10;

	// Set a value by field name
	void set(string_view key, string_view value) {

		// Convert string_view to uint8_t
		uint8_t intValue;
		auto result = from_chars(value.begin(), value.end(), intValue);
		if(result.ec != errc()) { return; }

		if(key == "shopSize") {
			shopSize = intValue;
		} else if(key == "minOrders") {
			minOrders = intValue;
		} else if(key == "maxOrders") {
			maxOrders = intValue;
		}
	}

	// Load config from file
	static Config fromFile(const char* filename = "config.ini") {
		Config config;
		ifstream file(filename);
		if(file.good()) {
			string line;
			line.reserve(64);
			while(getline(file, line)) {
				if(line.empty() || line[0] == '#') continue;
				string_view view { line };
				size_t delimeter = view.find_first_of('=');
				if(delimeter == string::npos) continue;
				string_view key = view.substr(0, delimeter);
				string_view value = view.substr(delimeter + 1);
				config.set(key, value);
			}
		}
		file.close();

		return config;
	}
};

#endif