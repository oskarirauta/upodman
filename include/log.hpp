#pragma once

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <list>
#include <map>

#ifndef EVENT_LOG_MAX_SIZE
# define EVENT_LOG_MAX_SIZE 100
#endif

extern bool log_trace;

namespace log {

	using endl_type = std::ostream& (std::ostream&);

	inline std::ostream *output_stream = &std::cout; // set to nullptr to quiet
	inline std::ostream *error_stream = &std::cerr; // set to nullptr to quiet
	inline std::ostream *file_stream = nullptr;

	enum type: uint8_t {
		info = 0,
		error = 1,
		verbose,
		vverbose,
		debug = 254,
		ANY = 255
	};

	inline std::ostream &endl(std::ostream &stream) {

		stream << "\x1B\n";
		return stream;
	}

	inline std::ostream& operator << (std::ostream &os, log::type type) {
		switch ( type ) {
			case log::type::info: return os << "info";
			case log::type::error: return os << "error";
			case log::type::verbose: return os << "verbose";
			case log::type::vverbose: return os << "vverbose";
			case log::type::debug: return os << "debug";
			case log::type::ANY: return os << "ANY";
			default: return os << "unknown";
		}
	}

	inline const std::string description(const log::type type) {
		switch ( type ) {
			case log::type::error: return "error";
			case log::type::debug: return "debug";
			default: return "info";
		}
	}

	inline std::map<log::type, bool> output_level {
		{ static_cast<log::type>(0), true },
		{ static_cast<log::type>(1), true },
	};

	struct entry {

		public:
			log::type type = static_cast<log::type>(0);
			std::chrono::seconds timestamp = std::chrono::duration_cast<std::chrono::seconds>
								(std::chrono::system_clock::now().time_since_epoch());
			std::chrono::seconds timestamp_last = timestamp;
			std::string msg;
			std::string description;
			int count = 0;

			const bool equals(log::entry rhs);
			inline const bool hasDescription() {

				return !this -> description.empty();
			};

	};

	namespace _private { // anonymous namespace / private member(s)

		struct detailTxt {

			public:

				std::string str;

				detailTxt(const std::string str) : str(str) { }
		};

		inline std::list<log::entry> store;
		inline std::map<log::type, std::stringstream> _stream;
		inline std::map<log::type, std::string> _detail;
		inline std::string _last_msg;

		const std::list<log::entry> filtered(void);
		const int lastIndexOf(const log::type type, const std::string msg);
		const bool typeShouldEcho(const log::type type, const bool screenOnly = false);
		void process_entry(const log::type type, const std::string msg, const bool entry_only = false, const std::string detailTxt = "");
		void flush(const log::type type);
		bool endOfEntry(const log::type &f);
	}

	inline const log::_private::detailTxt detail(const std::string s) {

		std::string str = s;
		while ( str.back() == '\n' )
			str.pop_back();
		return log::_private::detailTxt(str);
	}

	template<typename T>
	inline const log::type& operator << ( const log::type &f, const T input) {
		log::_private::_stream[f] << input;
		if ( log::_private::endOfEntry(f))
			log::_private::flush(f);
		return f;
	};

	inline const log::type& operator << ( const log::type &f, const log::endl_type endl) {
		log::_private::_stream[f] << endl;
		if ( log::_private::endOfEntry(f))
			log::_private::flush(f);
		return f;
	};

	inline const log::type& operator << ( const log::type &f, const log::_private::detailTxt detail) {
		log::_private::_detail[f] = detail.str.empty() ? "\x1B" : detail.str;
		return f;
	};

	const std::vector<log::entry> last(int count, const log::type type = log::type::ANY);
};
