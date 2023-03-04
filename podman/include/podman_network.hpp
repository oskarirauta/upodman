#pragma once

#include <string>
#include <vector>

namespace Podman {

	class Network {

		public:

			struct Subnet {
				std::string gateway;
				std::string subnet;
			};

			struct find_name: std::unary_function<Podman::Network, bool> {
				std::string name;
				find_name(std::string name):name(name) { }
				bool operator()(Podman::Network const &m) const {
					return m.name == name;
				}
			};

			std::string name;
			std::string id;
			std::string interface;
			std::string driver;
			std::string ipam_driver;
			bool ipv6;

			std::vector<Subnet> subnets;

			Network(const std::string id, const std::string name);

	};

}
