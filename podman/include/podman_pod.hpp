#pragma once

#include <string>
#include <vector>
#include <ctime>

#include "podman_container.hpp"
#include "mutex.hpp"

namespace Podman {

	class Pod {

		public:

			struct find_id: std::unary_function<Podman::Pod, bool> {
				std::string id;
				find_id(std::string id):id(id) { }
				bool operator()(Podman::Pod const &m) const {
					return m.id == id;
				}
			};

			struct find_name: std::unary_function<Podman::Pod, bool> {
				std::string name;
				find_name(std::string name):name(name) { }
				bool operator()(Podman::Pod const &m) const {
					return m.name == name;
				}
			};

			std::string id;
			std::string name;
			std::string status;
			std::string infraId;
			bool isRunning;
			std::vector<Podman::Container> containers;

			inline const bool hasInfra(void) {
				std::lock_guard<std::mutex> guard(mutex.podman);
				return this -> infraId.empty() ? false : true;
			}

			inline const int indexOfInfra(void) {
				std::lock_guard<std::mutex> guard(mutex.podman);
				if ( this -> infraId.empty()) return -1;
				for ( int i = 0; i < this -> containers.size(); i++ )
					if ( this -> containers[i].id == this -> infraId )
						return i;
				return -1;
			}

			Pod(const std::string id, const std::string name);

	};

}
