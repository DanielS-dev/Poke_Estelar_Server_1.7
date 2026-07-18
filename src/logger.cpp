#include "otpch.h"

#include "logger.hpp"

#include <chrono>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
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

void writeToBuffer(std::streambuf* buffer, const std::string& line)
{
	if (!buffer) {
		return;
	}

	buffer->sputn(line.c_str(), static_cast<std::streamsize>(line.size()));
	buffer->sputc('\n');
	buffer->pubsync();
}
} // namespace

class Logger::RedirectStreamBuffer final : public std::streambuf
{
	public:
		RedirectStreamBuffer(LogLevel logLevel, std::string logCategory) :
		    level(logLevel), category(std::move(logCategory))
		{}

		void flushPending()
		{
			sync();
		}

	protected:
		int overflow(int character) override
		{
			if (character == traits_type::eof()) {
				return traits_type::not_eof(character);
			}

			buffer.push_back(static_cast<char>(character));
			if (character == '\n') {
				submitLines(false);
			}
			return character;
		}

		int sync() override
		{
			submitLines(true);
			return 0;
		}

	private:
		void submitLines(bool flushPartial)
		{
			size_t start = 0;
			while (true) {
				const size_t lineBreak = buffer.find('\n', start);
				if (lineBreak == std::string::npos) {
					break;
				}

				std::string line = buffer.substr(start, lineBreak - start);
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				if (!line.empty()) {
					Logger::getInstance().log(level, category, line, nullptr, 0);
				}
				start = lineBreak + 1;
			}

			if (flushPartial && start < buffer.size()) {
				std::string line = buffer.substr(start);
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				if (!line.empty()) {
					Logger::getInstance().log(level, category, line, nullptr, 0);
				}
				start = buffer.size();
			}

			if (start > 0) {
				buffer.erase(0, start);
			}
		}

		LogLevel level;
		std::string category;
		std::string buffer;
};

Logger& Logger::getInstance()
{
	static Logger instance;
	return instance;
}

Logger::Stream::Stream(LogLevel logLevel, std::string logCategory, const char* sourceFile, int sourceLine) :
    level(logLevel), category(std::move(logCategory)), file(sourceFile), line(sourceLine)
{}

Logger::Stream::Stream(Stream&& other) noexcept :
    level(other.level), category(std::move(other.category)), file(other.file), line(other.line),
    buffer(std::move(other.buffer)), active(other.active)
{
	other.active = false;
}

Logger::Stream::~Stream()
{
	if (!active) {
		return;
	}

	std::string message = buffer.str();
	while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
		message.pop_back();
	}

	if (!message.empty()) {
		Logger::getInstance().log(level, category, message, file, line);
	}
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
	attachStandardStreams();
}

Logger::Stream Logger::stream(LogLevel level, std::string_view category, const char* file, int line) const
{
	return Stream(level, std::string(category), file, line);
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
	loggerConfig.captureStandardStreams = getEnvBool(env, "LOG_CAPTURE_STD_STREAMS", true);
	loggerConfig.consoleCompact = getEnvBool(env, "LOG_CONSOLE_COMPACT", true);
	loggerConfig.consoleShowCategory = getEnvBool(env, "LOG_CONSOLE_SHOW_CATEGORY", false);
	loggerConfig.consoleShowLegacyInfo = getEnvBool(env, "LOG_CONSOLE_SHOW_LEGACY_INFO", false);
	loggerConfig.fileIncludeTimestamp = getEnvBool(env, "LOG_FILE_INCLUDE_TIMESTAMP", true);
	loggerConfig.fileIncludeSource = getEnvBool(env, "LOG_FILE_INCLUDE_SOURCE", true);
	loggerConfig.directory = getEnvString(env, "LOG_DIR", "logs");
	loggerConfig.fileName = getEnvString(env, "LOG_FILE", "logs/server.log");
	loggerConfig.maxFileSizeMb = getEnvNumber(env, "LOG_MAX_FILE_SIZE_MB", 10);
	loggerConfig.maxFiles = getEnvNumber(env, "LOG_MAX_FILES", 5);

	initialize(loggerConfig);
}

