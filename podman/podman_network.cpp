#include <string>
#include <vector>
#include <algorithm>

#include "common.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "podman_t.hpp"
#include "podman_network.hpp"

Podman::Network::Network(const std::string id, const std::string name) {

	this -> name = name;
	this -> id = id;
	this -> interface = "";
	this -> driver = "";
	this -> ipam_driver = "";
	this -> ipv6 = false;
	this -> subnets = std::vector<Podman::Network::Subnet>();
}

const bool Podman::podman_t::update_networks(void) {

	if ( !log_trace )
		log::vverbose << "updating networks" << std::endl;

	Podman::Query::Response response;
	Podman::Query query = { .group = "networks", .action = "json" };

	if ( !socket.execute(query, response))
		return false;

	uint64_t hashValue = response.hash();

	// Simple comparison only against names, cni versions, and types. Subnets, ranges, etc.
	// are not retrieved by this query and are not part of comparison.

	mutex.podman.lock();

	if ( hashValue == this -> hash.networks ) {
		mutex.podman.unlock();
		return true;
	}

	mutex.podman.unlock();

	if ( !json_object_is_type(response.json, json_type_array)) {
		log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
		log::vverbose << "error: json result is not array" << std::endl;
		return false;
	}

	int array_size = json_object_array_length(response.json);
	std::vector<Podman::Network> new_networks;

	for ( int i = 0; i < array_size; i++ ) {

		struct json_object *jnet = json_object_array_get_idx(response.json, i);
		std::string name, id;

		if ( struct json_object *jname = json_object_object_get(jnet, "name"); json_object_is_type(jname, json_type_string))
			name = std::string(json_object_get_string(jname));

		if ( struct json_object *jid = json_object_object_get(jnet, "id"); json_object_is_type(jid, json_type_string))
			id = std::string(json_object_get_string(jid));

		if ( name.empty() || id.empty()) {

			if ( name.empty() && id.empty())
				log::debug << "error while parsing networks, name and id objects are missing, or invalid format" << std::endl;
			else if ( name.empty())
				log::debug << "error while parsing networks, name object is missing or invalid format" << std::endl;
			else if ( id.empty())
				log::debug << "error while parsing networks, id object is missing or invalid format" << std::endl;

			continue;
		}

		Podman::Network new_net(id, name);

		if ( struct json_object *jsubnets = json_object_object_get(jnet, "subnets"); json_object_is_type(jsubnets, json_type_array)) {

			std::vector<Podman::Network::Subnet> subnets;
			int subnets_size = json_object_array_length(response.json);

			for ( int i2 = 0; i2 < subnets_size; i2++ ) {

				Podman::Network::Subnet subnet;
				struct json_object *jsubnet = json_object_array_get_idx(jsubnets, i2);

				if ( !json_object_is_type(jsubnet, json_type_object)) {
					log::debug << "error while parsing networks, invalid subnet" << std::endl;
					continue;
				}

				if ( struct json_object *cidr = json_object_object_get(jsubnet, "subnet"); json_object_is_type(cidr, json_type_string))
					subnet.subnet = std::string(json_object_get_string(cidr));

				if ( struct json_object *gw = json_object_object_get(jsubnet, "gateway"); json_object_is_type(gw, json_type_string))
					subnet.gateway = std::string(json_object_get_string(gw));

				if ( subnet.subnet.empty() && subnet.gateway.empty()) {
					log::debug << "error, invalid subnet for network " << new_net.name << std::endl;
					continue;
				}

				subnets.push_back(subnet);
			}

			if ( subnets.size() > 0 )
				new_net.subnets = subnets;
			else log::vverbose << "warning, no subnets found for network " << new_net.name << std::endl;

		} else log::vverbose << "error while parsing network " << new_net.name << " subnets" << std::endl;

		if ( struct json_object *driver = json_object_object_get(jnet, "driver"); json_object_is_type(driver, json_type_string))
			new_net.driver = std::string(json_object_get_string(driver));
		else if ( struct json_object *driver = json_object_object_get(jnet, "type"); json_object_is_type(driver, json_type_string))
			new_net.driver = std::string(json_object_get_string(driver));
		else {
			new_net.driver = "unknown";
			log::vverbose << "warning, unable to parse driver for network " << new_net.name << std::endl;
		}

		if ( struct json_object *ipam = json_object_object_get(jnet, "ipam_options"); json_object_is_type(ipam, json_type_object)) {

			if ( struct json_object *idriver = json_object_object_get(ipam, "driver"); json_object_is_type(idriver, json_type_string))
				new_net.ipam_driver = std::string(json_object_get_string(idriver));
			else if ( struct json_object *idriver = json_object_object_get(ipam, "type"); json_object_is_type(idriver, json_type_string))
				new_net.ipam_driver = std::string(json_object_get_string(idriver));

		} else if ( struct json_object *ipam = json_object_object_get(jnet, "ipam"); json_object_is_type(ipam, json_type_object)) {

			if ( struct json_object *idriver = json_object_object_get(ipam, "driver"); json_object_is_type(idriver, json_type_string))
				new_net.ipam_driver = std::string(json_object_get_string(idriver));
			else if ( struct json_object *idriver = json_object_object_get(ipam, "type"); json_object_is_type(idriver, json_type_string))
				new_net.ipam_driver = std::string(json_object_get_string(idriver));

		}

		if ( new_net.ipam_driver.empty()) {
			new_net.ipam_driver = "unknown";
			log::vverbose << "warning, unable to parse ipam driver for network " << new_net.name << std::endl;
		}

		if ( struct json_object *ifd = json_object_object_get(jnet, "network_interface"); json_object_is_type(ifd, json_type_string))
			new_net.interface = std::string(json_object_get_string(ifd));
		else log::vverbose << "warning, unable to parse network interface for network " << new_net.name << std::endl;

		if ( struct json_object *ip6 = json_object_object_get(jnet, "ipv6_enabled"); json_object_is_type(ip6, json_type_boolean))
			new_net.ipv6 = json_object_get_boolean(ip6);
		else new_net.ipv6 = false;

		new_networks.push_back(new_net);

	}

	if ( new_networks.empty()) {
		log::vverbose << "error: no networks, this can't be right.." << std::endl;
		return false;
	}

	mutex.podman.lock();
	this -> networks = new_networks;
	this -> hash.networks = hashValue;
	this -> state.networks = Podman::Node::OK;
	mutex.podman.unlock();

	return true;
}
