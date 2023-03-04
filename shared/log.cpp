#include <iostream>
#include <algorithm>

#include "common.hpp"
#include "log.hpp"

#ifdef LOG_DUPLICATE_MSG
static const std::string _dup_msg_str = LOG_DUPLICATE_MSG;
#else
static const std::string _dup_msg_str = "(duplicate message atleast once)";
#endif

bool log_trace = false;

const bool log::entry::equals(log::entry rhs) {

	return ( this -> type == log::type::ANY ?
		static_cast<log::type>(0) : this -> type ) == rhs.type &&
		common::to_lower(this -> msg) == common::to_lower(rhs.msg);
}

const std::list<log::entry> log::_private::filtered(void) {

	std::list<log::entry> _filtered;
	for ( auto it = log::_private::store.begin(); it != log::_private::store.end(); ++it ) {
		if ( log::output_level[it -> type])
			_filtered.push_back(*(it));
	}
	return _filtered;
}

const int log::_private::lastIndexOf(const log::type type, const std::string msg) {

	auto idx = std::find_if(
		log::_private::store.rbegin(),
		log::_private::store.rend(),
		[&](log::entry e) { return e.type == type ? true : false; });

	return idx == log::_private::store.rend() ? 0 :
		( common::to_lower(msg) == common::to_lower(idx -> msg) ? std::distance(idx, log::_private::store.rend()) : 0);
}

const bool log::_private::typeShouldEcho(const log::type type, const bool screenOnly) {

	if ( type == static_cast<log::type>(1) && // error type
		log::error_stream != nullptr ) return true;
	else if ( log::output_stream != nullptr || (
		!screenOnly && log::file_stream != nullptr ))
		return true;
	else return false;
}

void log::_private::process_entry(const log::type type, const std::string msg,
		const bool entry_only, const std::string detailTxt) {

	std::string entry_msg = common::trim_ws(msg);

	if ( entry_msg.empty())
		return;

	log::entry _last = log::_private::store.size() > 0 ?
		log::_private::store.back() : log::entry({
			.type = log::type::ANY,
			.msg = "",
	});

	log::entry _entry = {
		.type = type == log::type::ANY ?
			static_cast<log::type>(0) : type,
		.msg = entry_msg,
		.description = detailTxt == "\x1B" ? "" : common::trim_ws(detailTxt),
		.count = 1,
	};

	int _last2_idx = log::_private::lastIndexOf(_entry.type, entry_msg);

	if ( _entry.equals(_last)) {
		_entry.timestamp = _last.timestamp;
		_entry.timestamp_last = ( _entry.timestamp > _last.timestamp ?
			_entry.timestamp : _last.timestamp);
		_entry.count = _last.count + 1;
		_entry.description = _entry.description.empty() && detailTxt != "\x1B" ?
			_last.description : _entry.description;
		_last2_idx = 0;
	}

	std::list<log::entry> filtered_list = log::_private::filtered();
	log::entry _last_filtered = filtered_list.size() > 0 ?
		filtered_list.back() : log::entry({
			.type = log::type::ANY,
			.msg = "",
	});

	if ( _entry.count > 1 )
		log::_private::store.pop_back();

	while ( log::_private::store.size() >= EVENT_LOG_MAX_SIZE + 1 )
		log::_private::store.pop_front();

	if ( !entry_only && log::_private::typeShouldEcho(type) &&
		log::output_level[_entry.type]) {
		bool equals = _entry.equals(_last_filtered);
		if ( !equals || ( equals && log::_private::_last_msg != _dup_msg_str )) {

			std::string _msg = equals ? _dup_msg_str : _entry.msg;
			if ( type != static_cast<log::type>(1) &&
				log::output_stream != nullptr)
				*log::output_stream << _msg << std::endl;
			 else if ( type == static_cast<log::type>(1) &&
				log::error_stream != nullptr )
				*log::error_stream << _msg << std::endl;

			if ( log::file_stream != nullptr )
				*log::file_stream << "[" << _entry.type << "] " << _msg << std::endl;

			log::_private::_last_msg = _msg;
		}
	}

	if ( !_last2_idx )
		log::_private::store.push_back(_entry);
	else if ( auto it = _last2_idx > 1 ?
			std::next(log::_private::store.begin(), _last2_idx - 1) :
			log::_private::store.begin();
		it != log::_private::store.end()) {

		it -> count++;
		it -> description = detailTxt == "\x1B" ? "" : (
			_entry.description.empty() ? it -> description :
				_entry.description);
	}
}

void log::_private::flush(const log::type type) {
	log::_private::_stream[type].str(std::string());
	log::_private::_detail[type].clear();
	log::_private::_stream[type].flush();
}

bool log::_private::endOfEntry(const log::type &f) {

	if ( log::_private::_stream[f].str().find_first_of('\n', 0) !=
		std::string::npos ) {

		bool just_entry = false;
		std::string str, d = log::_private::_detail[f];
		std::getline(log::_private::_stream[f], str);

		if ( str.back() == 0x1B ) {
			just_entry = true;
			str.pop_back();
		}

		while ( d.back() == '\n' )
			d.pop_back();

		log::_private::process_entry(f, str, just_entry, d);
		return true;
	}

	return false;
}

const std::vector<log::entry> log::last(int count, const log::type type) {

	std::vector<log::entry> store;
	if ( log::_private::store.size() == 0 )
		return store;

	while ( log::_private::store.size() >= EVENT_LOG_MAX_SIZE + 1 )
		log::_private::store.pop_front();

	count = count == 0 || log::_private::store.size() < count ?
		log::_private::store.size() : count;

	auto it = count == log::_private::store.size() ?
		log::_private::store.begin() :
			std::next(log::_private::store.begin(),
				log::_private::store.size() - count);

	for ( auto itl = it; itl != log::_private::store.end(); ++itl )
		store.push_back(*itl);

	return store;
}
