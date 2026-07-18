// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "otserv.h"

#include "configmanager.h"
#include "databasemanager.h"
#include "databasetasks.h"
#include "game.h"
#include "http/http.h"
#include "iomarket.h"
#include "logger.hpp"
#include "monsters.h"
#include "outfit.h"
#include "protocolstatus.h"
#include "rsa.h"
#include "scheduler.h"
#include "script.h"
#include "scriptmanager.h"
#include "server.h"

#include <fstream>

#if __has_include("gitmetadata.h")
#include "gitmetadata.h"
#endif

DatabaseTasks g_databaseTasks;
Dispatcher g_dispatcher;
Scheduler g_scheduler;

Game g_game;
Monsters g_monsters;
Vocations g_vocations;
extern Scripts* g_scripts;

std::mutex g_loaderLock;
std::condition_variable g_loaderSignal;
std::unique_lock<std::mutex> g_loaderUniqueLock(g_loaderLock);

namespace {

void startupErrorMessage(const std::string& errorStr)
{
	LOG_ERROR("Startup", errorStr);
	g_loaderSignal.notify_all();
}

void mainLoader(ServiceManager* services)
{
	// dispatcher thread
	g_game.setGameState(GAME_STATE_STARTUP);

	srand(static_cast<unsigned int>(OTSYS_TIME()));
#ifdef _WIN32
	SetConsoleTitle(STATUS_SERVER_NAME);

	// fixes a problem with escape characters not being processed in Windows consoles
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hOut, dwMode);
#endif

	printServerVersion();
	LOG_INFO("Startup", "Starting server...");

	// check if config.lua or config.lua.dist exist
	const std::string& configFile = getString(ConfigManager::CONFIG_FILE);
	std::ifstream c_test("./" + configFile);
	if (!c_test.is_open()) {
		std::ifstream config_lua_dist("./config.lua.dist");
		if (config_lua_dist.is_open()) {
			LOG_STDOUT << ">> copying config.lua.dist to " << configFile << std::endl;
			std::ofstream config_lua(configFile);
			config_lua << config_lua_dist.rdbuf();
			config_lua.close();
			config_lua_dist.close();
		}
	} else {
		c_test.close();
	}

	// read global config
	LOG_STDOUT << ">> Loading config" << std::endl;
	if (!ConfigManager::load()) {
		startupErrorMessage("Unable to load " + configFile + "!");
		return;
	}
	LOG_DEBUG("Startup", "Config loaded: " + configFile);

#ifdef _WIN32
	const std::string& defaultPriority = getString(ConfigManager::DEFAULT_PRIORITY);
	if (caseInsensitiveEqual(defaultPriority, "high")) {
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	} else if (caseInsensitiveEqual(defaultPriority, "above-normal")) {
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
	}
#endif

	// set RSA key
	LOG_STDOUT << ">> Loading RSA key " << std::endl;
	try {
		std::ifstream key{"key.pem"};
		std::string pem{std::istreambuf_iterator<char>{key}, std::istreambuf_iterator<char>{}};
		tfs::rsa::loadPEM(pem);
	} catch (const std::exception& e) {
		startupErrorMessage(e.what());
		return;
	}
	LOG_DEBUG("Startup", "RSA key loaded");

	LOG_STDOUT << ">> Establishing database connection..." << std::flush;

	if (!Database::getInstance().connect()) {
		startupErrorMessage("Failed to connect to database.");
		return;
	}

	LOG_INFO("Startup", std::string("Database connected: MySQL ") + Database::getClientVersion());

	// run database manager
	LOG_STDOUT << ">> Running database manager" << std::endl;

	if (!DatabaseManager::isDatabaseSetup()) {
		startupErrorMessage(
		    "The database you have specified in config.lua is empty, please import the schema.sql to your database.");
		return;
	}
	g_databaseTasks.start();
	LOG_DEBUG("Database", "Database manager started");

	DatabaseManager::updateDatabase();
	LOG_DEBUG("Database", "Database update check finished");

	if (getBoolean(ConfigManager::OPTIMIZE_DATABASE) && !DatabaseManager::optimizeTables()) {
		LOG_WARN("Database", "No tables were optimized");
	}

	// load vocations
	LOG_STDOUT << ">> Loading vocations" << std::endl;
	if (std::ifstream is{"data/XML/vocations.xml"}; !g_vocations.loadFromXml(is, "data/XML/vocations.xml")) {
		startupErrorMessage("Unable to load vocations!");
		return;
	}
	LOG_DEBUG("Startup", "Vocations loaded");

