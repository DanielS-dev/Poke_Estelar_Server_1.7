#include "otpch.h"

#include "logger.hpp"

#include <chrono>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <system_error>

namespace {
std::string trim(std::string value)
{
	const auto notSpace = [](unsigned char ch) {
		return std::isspace(ch) == 0;
	};

	value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
	value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
	return value;
}

std::string toLower(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return value;
}

const char* getLevelName(LogLevel level)
{
	switch (level) {
		case LogLevel::Trace:
			return "trace";
		case LogLevel::Debug:
			return "debug";
		case LogLevel::Info:
			return "info";
		case LogLevel::Warn:
			return "warn";
		case LogLevel::Error:
			return "error";
		case LogLevel::Fatal:
			return "fatal";
		case LogLevel::Off:
		default:
			return "off";
	}
}

LogLevel parseLevel(const std::string& value)
{
	const std::string level = toLower(trim(value));
	if (level == "trace") {
		return LogLevel::Trace;
	}
	if (level == "debug") {
		return LogLevel::Debug;
	}
	if (level == "warn" || level == "warning") {
		return LogLevel::Warn;
	}
	if (level == "error") {
		return LogLevel::Error;
	}
	if (level == "fatal") {
		return LogLevel::Fatal;
	}
	if (level == "off") {
		return LogLevel::Off;
	}
	return LogLevel::Info;
}

std::string parseEnvValue(std::string value)
{
	value = trim(std::move(value));
	if (value.empty()) {
		return value;
	}

	const char first = value.front();
	if (first == '"' || first == '\'') {
		const auto endQuote = value.find(first, 1);
		if (endQuote != std::string::npos) {
			return value.substr(1, endQuote - 1);
		}
		return value.substr(1);
	}

	const auto commentPos = value.find('#');
	if (commentPos != std::string::npos) {
		value.erase(commentPos);
	}
	return trim(std::move(value));
}

std::unordered_map<std::string, std::string> loadEnvFile(const std::filesystem::path& fileName)
{
	std::unordered_map<std::string, std::string> env;
	std::ifstream file(fileName);
	if (!file.is_open()) {
		return env;
	}

	std::string line;
	while (std::getline(file, line)) {
		line = trim(std::move(line));
		if (line.empty() || line.front() == '#') {
			continue;
		}

		const auto separator = line.find('=');
		if (separator == std::string::npos) {
			continue;
		}

		const std::string key = trim(line.substr(0, separator));
		if (!key.empty()) {
			env[key] = parseEnvValue(line.substr(separator + 1));
		}
	}

	return env;
}

std::string getEnvString(const std::unordered_map<std::string, std::string>& env, const std::string& key,
                         const std::string& defaultValue)
{
	if (const auto it = env.find(key); it != env.end()) {
		return it->second;
	}
	return defaultValue;
}

bool getEnvBool(const std::unordered_map<std::string, std::string>& env, const std::string& key, bool defaultValue)
{
	if (const auto it = env.find(key); it != env.end()) {
		const std::string value = toLower(trim(it->second));
		return value == "1" || value == "true" || value == "yes" || value == "on";
	}
	return defaultValue;
}

uint32_t getEnvNumber(const std::unordered_map<std::string, std::string>& env, const std::string& key,
                      uint32_t defaultValue)
{
	if (const auto it = env.find(key); it != env.end()) {
		char* end = nullptr;
		const unsigned long value = std::strtoul(it->second.c_str(), &end, 10);
		if (end != it->second.c_str() && *end == '\0') {
			return static_cast<uint32_t>(value);
		}
	}
	return defaultValue;
}

std::string buildTimestamp()
{
	const auto now = std::chrono::system_clock::now();
	const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	const std::time_t time = std::chrono::system_clock::to_time_t(now);

	std::tm localTime {};
#ifdef _WIN32
	localtime_s(&localTime, &time);
#else
	localtime_r(&time, &localTime);
#endif

	std::ostringstream stream;
	stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0')
	       << millis.count();
	return stream.str();
}
} // namespace

