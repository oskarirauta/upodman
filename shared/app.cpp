#include <iostream>
#include <vector>
#include <thread>

#include "podman_constants.hpp"
#include "constants.hpp"
#include "log.hpp"
#include "loop.hpp"
#include "ubus.hpp"
#include "app.hpp"

void version_info(void) {
	std::cout << APP_NAME << " version " << APP_VERSION;
	std::cout << "\n" << "Written by Oskari Rauta" << std::endl;
}

void usage(char* cmd) {
	std::cout << "\nusage: " << cmd << " [parameters]\n\n";
	std::cout << "   -h, --h             show this help\n";
	std::cout << "   --version           show version\n";
	std::cout << "   -s, --socket path   connect to specified ubus socket\n";
	std::cout << "   -l v4.0.0           force libpod version\n";
	std::cout << "   --interval ms       set interval for cpu load refreshing(default: " << DEFAULT_DELAY << "ms)\n";
	std::cout << "   -q, --quiet         silence output\n";
	std::cout << "   --only-errors       output only errors\n";
	std::cout << "   -v, --verbose       verbose output\n";
	std::cout << "   -vv                 more verbose output\n";
	std::cout << "   --debug             maximum verbose output\n";
	std::cout << "   --trace             debug level log output with json result output and socket queries\n";
	std::cout << std::endl;
}

void parse_cmdline(int argc, char **argv) {

	std::vector<std::string> args(argv + 1, argv + argc);
	int log_level = 0;

	for (auto i = args.begin(); i != args.end(); i++) {
		if ( *i == "--help" || *i == "--h" || *i == "-h" || *i == "-?" ) {
			version_info();
			usage(argv[0]);
			exit(0);
		} else if (( *i == "-v" || *i == "--verbose" ) && log_level < 2 ) {
			if ( log_level < 0 ) {
				std::cout << "error: quiet and verbose logging cannot be used at same time." << std::endl;
				exit(-1);
			}
			log_level = 2;
		} else if (( *i == "-vv" || *i == "--vv" ) && log_level < 3 ) {
			if ( log_level < 0 ) {
				std::cout << "error: quiet and (extra) verbose logging cannot be used at same time." << std::endl;
				exit(-1);
			}
			log_level = 3;
		} else if (( *i == "-debug" || *i == "--debug" ) && log_level < 4 ) {
			if ( log_level < 0 ) {
				std::cout << "error: quiet and debug logging cannot be used at same time." << std::endl;
				exit(-1);
			}
			log_level = 4;
		} else if (( *i == "-trace" || *i == "--trace" ) && log_level < 5 ) {
			if ( log_level < 0 ) {
				std::cout << "error: quiet and debug/trace logging cannot be used at same time." << std::endl;
				exit(-1);
			}
			log_level = 5;
		} else if ( *i == "--only-errors" && log_level < 1 ) {
			if ( log_level < 0 ) {
				std::cout << "error: quiet and error-only logging cannot be used at same time." << std::endl;
				exit(-1);
			}
			log_level = 1;
		} else if ( *i == "-q" || *i == "--q" || *i == "-quiet" || *i == "--quiet" ) {
			if ( log_level > 0 ) {
				std::cout << "error: quiet and other logging levels cannot be used at same time." << std::endl;
				exit(-1);
			}
			log_level = -1;
		} else if ( *i == "-s" || *i == "--s" || *i == "--sock" || *i == "--socket" || *i == "-socket" ) {
			std::string this_arg = *i;
			if ( std::next(i) != args.end())
				ubus_socket = *++i;
			else {
				std::cout << "error: socket's path not given for " << this_arg << " option." << std::endl;
				exit(-1);
			}

			if ( ubus_socket.size() > 1 && ubus_socket.front() == '"' && ubus_socket.back() == '"' ) {
				ubus_socket.erase(0, 1);
				ubus_socket.erase(ubus_socket.size() - 1);
			}

			if ( ubus_socket.empty()) {
				std::cout << "error: socket's path not given for " << this_arg << " option." << std::endl;
				exit(-1);
			}

		} else if ( *i == "-l" || *i == "--l" || *i == "--libpod" || *i == "--libpod-version" ) {
			std::string this_arg = *i;
			std::string _version;
			if ( std::next(i) != args.end())
				_version = *++i;
			else {
				std::cout << "error: libpod override version not defined for " << this_arg << " option." << std::endl;
				exit(-1);
			}

			if ( _version.size() > 1 && _version.front() == '"' && _version.back() == '"' ) {
				_version.erase(0, 1);
				_version.erase(_version.size() - 1);
			}

			libpod_version_override = _version;

		} else if ( *i == "-interval" || *i == "--interval" ) {
			std::string this_arg = *i;
			if ( std::next(i) != args.end()) {
				std::string str = *++i;
				for ( char c : str ) if ( !isdigit(c)) {
					std::cout << "error: " << this_arg << " requires numeric parameter" << std::endl;
					exit(-1);
				}
				int value = 0;
				try {
					value = std::stoi(str, nullptr);
				} catch (...) {
					std::cout << "error: " << this_arg << " requires numeric parameter" << std::endl;
					exit(-1);
				}
				if ( value < 500 || value > 1800 ) {
					std::cout << "error: " << this_arg << " requires value between 500 and 1800 (seconds)." << std::endl;
					exit(-1);
				}
				main_loop.set_delay(value);
			} else {
				std::cout << "error: " << *i << " requires value between 500 and 1800 (seconds)." << std::endl;
				exit(-1);
			}
		} else if ( *i == "-version" || *i == "--version" ) {
			version_info();
			exit(0);
		} else if ( *i != "" ) {
			std::cout << "Unknown argument: " << *i << std::endl;
			usage(argv[0]);
			exit(-1);
		}
	}

	if ( log_level == -1 ) {
		log::output_stream = nullptr;
		log::error_stream = nullptr;
	} else if ( log_level == 1 )
		log::output_stream = nullptr;
	else {

		if ( log_level > 1 )
			log::output_level[log::verbose] = true;

		if ( log_level > 2 )
			log::output_level[log::vverbose] = true;

		if ( log_level > 3 )
			log::output_level[log::debug] = true;

		if ( log_level > 4 )
			log_trace = true;
	}

}
