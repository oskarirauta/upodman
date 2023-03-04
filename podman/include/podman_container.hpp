#pragma once

#ifndef CONTAINER_LOG_SIZE
#define CONTAINER_LOG_SIZE 25
#endif

#include <string>
#include <vector>

#include "podman_busystat.hpp"
#include "podman_socket.hpp"

namespace Podman {

	class Container {

		public:

			struct MemoryStats {
				double used = 0, max = 0, free = 0, percent = 0;
			};

			struct CpuStats {
				double percent = 0;
				std::string text = "--";
			};

			struct find_id: std::unary_function<Podman::Container, bool> {
				std::string id;
				find_id(std::string id):id(id) { }
				bool operator()(Podman::Container const &m) const {
					return m.id == id;
				}
			};

			struct find_name: std::unary_function<Podman::Container, bool> {
				std::string name;
				find_name(std::string name):name(name) { }
				bool operator()(Podman::Container const &m) const {
					return m.name == name;
				}
			};

			std::string id;
			std::string name;
			std::string image;
			std::string command;
			std::string pod;
			std::string podName;
			bool isInfra = false;
			bool isRunning = false;
			bool isRestarting = false;
			pid_t pid;
			std::string state;
			std::time_t startedAt;
			std::time_t uptime;
			std::vector<pid_t> pids;

			std::vector<std::string> logs;
			Podman::Container::CpuStats cpu;
			Podman::Container::MemoryStats ram;
			Podman::BusyStat busyState = Podman::BusyStat::Value::NONE;

			Container(std::string name, std::string id = "");

			void copy_details(Podman::Container *other);
			bool update_logs(Podman::Socket *socket);
	};

}
