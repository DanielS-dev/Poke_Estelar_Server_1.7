#ifndef FS_LOGGER_H_40A8E2D8D2D349D8A2E6E0A726F7E1C2
#define FS_LOGGER_H_40A8E2D8D2D349D8A2E6E0A726F7E1C2

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

enum class LogLevel : uint8_t {
	Trace = 0,
	Debug,
	Info,
	Warn,
	Error,
	Fatal,
	Off
};

class Logger
{
	public:
		struct Config {
			LogLevel level = LogLevel::Info;
			LogLevel consoleLevel = LogLevel::Info;
			LogLevel fileLevel = LogLevel::Info;
			bool console = true;
			bool file = true;
			bool splitFilesByLevel = true;
			std::filesystem::path fileName = "logs/server.log";
			std::filesystem::path directory = "logs";
			uint32_t maxFileSizeMb = 10;
			uint32_t maxFiles = 5;
		};

		static Logger& getInstance();

		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		void initialize(const Config& loggerConfig);
		void initializeFromEnv(const std::filesystem::path& fileName);
		void shutdown();

		bool shouldLog(LogLevel level) const;
		void log(LogLevel level, std::string_view category, const std::string& message, const char* file, int line);

	private:
		struct LogMessage {
			LogLevel level = LogLevel::Info;
			std::string category;
			std::string message;
			std::string sourceFile;
			int line = 0;
			std::string timestamp;
		};

		Logger() = default;
		~Logger();

		void threadMain();
		void writeMessage(const LogMessage& message);
		void rotateFileIfNeeded(const std::filesystem::path& fileName, std::ofstream& stream);
		std::ofstream* getFileStream(LogLevel level);
		std::filesystem::path getFileName(LogLevel level) const;
		bool openFileStream(std::ofstream& stream, const std::filesystem::path& fileName);
		void closeStreams();
		void disableFileLogging(const std::string& reason);

		Config config;
		std::ofstream fileStream;
		std::unordered_map<LogLevel, std::ofstream> levelFileStreams;
		std::deque<LogMessage> queue;
		std::thread worker;
		mutable std::mutex mutex;
		std::condition_variable signal;
		bool running = false;
		bool initialized = false;
	};

#define LOG_TRACE(category, message) Logger::getInstance().log(LogLevel::Trace, category, message, __FILE__, __LINE__)
#define LOG_DEBUG(category, message) Logger::getInstance().log(LogLevel::Debug, category, message, __FILE__, __LINE__)
#define LOG_INFO(category, message) Logger::getInstance().log(LogLevel::Info, category, message, __FILE__, __LINE__)
#define LOG_WARN(category, message) Logger::getInstance().log(LogLevel::Warn, category, message, __FILE__, __LINE__)
#define LOG_ERROR(category, message) Logger::getInstance().log(LogLevel::Error, category, message, __FILE__, __LINE__)
#define LOG_FATAL(category, message) Logger::getInstance().log(LogLevel::Fatal, category, message, __FILE__, __LINE__)

#endif
