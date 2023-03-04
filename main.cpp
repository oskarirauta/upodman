#include <iostream>
#include <string>
#include <thread>

#include "log.hpp"
#include "podman.hpp"
#include "mutex.hpp"
#include "ubus.hpp"
#include "app.hpp"
#include "loop.hpp"

void freedata(void) {

	free(podman_scheduler);
	free(podman_data);
}

int main(int argc, char **argv) {

	parse_cmdline(argc, argv);

	podman_data = new Podman::podman_t(Podman::init);
	podman_scheduler = new Podman::Scheduler(podman_data);

	uloop_init();
	ctx = ubus_connect(ubus_socket == "" ? NULL : ubus_socket.c_str());

	if ( !ctx ) {
		log::error << APP_NAME << ": failed to connect to ubus socket " << ubus_socket << std::endl;
		freedata();
		return -1;
	}

	ubus_add_uloop(ctx);

	if ( log::output_level[log::verbose] == true )
		std::cout << APP_NAME << " version " << APP_VERSION << std::endl;

	if ( int ret = ubus_create(); ret != 0 ) {
		ubus_free(ctx);
		freedata();
		return(ret);
	}

	log::verbose << APP_NAME << ": starting main loop" << std::endl;
	std::thread loop_thread(run_main_loop);


	log::info << APP_NAME << ": service started" << std::endl;
	log::verbose << APP_NAME << ": starting ubus service" << std::endl;

	uloop_run();

	uloop_done();
	ubus_free(ctx);

	log::verbose << APP_NAME << ": ubus service has stopped" << std::endl;
	log::vverbose << APP_NAME << ": exiting main loop" << std::endl;

	main_loop.set_sig_exit(true);

	while ( main_loop.running())
		std::this_thread::sleep_for(
			std::chrono::milliseconds(SIG_DELAY));

	log::vverbose << APP_NAME << ": main loop stopped" << std::endl;

	loop_thread.join();
	freedata();

	log::verbose << APP_NAME << ": ended service" << std::endl;

	return 0;
}
