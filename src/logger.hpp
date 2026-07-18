#ifndef FS_LOGGER_H_40A8E2D8D2D349D8A2E6E0A726F7E1C2
#define FS_LOGGER_H_40A8E2D8D2D349D8A2E6E0A726F7E1C2

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <streambuf>
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
		class Stream;

		struct Config {
			LogLevel level = LogLevel::Info;
			LogLevel consoleLevel = LogLevel::Info;
			LogLevel fileLevel = LogLevel::Info;
			bool console = true;
			bool file = true;
			bool splitFilesByLevel = true;
			bool captureStandardStreams = true;
			bool consoleCompact = true;
			bool consoleShowCategory = false;
			bool consoleShowLegacyInfo = false;
			bool fileIncludeTimestamp = true;
			bool fileIncludeSource = true;
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
		[[nodiscard]] Stream stream(LogLevel level, std::string_view category, const char* file, int line) const;

	private:
		struct LogMessage {
			LogLevel level = LogLevel::Info;
			std::string category;
			std::string message;
			std::string sourceFile;
			int line = 0;
			std::string timestamp;
		};

		class RedirectStreamBuffer;

	public:
		class Stream
		{
			public:
				Stream(LogLevel logLevel, std::string logCategory, const char* sourceFile, int sourceLine);
				Stream(const Stream&) = delete;
				Stream& operator=(const Stream&) = delete;
				Stream(Stream&& other) noexcept;
				Stream& operator=(Stream&& other) noexcept = delete;
				~Stream();

				template <typename T>
				Stream& operator<<(const T& value)
				{
					buffer << value;
					return *this;
				}

				Stream& operator<<(std::ostream& (*manipulator)(std::ostream&))
				{
					manipulator(buffer);
					return *this;
				}

				Stream& operator<<(std::ios& (*manipulator)(std::ios&))
				{
					manipulator(buffer);
					return *this;
				}

				Stream& operator<<(std::ios_base& (*manipulator)(std::ios_base&))
				{
					manipulator(buffer);
					return *this;
				}

			private:
				LogLevel level;
				std::string category;
				const char* file;
				int line;
				std::ostringstream buffer;
				bool active = true;
		};

	private:
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
		void attachStandardStreams();
		void detachStandardStreams();
		void flushRedirectedStreams();
		void writeConsoleLine(LogLevel level, const std::string& line);
		std::string buildConsoleMessage(const LogMessage& message) const;
		std::string buildFileMessage(const LogMessage& message) const;

		Config config;
		std::ofstream fileStream;
		std::unordered_map<LogLevel, std::ofstream> levelFileStreams;
		std::deque<LogMessage> queue;
		std::thread worker;
		std::unique_ptr<RedirectStreamBuffer> coutRedirect;
		std::unique_ptr<RedirectStreamBuffer> cerrRedirect;
		std::unique_ptr<RedirectStreamBuffer> clogRedirect;
		std::streambuf* originalCoutBuffer = nullptr;
		std::streambuf* originalCerrBuffer = nullptr;
		std::streambuf* originalClogBuffer = nullptr;
		mutable std::mutex mutex;
		std::condition_variable signal;
		bool running = false;
		bool initialized = false;
		bool standardStreamsAttached = false;
	};

#define LOG_TRACE(category, message) Logger::getInstance().log(LogLevel::Trace, category, message, __FILE__, __LINE__)
#define LOG_DEBUG(category, message) Logger::getInstance().log(LogLevel::Debug, category, message, __FILE__, __LINE__)
#define LOG_INFO(category, message) Logger::getInstance().log(LogLevel::Info, category, message, __FILE__, __LINE__)
#define LOG_WARN(category, message) Logger::getInstance().log(LogLevel::Warn, category, message, __FILE__, __LINE__)
#define LOG_ERROR(category, message) Logger::getInstance().log(LogLevel::Error, category, message, __FILE__, __LINE__)
#define LOG_FATAL(category, message) Logger::getInstance().log(LogLevel::Fatal, category, message, __FILE__, __LINE__)
#define LOG_TRACE_STREAM(category) Logger::getInstance().stream(LogLevel::Trace, category, __FILE__, __LINE__)
#define LOG_DEBUG_STREAM(category) Logger::getInstance().stream(LogLevel::Debug, category, __FILE__, __LINE__)
#define LOG_INFO_STREAM(category) Logger::getInstance().stream(LogLevel::Info, category, __FILE__, __LINE__)
#define LOG_WARN_STREAM(category) Logger::getInstance().stream(LogLevel::Warn, category, __FILE__, __LINE__)
#define LOG_ERROR_STREAM(category) Logger::getInstance().stream(LogLevel::Error, category, __FILE__, __LINE__)
#define LOG_FATAL_STREAM(category) Logger::getInstance().stream(LogLevel::Fatal, category, __FILE__, __LINE__)
#define LOG_STDOUT Logger::getInstance().stream(LogLevel::Info, "Legacy", __FILE__, __LINE__)
#define LOG_STDERR Logger::getInstance().stream(LogLevel::Error, "Legacy", __FILE__, __LINE__)
#define LOG_STDLOG Logger::getInstance().stream(LogLevel::Debug, "Legacy", __FILE__, __LINE__)

#endif
