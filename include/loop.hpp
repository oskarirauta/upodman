#pragma once

#include <mutex>
#include "constants.hpp"

struct Loop {

	private:
		bool _sig_exit = false;
		bool _running = false;
		int _delay = DEFAULT_DELAY;
		std::mutex sig_mutex;

		void sleep(int ms);

	public:
		bool sig_exit(void);
		bool running(void);
		int delay(void);

		void set_sig_exit(bool state);
		void set_running(bool state);
		void set_delay(int delay);

		void run(void);
};

extern Loop main_loop;

void run_main_loop(void);
