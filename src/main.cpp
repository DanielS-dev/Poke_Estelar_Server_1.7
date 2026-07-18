#include "otpch.h"

#include "configmanager.h"
#include "logger.hpp"
#include "otserv.h"
#include "tools.h"

static bool argumentsHandler(const std::vector<std::string_view>& args)
{
	for (const auto& arg : args) {
		LOG_DEBUG("Main", std::string("Processing argument: ") + std::string(arg));

		if (arg == "--help") {
			LOG_STDLOG << "Usage:\n"
			             "\n"
			             "\t--config=$1\t\tAlternate configuration file path.\n"
			             "\t--ip=$1\t\t\tIP address of the server.\n"
			             "\t\t\t\tShould be equal to the global IP.\n"
			             "\t--http-port=$1\tPort for http to listen on.\n"
			             "\t--game-port=$1\tPort for game server to listen on.\n";
			LOG_DEBUG("Main", "Help requested");
			return false;
		} else if (arg == "--version") {
			LOG_DEBUG("Main", "Version requested");
			printServerVersion();
			return false;
		}

		auto tmp = explodeString(arg, "=");

		if (tmp[0] == "--config") {
			ConfigManager::setString(ConfigManager::CONFIG_FILE, tmp[1]);
			LOG_DEBUG("Main", std::string("Config file overridden: ") + std::string(tmp[1]));
		} else if (tmp[0] == "--ip") {
			ConfigManager::setString(ConfigManager::IP, tmp[1]);
			LOG_DEBUG("Main", std::string("IP overridden: ") + std::string(tmp[1]));
		} else if (tmp[0] == "--http-port") {
			ConfigManager::setNumber(ConfigManager::HTTP_PORT, std::stoi(tmp[1].data()));
			LOG_DEBUG("Main", std::string("HTTP port overridden: ") + std::string(tmp[1]));
		} else if (tmp[0] == "--game-port") {
			ConfigManager::setNumber(ConfigManager::GAME_PORT, std::stoi(tmp[1].data()));
			LOG_DEBUG("Main", std::string("Game port overridden: ") + std::string(tmp[1]));
		}
	}

	return true;
}

int main(int argc, const char** argv)
{
	Logger::getInstance().initializeFromEnv(".env");

	std::vector<std::string_view> args(argv, argv + argc);
	if (!argumentsHandler(args)) {
		Logger::getInstance().shutdown();
		return 1;
	}

	startServer();
	Logger::getInstance().shutdown();
	return 0;
}
