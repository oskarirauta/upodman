#pragma once

#include <string>
#include <vector>
#include <json-c/json.h>

#include "podman_constants.hpp"
#include "podman_socket.hpp"
#include "podman_query.hpp"
#include "podman_network.hpp"
#include "podman_pod.hpp"
#include "podman_node.hpp"

namespace Podman {

	enum PODMAN_STATUS : uint8_t {
		UNAVAILABLE = 0,
		RUNNING,
		UNKNOWN
	};

	class podman_t {

		private:
			bool needsSystemUpdate;

		public:

			struct Hashes {
				uint64_t networks = 0;
				uint64_t pods = 0;
				uint64_t containers = 0;
			};

			struct States {
				Podman::Node::State networks = Podman::Node::INCOMPLETE;
				Podman::Node::State pods = Podman::Node::INCOMPLETE;
				Podman::Node::State containers = Podman::Node::INCOMPLETE;
				Podman::Node::State stats = Podman::Node::INCOMPLETE;
			};

			Podman::Socket socket;
			PODMAN_STATUS status = UNKNOWN;

			std::string arch;
			std::string os, os_version;
			std::string hostname;
			std::string kernel;
			std::string memTotal, memFree;
			std::string swapTotal, swapFree;
			std::string conmon, ociRuntime;
			std::string version, api_version;

			podman_t(std::string socket_path = Podman::API_SOCKET);
			podman_t(void (*creator_func)(Podman::podman_t*), std::string socket_path = Podman::API_SOCKET);
			//~podman_t();

			std::vector<Podman::Network> networks;
			std::vector<Podman::Pod> pods;

			Podman::podman_t::Hashes hash;
			Podman::podman_t::States state;

			/* --- Calls --- */

			// System
			const bool update_system(void);

			// Network
			const bool update_networks(void);

			// Pods
			const int podIndex(const std::string name);

			// Containers
			const bool update_containers(void);
			const bool update_pods(void);
			const bool update_stats(void);
			const bool update_logs(void);

			// Global
			const bool update_busy_cycles(void);

			// Commands: containers
			const bool container_stop(const std::string name);
			const bool container_start(const std::string name);
			const bool container_restart(const std::string name);

			// Commands: pods
			const bool pod_stop(const std::string name);
			const bool pod_start(const std::string name);
			const bool pod_restart(const std::string name);

			// Helpers
			const std::string containerNameForID(const std::string id);
			const bool setContainerBusyState(const std::string name, Podman::BusyStat::Value state);
			const bool resetContainerBusyState(const std::string name);

			// Parsers
			const bool parse_cntr(struct json_object *json, Podman::Container &cntr);
			const bool parse_stats(struct json_object *json, Podman::Container &cntr);

	};

}