	// load item data
	LOG_STDOUT << ">> Loading items... ";
	if (!Item::items.loadFromOtb("data/items/items.otb")) {
		startupErrorMessage("Unable to load items (OTB)!");
		return;
	}
	LOG_STDOUT << fmt::format("OTB v{:d}.{:d}.{:d}", Item::items.majorVersion, Item::items.minorVersion,
	                         Item::items.buildNumber)
	          << std::endl;
	LOG_DEBUG("Startup", fmt::format("Items OTB loaded: v{:d}.{:d}.{:d}", Item::items.majorVersion,
	                                 Item::items.minorVersion, Item::items.buildNumber));

	if (!Item::items.loadFromXml()) {
		startupErrorMessage("Unable to load items (XML)!");
		return;
	}
	LOG_INFO("Startup", fmt::format("Items loaded: OTB v{:d}.{:d}.{:d}", Item::items.majorVersion,
	                                Item::items.minorVersion, Item::items.buildNumber));

	LOG_STDOUT << ">> Loading script systems" << std::endl;
	if (!ScriptingManager::getInstance().loadScriptSystems()) {
		startupErrorMessage("Failed to load script systems");
		return;
	}
	LOG_DEBUG("Scripts", "Script systems loaded");

	LOG_STDOUT << ">> Loading lua scripts" << std::endl;
	if (!g_scripts->loadScripts("scripts", false, false)) {
		startupErrorMessage("Failed to load lua scripts");
		return;
	}
	LOG_DEBUG("Scripts", "Lua scripts loaded");

	LOG_STDOUT << ">> Loading monsters" << std::endl;
	if (!g_monsters.loadFromXml()) {
		startupErrorMessage("Unable to load monsters!");
		return;
	}
	LOG_INFO("Startup", "Monsters loaded");

	LOG_STDOUT << ">> Loading lua monsters" << std::endl;
	if (!g_scripts->loadScripts("monster", false, false)) {
		startupErrorMessage("Failed to load lua monsters");
		return;
	}
	LOG_DEBUG("Scripts", "Lua monster scripts loaded");

	LOG_STDOUT << ">> Loading outfits" << std::endl;
	if (!Outfits::getInstance().loadFromXml()) {
		startupErrorMessage("Unable to load outfits!");
		return;
	}
	LOG_DEBUG("Startup", "Outfits loaded");

	LOG_STDOUT << ">> Checking world type... " << std::flush;
	std::string worldType = boost::algorithm::to_lower_copy(getString(ConfigManager::WORLD_TYPE));
	if (worldType == "pvp") {
		g_game.setWorldType(WORLD_TYPE_PVP);
	} else if (worldType == "no-pvp") {
		g_game.setWorldType(WORLD_TYPE_NO_PVP);
	} else if (worldType == "pvp-enforced") {
		g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
	} else {
		LOG_STDOUT << std::endl;
		startupErrorMessage(
		    fmt::format("Unknown world type: {:s}, valid world types are: pvp, no-pvp and pvp-enforced.",
		                getString(ConfigManager::WORLD_TYPE)));
		return;
	}
	LOG_STDOUT << boost::algorithm::to_upper_copy(worldType) << std::endl;
	LOG_DEBUG("Startup", "World type set to: " + boost::algorithm::to_upper_copy(worldType));

	LOG_STDOUT << ">> Loading map" << std::endl;
	if (!g_game.loadMainMap(getString(ConfigManager::MAP_NAME))) {
		startupErrorMessage("Failed to load map");
		return;
	}
	LOG_DEBUG("Startup", "Map loaded: " + getString(ConfigManager::MAP_NAME));

	LOG_STDOUT << ">> Initializing gamestate" << std::endl;
	g_game.setGameState(GAME_STATE_INIT);
	LOG_DEBUG("Startup", "Game state initialized");

	// Game client protocols
	services->add<ProtocolGame>(static_cast<uint16_t>(getNumber(ConfigManager::GAME_PORT)));

	// OT protocols
	services->add<ProtocolStatus>(static_cast<uint16_t>(getNumber(ConfigManager::STATUS_PORT)));

#ifdef HTTP
	// HTTP server
	tfs::http::start(getBoolean(ConfigManager::BIND_ONLY_GLOBAL_ADDRESS), getString(ConfigManager::IP),
	                 getNumber(ConfigManager::HTTP_PORT), getNumber(ConfigManager::HTTP_WORKERS));
#endif

	RentPeriod_t rentPeriod;
	std::string strRentPeriod = boost::algorithm::to_lower_copy(getString(ConfigManager::HOUSE_RENT_PERIOD));

