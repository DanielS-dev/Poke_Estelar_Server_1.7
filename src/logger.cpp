#include "otpch.h"

#include "logger.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <system_error>

namespace {
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
