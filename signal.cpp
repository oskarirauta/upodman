#include "constants.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "ubus.hpp"

static void die_handler(int signum) {

	log::debug << APP_NAME << ": received TERM signal" << std::endl;

	if ( !uloop_cancelled) {
		log::verbose << APP_NAME << ": exiting ubus service" << std::endl;
		uloop_end();
	}
}

MutexStore mutex(die_handler);