	if (strRentPeriod == "yearly") {
		rentPeriod = RENTPERIOD_YEARLY;
	} else if (strRentPeriod == "weekly") {
		rentPeriod = RENTPERIOD_WEEKLY;
	} else if (strRentPeriod == "monthly") {
		rentPeriod = RENTPERIOD_MONTHLY;
	} else if (strRentPeriod == "daily") {
		rentPeriod = RENTPERIOD_DAILY;
	} else {
		rentPeriod = RENTPERIOD_NEVER;
	}

	g_game.map.houses.payHouses(rentPeriod);

	tfs::iomarket::checkExpiredOffers();
	tfs::iomarket::updateStatistics();
	LOG_DEBUG("Startup", "House rent and market maintenance finished");

	LOG_STDOUT << ">> Loaded all modules, server starting up..." << std::endl;
	LOG_INFO("Startup", "Core systems loaded");

#ifndef _WIN32
	if (getuid() == 0 || geteuid() == 0) {
		LOG_STDOUT << "> Warning: " << STATUS_SERVER_NAME
		          << " has been executed as root user, please consider running it as a normal user." << std::endl;
		LOG_WARN("Startup", std::string(STATUS_SERVER_NAME) + " is running as root user");
	}
#endif

	g_game.start(services);
	g_game.setGameState(GAME_STATE_NORMAL);
	LOG_DEBUG("Startup", "Game state set to normal");
	g_loaderSignal.notify_all();
}

[[noreturn]] void badAllocationHandler()
{
	LOG_FATAL("Memory", "Allocation failed, server out of memory. Decrease the size of your map or compile in 64 bits mode.");
	getchar();
	exit(-1);
}

} // namespace

void startServer()
{
	// Setup bad allocation handler
	std::set_new_handler(badAllocationHandler);

	ServiceManager serviceManager;
	LOG_DEBUG("Startup", "Starting dispatcher and scheduler");

	g_dispatcher.start();
	g_scheduler.start();

	g_dispatcher.addTask([services = &serviceManager]() { mainLoader(services); });

	g_loaderSignal.wait(g_loaderUniqueLock);

	if (serviceManager.is_running()) {
		LOG_INFO("Startup", getString(ConfigManager::SERVER_NAME) + " server online");
		LOG_STDOUT << ">> " << getString(ConfigManager::SERVER_NAME) << " Server Online!" << std::endl << std::endl;
		serviceManager.run();
		LOG_DEBUG("Shutdown", "Service manager finished");
	} else {
		LOG_WARN("Startup", "No services running. Server is not online");
		LOG_STDOUT << ">> No services running. The server is NOT online." << std::endl;
		g_scheduler.shutdown();
		g_databaseTasks.shutdown();
		g_dispatcher.shutdown();
	}

	LOG_DEBUG("Shutdown", "Waiting worker threads to finish");
	g_scheduler.join();
	g_databaseTasks.join();
	g_dispatcher.join();
	LOG_DEBUG("Shutdown", "All worker threads finished");
}

void printServerVersion()
{
#if defined(GIT_RETRIEVED_STATE) && GIT_RETRIEVED_STATE
	LOG_INFO("Startup", std::string(STATUS_SERVER_NAME) + " - Version " + GIT_DESCRIBE);
	LOG_DEBUG("Startup", std::string("Git SHA1 ") + GIT_SHORT_SHA1 + " dated " + GIT_COMMIT_DATE_ISO8601);
	LOG_STDOUT << STATUS_SERVER_NAME << " - Version " << GIT_DESCRIBE << std::endl;
	LOG_STDOUT << "Git SHA1 " << GIT_SHORT_SHA1 << " dated " << GIT_COMMIT_DATE_ISO8601 << std::endl;
#if GIT_IS_DIRTY
	LOG_WARN("Startup", "Dirty build detected");
	LOG_STDOUT << "*** DIRTY - NOT OFFICIAL RELEASE ***" << std::endl;
#endif
#else
	LOG_INFO("Startup", std::string(STATUS_SERVER_NAME) + " - Version " + STATUS_SERVER_VERSION);
	LOG_STDOUT << STATUS_SERVER_NAME << " - Version " << STATUS_SERVER_VERSION << std::endl;
#endif
	LOG_INFO("Startup", std::string("Developers: ") + STATUS_SERVER_DEVELOPERS);
	LOG_DEBUG("Startup", std::string("Compiled with ") + BOOST_COMPILER);
	LOG_DEBUG("Startup", std::string("Compiled on ") + __DATE__ + " " + __TIME__);
#if defined(LUAJIT_VERSION)
	LOG_DEBUG("Startup", std::string("Linked with ") + LUAJIT_VERSION + " for Lua support");
#else
	LOG_DEBUG("Startup", std::string("Linked with ") + LUA_RELEASE + " for Lua support");
#endif
}
