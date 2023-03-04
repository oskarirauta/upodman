#include <string>

#include "common.hpp"
#include "mutex.hpp"
#include "podman_t.hpp"
#include "podman_dump.hpp"

std::string Podman::dump::system(Podman::podman_t *podman) {

	std::string dump;

	std::lock_guard<std::mutex> guard(mutex.podman);

	if ( podman -> status != RUNNING ) {

		dump = "status: ";
		dump += podman -> status == UNAVAILABLE ? "unavailable" : "unknown";
		return dump + "\n";
	}

	dump += "status: running\n";
	dump += "arch: " + podman -> arch + "\n";
	dump += "os: " + podman -> os + " " + podman -> os_version + "\n";
	dump += "hostname: " + podman -> hostname + "\n";
	dump += "kernel: " + podman -> kernel + "\n";
	dump += "memory: " + podman -> memFree + "/" + podman -> memTotal + "\n";
	dump += "swap: " + podman -> swapFree + "/" + podman -> swapTotal + "\n";
	dump += "conmon: " + podman -> conmon + "\n";
	dump += "oci runtime: " + podman -> ociRuntime + "\n";
	dump += "podman version: " + podman -> version + "\n";
	dump += "api version: " + podman -> api_version + "\n";

	return dump;
}

std::string Podman::dump::networks(Podman::podman_t *podman) {

	std::lock_guard<std::mutex> guard(mutex.podman);
	if ( podman -> status != RUNNING )
		return "";

	std::string dump;
	int c = 0;

	for ( const auto& network : podman -> networks ) {
		if ( c != 0 ) dump += "\n";
		dump += "#" + std::to_string(c) + "\n";
		dump += "name: " + network.name + "\n";
		dump += "id: " + network.id + "\n";
		dump += "driver: " + network.driver + "\n";
		dump += "interface: " + network.interface + "\n";
		dump += "ipam: " + network.ipam_driver + "\n";
		for ( int i = 0; i < network.subnets.size(); i++ ) {
			dump += "subnet #" + std::to_string(i) + ":\n";
			dump += "\tgateway: " + network.subnets[i].gateway + "\n";
			dump += "\tsubnet: " + network.subnets[i].subnet + "\n";
		}
		c++;
	}

	return dump;
}


std::string Podman::dump::pods(Podman::podman_t *podman) {

	std::lock_guard<std::mutex> guard(mutex.podman);

	if ( podman -> status != RUNNING )
		return "";

	std::string dump;
	int c = 0;

	for ( const auto& pod : podman -> pods ) {
		if ( c!= 0 ) dump += "\n";
		dump += "#" + std::to_string(c) + "\n";
		dump += "name: " + pod.name + "\n";
		dump += "id: " + pod.id + "\n";
		dump += "status: " + pod.status + "\n";
		dump += "infra id: " + pod.infraId + "\n";
		dump += "running: ";
		if ( pod.isRunning ) dump += "true\n";
		else dump += "false\n";
		dump += "hasInfra: ";
		if ( pod.infraId != "" ) dump += "true\n";
		else dump += "false\n";

		dump += "containers: " + std::to_string(pod.containers.size()) + "\n";

		for ( int i = 0; i < pod.containers.size(); i++ ) {
			dump += "\tcontainer #" + std::to_string(i) + ":\n";
			dump += "\t\tname: " + pod.containers[i].name + "\n";
			dump += "\t\tid: " + pod.containers[i].id + "\n";
			dump += "\t\timage: " + pod.containers[i].image + "\n";
			dump += "\t\tcommand: " + pod.containers[i].command + "\n";
			dump += "\t\tpod: " + pod.containers[i].pod + "\n";
			dump += "\t\tinfra: ";
			if ( pod.containers[i].isInfra ) dump += "true\n";
			else dump += "false\n";
			dump += "\t\trunning: ";
			if ( pod.containers[i].isRunning ) dump += "true\n";
			else dump += "false\n";
			dump += "\t\tpid: " + std::to_string(pod.containers[i].pid) + "\n";
			dump += "\t\tstatus: " + pod.containers[i].state + "\n";
			time_t started = pod.containers[i].startedAt;
			dump += "\t\tstarted: " + common::time_str(started) + " (" + std::to_string(started) + ")\n";
			time_t uptime = pod.containers[i].uptime;
			dump += "\t\tuptime: " + common::uptime_str(uptime) + " (" + std::to_string(uptime) + ")\n";
			dump += "\t\tcpu usage: " + pod.containers[i].cpu.text + "\n";
			dump += "\t\tRAM used: " + common::memToStr(pod.containers[i].ram.used, true);
			dump += " (" + common::to_string(pod.containers[i].ram.percent, 1) + "%) ";
			dump += "\tmax: " + common::memToStr(pod.containers[i].ram.max, true) + "\n";
			dump += "\t\tRAM Free: " + common::memToStr(pod.containers[i].ram.free, true) + "\n";
			if ( pod.containers[i].isInfra )
				dump += "\t\tLog file: no logging for infra containers";
			else if ( pod.containers[i].logs.size() == 0 )
				dump += "\t\tLog file: no log entries found";
			else {
				dump += "\t\tLog file:\n";
				for ( int i2 = 0; i2 < pod.containers[i].logs.size(); i2++ ) {
					dump += "\t\t\t" + pod.containers[i].logs[i2] + "\n";
				}
			}
			dump += "\n";
		}
		c++;
	}

	return dump;
}