Logger& Logger::getInstance()
{
	static Logger instance;
	return instance;
}

Logger::~Logger()
{
	shutdown();
}

void Logger::initialize(const Config& loggerConfig)
{
	shutdown();

	{
		std::scoped_lock lock(mutex);
		config = loggerConfig;
		if (config.file) {
			if (config.splitFilesByLevel) {
				std::filesystem::create_directories(config.directory);
			} else {
				std::filesystem::create_directories(config.fileName.parent_path());
				openFileStream(fileStream, config.fileName);
			}
		}

		running = true;
		initialized = true;
	}

	worker = std::thread(&Logger::threadMain, this);
}

void Logger::initializeFromEnv(const std::filesystem::path& fileName)
{
	const auto env = loadEnvFile(fileName);

	Config loggerConfig;
	const std::string globalLevel = getEnvString(env, "LOG_LEVEL", "info");
	loggerConfig.level = parseLevel(globalLevel);
	loggerConfig.consoleLevel = parseLevel(getEnvString(env, "LOG_CONSOLE_LEVEL", globalLevel));
	loggerConfig.fileLevel = parseLevel(getEnvString(env, "LOG_FILE_LEVEL", globalLevel));
	loggerConfig.console = getEnvBool(env, "LOG_TO_CONSOLE", true);
	loggerConfig.file = getEnvBool(env, "LOG_TO_FILE", true);
	loggerConfig.splitFilesByLevel = getEnvBool(env, "LOG_SPLIT_BY_LEVEL", true);
	loggerConfig.directory = getEnvString(env, "LOG_DIR", "logs");
	loggerConfig.fileName = getEnvString(env, "LOG_FILE", "logs/server.log");
	loggerConfig.maxFileSizeMb = getEnvNumber(env, "LOG_MAX_FILE_SIZE_MB", 10);
	loggerConfig.maxFiles = getEnvNumber(env, "LOG_MAX_FILES", 5);

	initialize(loggerConfig);
}

void Logger::shutdown()
{
	{
		std::scoped_lock lock(mutex);
		if (!initialized) {
			return;
		}
		running = false;
	}

	signal.notify_all();

	if (worker.joinable()) {
		worker.join();
	}

	std::scoped_lock lock(mutex);
	closeStreams();
	queue.clear();
	initialized = false;
}

bool Logger::shouldLog(LogLevel level) const
{
	std::scoped_lock lock(mutex);
	if (!initialized || config.level == LogLevel::Off || level < config.level) {
		return false;
	}

	const bool logToConsole = config.console && config.consoleLevel != LogLevel::Off && level >= config.consoleLevel;
	const bool logToFile = config.file && config.fileLevel != LogLevel::Off && level >= config.fileLevel;
	return logToConsole || logToFile;
}

void Logger::log(LogLevel level, std::string_view category, const std::string& message, const char* file, int line)
{
	LogMessage logMessage;
	logMessage.level = level;
	logMessage.category.assign(category.begin(), category.end());
	logMessage.message = message;
	logMessage.sourceFile = file ? file : "";
	logMessage.line = line;
	logMessage.timestamp = buildTimestamp();

	{
		std::scoped_lock lock(mutex);
		if (!initialized || config.level == LogLevel::Off || level < config.level) {
			return;
		}

		const bool logToConsole = config.console && config.consoleLevel != LogLevel::Off && level >= config.consoleLevel;
		const bool logToFile = config.file && config.fileLevel != LogLevel::Off && level >= config.fileLevel;
		if (!logToConsole && !logToFile) {
			return;
		}

		queue.push_back(std::move(logMessage));
	}

	signal.notify_one();
}

void Logger::threadMain()
{
	while (true) {
		LogMessage message;

		{
			std::unique_lock<std::mutex> lock(mutex);
			signal.wait(lock, [this] {
				return !running || !queue.empty();
			});

			if (queue.empty()) {
				if (!running) {
					break;
				}
				continue;
			}

			message = std::move(queue.front());
			queue.pop_front();
		}

		writeMessage(message);
	}
}

