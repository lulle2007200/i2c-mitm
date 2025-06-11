#pragma once

#include <stratosphere.hpp>

namespace ams::mitm::i2c {
	struct I2CMitmConfig {
		int voltage;
		u8 voltage_config;
	};

	Result InitializeConfig();
	const I2CMitmConfig &GetConfig();
	void LogConfig();

}