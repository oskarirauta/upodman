#pragma once

#include <deque>
#include <string>

#include "podman_query.hpp"
#include "podman_t.hpp"

namespace Podman {

	class Scheduler {

		public:

			enum CmdType : uint8_t {
				CONTAINER_STOP = 0,
				CONTAINER_START,
				CONTAINER_RESTART,
				POD_STOP,
				POD_START,
				POD_RESTART
			};

			struct Cmd {
				Podman::Scheduler::CmdType type;
				std::string name;
			};

			enum Task : uint8_t {
				UPDATE_CONTAINERS = 0,
				UPDATE_PODS,
				UPDATE_STATS,
				UPDATE_LOGS,
				UPDATE_NETWORKS,
				UPDATE_BUSY_CYCLES,
				__UPDATE_MAX__
			};

		private:

			Podman::podman_t *podman = nullptr;
			int next_task = 0;
			std::deque<Podman::Scheduler::Cmd> cmds;

		public:

			Podman::Scheduler::Task nextTask(void);
			Podman::Scheduler::Cmd *nextCmd(void);

			Scheduler(void);
			Scheduler(Podman::podman_t *podman);

			void run(void);
			void addCmd(Podman::Scheduler::Cmd cmd);
	};

}
