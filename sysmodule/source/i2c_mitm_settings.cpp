#include "i2c_mitm_settings.hpp"
#include "logging.hpp"
#include <stratosphere.hpp>

namespace ams::mitm::i2c {

	namespace {
		constexpr const char config_file_path[] = "sdmc:/config/i2c_mitm/i2c_mitm.ini";

		constinit I2CMitmConfig g_i2c_config = {
			.voltage        = 4200,
			.voltage_config = 0xb2,
		};

		Result ParseInt(const char *value, int &out, int min = INT_MIN, int max = INT_MAX) {
			int tmp = std::strtol(value, nullptr, 10);
			if (tmp >= min && tmp <= max) {
				out = tmp;
				R_SUCCEED();
			}
			R_THROW(::ams::settings::ResultInvalidArgument());
		}

		Result ParseVoltage(const char *value, int &out_voltage, u8 &out_voltage_config) {
			int tmp;
			Result result = ParseInt(value, tmp, 3504, 4400);
			if (R_FAILED(result)) {
				log::DebugLog("Invalid voltage set in config (%" PRIi32"), must be in range 3504-4400mV. Using 4200mV\n");
				R_THROW(result);
			}

			out_voltage = tmp;
			tmp -= 3504;
			u8 voltage_config = 0x02;
			for (int i = 7; i >= 2; i--) {
				int v = (4 << i);
				if (tmp >= v) {
					tmp -= v;
					voltage_config |= (1 << i);
				}
			}

			out_voltage_config = voltage_config;

			R_SUCCEED();
		}

		int ConfigIniHandler(void *user, const char *section, const char *name, const char *value) {
			Result &result = *static_cast<Result*>(user);

			if (R_FAILED(result)) {
				return 0;
			}

			if (strcasecmp(section, "battery") == 0) {
				if (strcasecmp(name, "chrg_voltage") == 0) {
					result = ParseVoltage(value, g_i2c_config.voltage, g_i2c_config.voltage_config);
				}
			}

			if (R_FAILED(result)) {
				log::DebugLog("Failed to parse config\n");
			}

			return R_SUCCEEDED(result) ? 1 : 0;
		}

		Result LoadFromSD() {
			fs::FileHandle f;
			R_SUCCEED_IF(R_FAILED(fs::OpenFile(std::addressof(f), config_file_path, fs::OpenMode_Read)));
			ON_SCOPE_EXIT{fs::CloseFile(f);};

			Result result = ResultSuccess();
			util::ini::ParseFile(f, std::addressof(result), ConfigIniHandler);

			R_TRY(result);
			R_SUCCEED();
		}
	}

	Result InitializeConfig() {
		R_RETURN(LoadFromSD());
	}

	const I2CMitmConfig &GetConfig() {
		return g_i2c_config;
	}

	void LogConfig() {
		log::DebugLog("i2c mitm config: voltage: %" PRIi32 ", voltage config: 0x%" PRIx8 "\n", GetConfig().voltage, GetConfig().voltage_config);
	}
}