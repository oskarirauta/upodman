#include <thread>

#include "podman.hpp"
#include "mutex.hpp"
#include "ubus.hpp"
#include "log.hpp"
#include "loop.hpp"

bool Loop::sig_exit(void) {

	std::lock_guard<std::mutex> guard(this -> sig_mutex);
	return this -> _sig_exit;
}

bool Loop::running(void) {

	std::lock_guard<std::mutex> guard(this -> sig_mutex);
	return this -> _running;
}

int Loop::delay(void) {

	std::lock_guard<std::mutex> guard(this -> sig_mutex);
	return this -> _delay;
}

void Loop::set_sig_exit(bool state) {

	std::lock_guard<std::mutex> guard(this -> sig_mutex);
	this -> _sig_exit = state;
}

void Loop::set_running(bool state) {

	std::lock_guard<std::mutex> guard(this -> sig_mutex);
	this -> _running = state;
}

void Loop::set_delay(int delay) {

	std::lock_guard<std::mutex> guard(this -> sig_mutex);
	this -> _delay = delay;
}

void Loop::sleep(int ms) {

	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void Loop::run(void) {

	if ( this -> running())
		return;

	bool minimalSleep = false;

	this -> set_running(true);
	int __delay = this -> delay();

	this -> sleep((int)(__delay * 0.5));

	while ( !this -> sig_exit()) {

		if ( podman_data -> status != Podman::PODMAN_STATUS::RUNNING ) {

			log::debug << "connection to podman socket unverified." << std::endl;
			log::vverbose << "attempting to connect to podman socket" << std::endl;

			Podman::init(podman_data);
			this -> sleep(__delay * 2);
			continue;
		}

		podman_scheduler -> run();
		this -> sleep(__delay);
	}

	this -> set_running(false);

}

Loop main_loop;

void run_main_loop(void) {
	main_loop.run();
}
