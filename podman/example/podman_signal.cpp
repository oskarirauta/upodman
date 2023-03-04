#include "constants.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "podman_loop.hpp"

static void die_handler(int signum) {

	log::info << "podman_example: received TERM signal" << std::endl;
	shouldExit = true;

}

MutexStore mutex(die_handler);