void Logger::writeMessage(const LogMessage& message)
{
	std::ostringstream fullMessage;
	fullMessage << '[' << message.timestamp << "] [" << getLevelName(message.level) << "] [" << message.category << "] "
	            << message.message;
	if (!message.sourceFile.empty()) {
		fullMessage << " (" << message.sourceFile << ':' << message.line << ')';
	}

	if (config.console && config.consoleLevel != LogLevel::Off && message.level >= config.consoleLevel) {
		if (message.level == LogLevel::Info) {
			std::cout << '[' << message.category << "] " << message.message << std::endl;
		} else {
			std::cout << '[' << getLevelName(message.level) << "] [" << message.category << "] " << message.message
			          << std::endl;
		}
	}

	if (config.file && config.fileLevel != LogLevel::Off && message.level >= config.fileLevel) {
		if (std::ofstream* output = getFileStream(message.level)) {
			const auto fileName = getFileName(message.level);
			rotateFileIfNeeded(fileName, *output);
			*output << fullMessage.str() << std::endl;
		}
	}
}

void Logger::rotateFileIfNeeded(const std::filesystem::path& fileName, std::ofstream& stream)
{
	if (config.maxFileSizeMb == 0 || config.maxFiles == 0) {
		return;
	}

	std::error_code ec;
	if (!std::filesystem::exists(fileName, ec)) {
		return;
	}

	const auto maxSize = static_cast<uintmax_t>(config.maxFileSizeMb) * 1024u * 1024u;
	const auto currentSize = std::filesystem::file_size(fileName, ec);
	if (ec || currentSize < maxSize) {
		return;
	}

	if (stream.is_open()) {
		stream.flush();
		stream.close();
	}

	for (uint32_t index = config.maxFiles; index > 0; --index) {
		auto current = fileName;
		current += "." + std::to_string(index);

		if (index == config.maxFiles) {
			std::filesystem::remove(current, ec);
			continue;
		}

		auto next = fileName;
		next += "." + std::to_string(index + 1);
		if (std::filesystem::exists(current, ec)) {
			std::filesystem::rename(current, next, ec);
		}
	}

	auto rotated = fileName;
	rotated += ".1";
	std::filesystem::rename(fileName, rotated, ec);
	openFileStream(stream, fileName);
}

std::ofstream* Logger::getFileStream(LogLevel level)
{
	if (!config.file) {
		return nullptr;
	}

	if (!config.splitFilesByLevel) {
		if (!fileStream.is_open() && !openFileStream(fileStream, config.fileName)) {
			return nullptr;
		}
		return &fileStream;
	}

	auto it = levelFileStreams.find(level);
	if (it != levelFileStreams.end()) {
		return &it->second;
	}

	const auto fileName = getFileName(level);
	std::filesystem::create_directories(fileName.parent_path());

	auto [insertedIt, inserted] = levelFileStreams.emplace(level, std::ofstream());
	(void)inserted;
	if (!openFileStream(insertedIt->second, fileName)) {
		return nullptr;
	}
	return &insertedIt->second;
}

std::filesystem::path Logger::getFileName(LogLevel level) const
{
	if (!config.splitFilesByLevel) {
		return config.fileName;
	}

	return config.directory / (std::string(getLevelName(level)) + ".log");
}

bool Logger::openFileStream(std::ofstream& stream, const std::filesystem::path& fileName)
{
	stream.open(fileName, std::ios::app);
	if (stream.is_open()) {
		return true;
	}

	disableFileLogging("failed to open log file: " + fileName.string());
	return false;
}

void Logger::closeStreams()
{
	if (fileStream.is_open()) {
		fileStream.flush();
		fileStream.close();
	}

	for (auto& [level, stream] : levelFileStreams) {
		(void)level;
		if (stream.is_open()) {
			stream.flush();
			stream.close();
		}
	}
	levelFileStreams.clear();
}

void Logger::disableFileLogging(const std::string& reason)
{
	config.file = false;
	closeStreams();
	std::cerr << "[error] [Logger] File logging disabled: " << reason << std::endl;
}
