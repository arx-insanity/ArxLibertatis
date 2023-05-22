/*
 * Copyright 2011-2022 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "io/log/Logger.h"

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "io/log/ConsoleLogger.h"
#include "io/log/LogBackend.h"
#include "io/log/MsvcLogger.h"

#include "platform/ProgramOptions.h"

#include "Configure.h"

namespace {

struct LogManager {
	
	static const Logger::LogLevel defaultLevel;
	static Logger::LogLevel minimumLevel;
	
	static std::mutex mutex;
	
	//! note: using the pointer value of a string constant as a hash map index.
	typedef std::unordered_map<const char *, logger::Source> Sources;
	static Sources sources;
	
	typedef std::vector<logger::Backend *> Backends;
	static Backends backends;
	
	typedef std::unordered_map<std::string, Logger::LogLevel> Rules;
	static Rules rules;
	
	static logger::Source * getSource(const char * file);
	static void deleteAllBackends();
	
};

const Logger::LogLevel LogManager::defaultLevel = Logger::Info;
Logger::LogLevel LogManager::minimumLevel = LogManager::defaultLevel;
LogManager::Sources LogManager::sources;
LogManager::Backends LogManager::backends;
LogManager::Rules LogManager::rules;
std::mutex LogManager::mutex;

logger::Source * LogManager::getSource(const char * file) {
	
	if(auto it = LogManager::sources.find(file); it != sources.end()) {
		return &it->second;
	}
	
	logger::Source * source = &LogManager::sources[file];
	source->file = file;
	source->level = LogManager::defaultLevel;
	
	const char * end = file + strlen(file);
	bool first = true;
	for(const char * p = end; ; p--) {
		if(p == file || *(p - 1) == '/' || *(p - 1) == '\\') {
			
			std::string component(p, end);
			
			if(first) {
				size_t pos = component.find_last_of('.');
				if(pos != std::string::npos) {
					component.resize(pos);
				}
				source->name = component;
				first = false;
			}
			
			LogManager::Rules::const_iterator i = LogManager::rules.find(component);
			if(i != LogManager::rules.end()) {
				source->level = i->second;
				break;
			}
			
			if(p == file || component == "src" || component == "tools") {
				break;
			}
			
			end = p - 1;
		}
	}
	
	return source;
}

void LogManager::deleteAllBackends() {
	
	for(logger::Backend * backend : backends) {
		delete backend;
	}
	
	backends.clear();
	
}

} // anonymous namespace

void Logger::add(logger::Backend * backend) {
	
	std::scoped_lock lock(LogManager::mutex);
	
	if(backend != nullptr) {
		LogManager::backends.push_back(backend);
	}
	
}

void Logger::remove(logger::Backend * backend) {
	
	std::scoped_lock lock(LogManager::mutex);
	
	LogManager::backends.erase(std::remove(LogManager::backends.begin(),
	                                       LogManager::backends.end(),
	                                        backend),
	                           LogManager::backends.end());
	
}

bool Logger::isEnabled(const char * file, LogLevel level) {
	
	if(level < LogManager::minimumLevel) {
		return false;
	}
	
	std::scoped_lock lock(LogManager::mutex);
	
	return (LogManager::getSource(file)->level <= level);
}

void Logger::log(const char * file, int line, LogLevel level, std::string_view str) {
	
	if(level == None) {
		return;
	}
	
	std::scoped_lock lock(LogManager::mutex);
	
	const logger::Source * source = LogManager::getSource(file);
	
	for(logger::Backend * backend : LogManager::backends) {
		backend->log(*source, line, level, str);
	}
	
}

void Logger::set(const std::string & component, Logger::LogLevel level) {
	
	std::scoped_lock lock(LogManager::mutex);
	
	auto ret = LogManager::rules.emplace(component, level);
	
	if(!ret.second) {
		// entry already existed
		
		LogLevel oldLevel = ret.first->second;
		if(level == oldLevel) {
			// nothing changed
			return;
		}
		ret.first->second = level;
		
		if(level > oldLevel && oldLevel < LogManager::defaultLevel) {
			// minimum log level may have changed
			LogManager::minimumLevel = LogManager::defaultLevel;
			for(const auto & entry : LogManager::rules) {
				LogManager::minimumLevel = std::min(LogManager::minimumLevel, entry.second);
			}
		}
		
	}
	
	LogManager::minimumLevel = std::min(LogManager::minimumLevel, level);
	
	LogManager::sources.clear();
}

void Logger::reset(const std::string & component) {
	
	std::scoped_lock lock(LogManager::mutex);
	
	auto it = LogManager::rules.find(component);
	if(it == LogManager::rules.end()) {
		return;
	}
	
	if(it->second < LogManager::defaultLevel) {
		// minimum log level may have changed
		LogManager::minimumLevel = LogManager::defaultLevel;
		for(const auto & entry : LogManager::rules) {
			LogManager::minimumLevel = std::min(LogManager::minimumLevel, entry.second);
		}
	}
	
	LogManager::rules.erase(it);
	LogManager::sources.clear();
	
}

void Logger::flush() {
	
	std::scoped_lock lock(LogManager::mutex);
	
	for(logger::Backend * backend : LogManager::backends) {
		backend->flush();
	}
	
}

void Logger::configure(const std::string & settings) {
	
	size_t start = 0;
	
	while(start < settings.length()) {
		
		size_t pos = settings.find(',', start);
		if(pos == std::string::npos) {
			pos = settings.length();
		}
		if(pos == start) {
			start++;
			continue;
		}
		
		std::string entry = settings.substr(start, pos - start);
		start = pos + 1;
		
		size_t eq = entry.find('=');
		std::string level;
		if(eq != std::string::npos) {
			level = entry.substr(eq + 1), entry.resize(eq);
		}
		
		if(level.empty() || level == "debug" || level == "d" || level == "D") {
			set(entry, Debug);
		} else if(level == "info" || level == "i" || level == "I") {
			set(entry, Info);
		} else if(level == "warning" || level == "warn" || level == "w" || level == "W") {
			set(entry, Warning);
		} else if(level == "error" || level == "e" || level == "E") {
			set(entry, Error);
		} else if(level == "critical" || level == "c" || level == "C") {
			set(entry, Error);
		} else if(level == "none" || level == "n" || level == "N") {
			set(entry, None);
		} else if(level == "reset" || level == "r" || level == "R" || level == "-") {
			reset(entry);
		}
		
	}
	
}

void Logger::initialize() {
	
	add(logger::Console::get());
	
#if ARX_PLATFORM == ARX_PLATFORM_WIN32
	add(logger::MsvcDebugger::get());
#endif
	
	const char * arxdebug = getenv("ARXDEBUG");
	if(arxdebug) {
		configure(arxdebug);
	}
	
}

void Logger::shutdown() {
	
	std::scoped_lock lock(LogManager::mutex);
	
	LogManager::sources.clear();
	LogManager::rules.clear();
	
	LogManager::minimumLevel = LogManager::defaultLevel;
	
	LogManager::deleteAllBackends();
	
}

void Logger::quickShutdown() {
	for(logger::Backend * backend : LogManager::backends) {
		backend->quickShutdown();
	}
}

ARX_PROGRAM_OPTION_ARG("debug", "g", "Log level settings", &Logger::configure, "LEVELS")