void Logger::shutdown()
{
	detachStandardStreams();

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
	if (config.console && config.consoleLevel != LogLevel::Off && message.level >= config.consoleLevel) {
		const std::string consoleMessage = buildConsoleMessage(message);
		if (!consoleMessage.empty()) {
			writeConsoleLine(message.level, consoleMessage);
		}
	}

	if (config.file && config.fileLevel != LogLevel::Off && message.level >= config.fileLevel) {
		if (std::ofstream* output = getFileStream(message.level)) {
			const auto fileName = getFileName(message.level);
			rotateFileIfNeeded(fileName, *output);
			*output << buildFileMessage(message) << std::endl;
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
	writeConsoleLine(LogLevel::Error, "[error] [Logger] File logging disabled: " + reason);
}

void Logger::attachStandardStreams()
{
	std::scoped_lock lock(mutex);
	if (!initialized || standardStreamsAttached || !config.captureStandardStreams) {
		return;
	}

	originalCoutBuffer = std::cout.rdbuf();
	originalCerrBuffer = std::cerr.rdbuf();
	originalClogBuffer = std::clog.rdbuf();

	coutRedirect = std::make_unique<RedirectStreamBuffer>(LogLevel::Info, "StdOut");
	cerrRedirect = std::make_unique<RedirectStreamBuffer>(LogLevel::Error, "StdErr");
	clogRedirect = std::make_unique<RedirectStreamBuffer>(LogLevel::Debug, "StdLog");

	std::cout.rdbuf(coutRedirect.get());
	std::cerr.rdbuf(cerrRedirect.get());
	std::clog.rdbuf(clogRedirect.get());
	standardStreamsAttached = true;
}

void Logger::detachStandardStreams()
{
	{
		std::scoped_lock lock(mutex);
		if (!standardStreamsAttached) {
			return;
		}
	}

	flushRedirectedStreams();

	std::scoped_lock lock(mutex);
	std::cout.rdbuf(originalCoutBuffer);
	std::cerr.rdbuf(originalCerrBuffer);
	std::clog.rdbuf(originalClogBuffer);
	coutRedirect.reset();
	cerrRedirect.reset();
	clogRedirect.reset();
	originalCoutBuffer = nullptr;
	originalCerrBuffer = nullptr;
	originalClogBuffer = nullptr;
	standardStreamsAttached = false;
}

void Logger::flushRedirectedStreams()
{
	std::unique_ptr<RedirectStreamBuffer>* buffers[] = {&coutRedirect, &cerrRedirect, &clogRedirect};
	for (auto* buffer : buffers) {
		if (buffer->get()) {
			(*buffer)->flushPending();
		}
	}
}

void Logger::writeConsoleLine(LogLevel level, const std::string& line)
{
	std::streambuf* target = originalCoutBuffer;
	if (level >= LogLevel::Error && originalCerrBuffer) {
		target = originalCerrBuffer;
	}

	if (!target) {
		target = level >= LogLevel::Error ? std::cerr.rdbuf() : std::cout.rdbuf();
	}

	writeToBuffer(target, line);
}

std::string Logger::buildConsoleMessage(const LogMessage& message) const
{
	if (!config.consoleShowLegacyInfo && message.category == "Legacy" && message.level <= LogLevel::Info) {
		return {};
	}

	if (!config.consoleCompact) {
		return buildFileMessage(message);
	}

	std::ostringstream stream;
	if (message.level == LogLevel::Warn || message.level == LogLevel::Error || message.level == LogLevel::Fatal) {
		stream << '[' << getLevelName(message.level) << "] ";
	}

	if (config.consoleShowCategory && !message.category.empty()) {
		stream << '[' << message.category << "] ";
	}

	stream << message.message;
	return stream.str();
}

std::string Logger::buildFileMessage(const LogMessage& message) const
{
	std::ostringstream stream;
	if (config.fileIncludeTimestamp && !message.timestamp.empty()) {
		stream << '[' << message.timestamp << "] ";
	}

	stream << '[' << getLevelName(message.level) << "] ";
	if (!message.category.empty()) {
		stream << '[' << message.category << "] ";
	}

	stream << message.message;
	if (config.fileIncludeSource && !message.sourceFile.empty()) {
		stream << " (" << message.sourceFile << ':' << message.line << ')';
	}

	return stream.str();
}
