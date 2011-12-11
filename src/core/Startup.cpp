/*
 * Copyright 2011 Arx Libertatis Team (see the AUTHORS file)
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

#include "core/Startup.h"

#include <stddef.h>
#include <string>
#include <vector>
#include <iostream>

#include <boost/program_options.hpp>

#include "core/Config.h"
#include "io/fs/FilePath.h"
#include "io/fs/Filesystem.h"
#include "io/log/Logger.h"
#include "platform/Environment.h"

#include "Configure.h"

namespace po = boost::program_options;

using std::string;

static fs::path findSubdirectory(const std::string & where, const fs::path & dir,
                                 fs::path * to_create = NULL) {
	
	string prefixes = expandEvironmentVariables(where);
	
	bool create_exists = false;
	
	size_t start = 0;
	while(true) {
		size_t end = prefixes.find(':', start);
		fs::path prefix = prefixes.substr(start, (end == string::npos) ? end : (end - start));
		
		fs::path subdir = prefix / dir;
		if(fs::is_directory(subdir)) {
			return subdir;
		}
		
		if(to_create) {
			if(to_create->empty() || (!create_exists && fs::is_directory(prefix))) {
				*to_create = subdir;
				create_exists = fs::is_directory(prefix);
			}
		}
		
		if(end == string::npos) {
			break;
		} else {
			start = end + 1;
		}
	}
	
	return fs::path();
}

static void findDataDirectory() {
	
	config.paths.data.clear();
	
	string temp;
	if(getSystemConfiguration("DataDir", temp)) {
		config.paths.data = temp;
		LogDebug("Got data directory from registry: " << config.paths.data);
		return;
	}
	
#ifdef DATA_DIR
	
	fs::path dir = expandEvironmentVariables(DATA_DIR);
	
#ifdef DATA_DIR_PREFIXES 
	if(dir.is_relative()) {
		config.paths.data = findSubdirectory(DATA_DIR_PREFIXES, dir);
		if(!config.paths.data.empty()) {
			LogDebug("Got data directory from DATA_DIR_PREFIXES: " << config.paths.data);
			return;
		}
	}
#endif // DATA_DIR_PREFIXES
	
	if(fs::is_directory(dir)) {
		config.paths.data = dir;
		LogDebug("Got data directory from DATA_DIR: " << config.paths.data);
		return;
	}
	
#endif // DATA_DIR
	
	LogDebug("No data directory found.");
}

static void findUserDirectory() {
	
	config.paths.user.clear();
	
	string temp;
	if(getSystemConfiguration("UserDir", temp)) {
		config.paths.user = temp;
		LogDebug("Got user directory from registry: " << config.paths.user);
		return;
	}
	
#ifdef USER_DIR
	
	fs::path dir = expandEvironmentVariables(USER_DIR);
	
	fs::path to_create;
#ifdef USER_DIR_PREFIXES
	if(dir.is_relative()) {
		config.paths.user = findSubdirectory(USER_DIR_PREFIXES, dir, &to_create);
		if(!config.paths.user.empty()) {
			LogDebug("Got user directory from USER_DIR_PREFIXES: " << config.paths.user);
			return;
		}
	}
#endif // USER_DIR_PREFIXES
	
	if(fs::is_directory(dir)) {
		config.paths.user = dir;
		LogDebug("Got user directory from USER_DIR: " << config.paths.user);
		return;
	}
	
	// Create a new user directory.
	if(!config.paths.data.empty()) {
		if(!to_create.empty()) {
			config.paths.user = to_create;
			LogDebug("Selected new user directory from USER_DIR_PREFIXES: " << config.paths.user);
		} else {
			config.paths.user = dir;
			LogDebug("Selected new user directory from USER_DIR: " << config.paths.user);
		}
		return;
	}
	
#endif // USER_DIR
	
	// Use the current directory for both data and config files.
	config.paths.user = ".";
	LogDebug("Using working directory as user directory: " << config.paths.user);
}

static void createUserDirectory() {
	
	if(config.paths.user.empty()) {
		LogError << "No user config directory available.";
		exit(1);
	}
	
	if(!fs::is_directory(config.paths.user)) {
		if(!fs::create_directories(config.paths.user)) {
			LogError << "Error creating user config directory at " << config.paths.user;
			exit(1);
		}
		LogInfo << "Created new user config directory at " << config.paths.user;
	}
	
	if(config.paths.data == config.paths.user) {
		config.paths.data.clear();
	}
}

#if ARX_PLATFORM != ARX_PLATFORM_WIN32
void parseCommandLine(int argc, const char * const * argv) {
#else
void parseCommandLine(const char * command_line) {
#endif
	
	po::options_description options_desc("Arx Libertatis Options");
	options_desc.add_options()
		("help,h", "Show supported options.")
		("no-data-dir,n", "Don't automatically detect a data directory.")
		("data-dir,d", po::value<string>(), "Where to find the data files."
#ifdef DATA_DIR
		  "\n Default: \"" DATA_DIR "\""
#ifdef DATA_DIR_PREFIXES
			" in \"" DATA_DIR_PREFIXES "\""
#endif
#endif
		)
		("user-dir,u", po::value<string>(), "Where to store config and save files."
#ifdef USER_DIR
		  "\n Default: \"" USER_DIR "\""
#ifdef USER_DIR_PREFIXES
			" in \"" USER_DIR_PREFIXES "\""
#endif
#endif
		)
		("debug,g", po::value<string>(), "Log level settings.")
	;
	
	po::variables_map options;
	
	try {
		
#if ARX_PLATFORM != ARX_PLATFORM_WIN32
		po::store(po::parse_command_line(argc, argv, options_desc), options);
#else
		std::vector<string> args = po::split_winmain(command_line);
		po::store(po::command_line_parser(args).options(options_desc).run(), options);
#endif
		
		po::notify(options);
		
		if(options.count("help")) {
			std::cout << options_desc << std::endl;
			exit(0);
		}
		
		po::variables_map::const_iterator debug = options.find("debug");
		if(debug != options.end()) {
			Logger::configure(debug->second.as<string>());
		}
		
		defineXdgDirectories();
		
		po::variables_map::const_iterator data_dir = options.find("data-dir");
		if(data_dir != options.end()) {
			config.paths.data = data_dir->second.as<string>();
			LogDebug("Got data directory from command-line: " << config.paths.data);
		} else if(options.count("no-data-dir")) {
			LogDebug("Disabled data directory.");
		} else {
			findDataDirectory();
		}
		if(!config.paths.data.empty() && !fs::is_directory(config.paths.data)) {
			LogWarning << "Data directory " << config.paths.data << " does not exist.";
			config.paths.data.clear();
		}
		
		po::variables_map::const_iterator user_dir = options.find("user-dir");
		if(user_dir != options.end()) {
			config.paths.user = user_dir->second.as<string>();
			LogDebug("Got user directory from command-line: " << config.paths.user);
		} else {
			findUserDirectory();
		}
		createUserDirectory();
		
	} catch(po::error & e) {
		std::cerr << "Error parsing command-line: " << e.what() << "\n\n";
		std::cout << options_desc << std::endl;
		exit(1);
	}
}
