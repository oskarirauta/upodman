#include <string>

#include "constants.hpp"
#include "common.hpp"
#include "log.hpp"
#include "podman.hpp"
#include "mutex.hpp"
#include "podman_busystat.hpp"
#include "ubus_podman.hpp"

enum PODMAN_EXEC_ACTION_TYPE {
	PODMAN_EXEC_ACTION_UNKNOWN = 0,
	PODMAN_EXEC_ACTION_START,
	PODMAN_EXEC_ACTION_STOP,
	PODMAN_EXEC_ACTION_RESTART,
};

enum PODMAN_EXEC_GROUP_TYPE {
	PODMAN_EXEC_GROUP_UNKNOWN = 0,
	PODMAN_EXEC_GROUP_POD,
	PODMAN_EXEC_GROUP_CONTAINER,
};

int ubus_func_status(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::status received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);
	blobmsg_add_u8(&b, "running", podman_data -> status == Podman::RUNNING ? true : false);
	blobmsg_add_string(&b, "status", podman_data -> status == Podman::RUNNING ? "running" : ( podman_data -> status == Podman::UNAVAILABLE ? "unavailable" : "unknown" ));
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int ubus_func_info(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::info received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "status", podman_data -> status == Podman::RUNNING ? "running" : ( podman_data -> status == Podman::UNAVAILABLE ? "unavailable" : "unknown"));
	blobmsg_add_string(&b, "arch", podman_data -> arch.c_str());
	blobmsg_add_string(&b, "os", podman_data -> os == "\"openwrt\" linux" ? "openwrt" : podman_data -> os.c_str());
	blobmsg_add_string(&b, "os_version", podman_data -> os_version.c_str());
	blobmsg_add_string(&b, "hostname", podman_data -> hostname.c_str());
	blobmsg_add_string(&b, "kernel", podman_data -> kernel.c_str());

	void *cookie = blobmsg_open_table(&b, "mem");
	blobmsg_add_string(&b, "free", podman_data -> memFree.c_str());
	blobmsg_add_string(&b, "total", podman_data -> memTotal.c_str());
	blobmsg_close_table(&b, cookie);

	void *cookie2 = blobmsg_open_table(&b, "swap");
	blobmsg_add_string(&b, "free", podman_data -> swapFree.c_str());
	blobmsg_add_string(&b, "total", podman_data -> swapTotal.c_str());
	blobmsg_close_table(&b, cookie2);

	blobmsg_add_string(&b, "conmon", podman_data -> conmon.c_str());
	blobmsg_add_string(&b, "ociRuntime", podman_data -> ociRuntime.c_str());
	blobmsg_add_string(&b, "version", podman_data -> version.c_str());
	blobmsg_add_string(&b, "api_version", podman_data -> api_version.c_str());
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int ubus_func_networks(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::networks received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);

	if ( podman_data -> status != Podman::PODMAN_STATUS::RUNNING ) {
		blobmsg_add_string(&b, "message", podman_data -> status == Podman::PODMAN_STATUS::UNAVAILABLE ?
			"Podman unavailable" : "Podman state unknown");
		ubus_send_reply(ctx, req, b.head);
		return 0;
	}

	void *cookie = blobmsg_open_array(&b, "networks");

	for ( const auto& network : podman_data -> networks ) {
		void *cookie2 = blobmsg_open_table(&b, "");
		blobmsg_add_string(&b, "name", network.name.c_str());
		blobmsg_add_string(&b, "driver", network.driver.c_str());

		void *cookie3 = blobmsg_open_table(&b, "ipam");
		blobmsg_add_string(&b, "driver", network.ipam_driver.c_str());

		void *cookie4 = blobmsg_open_array(&b, "subnets");
		for ( const auto& subnet : network.subnets ) {
			void *cookie5 = blobmsg_open_table(&b, "");
			blobmsg_add_string(&b, "gateway", subnet.gateway.c_str());
			blobmsg_add_string(&b, "subnet", subnet.subnet.c_str());
			blobmsg_close_table(&b, cookie5);
		}
		blobmsg_close_array(&b, cookie4);
		blobmsg_close_table(&b, cookie3);
		blobmsg_close_table(&b, cookie2);
	}
	blobmsg_close_array(&b, cookie);

	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int ubus_func_list(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::list received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);

	if ( podman_data -> status != Podman::PODMAN_STATUS::RUNNING ) {
		blobmsg_add_string(&b, "message", podman_data -> status == Podman::PODMAN_STATUS::UNAVAILABLE ?
			"Podman unavailable" : "Podman state unknown");
		ubus_send_reply(ctx, req, b.head);
		return 0;
	}

	void *cookie = blobmsg_open_array(&b, "pods");

	for ( const auto& pod : podman_data -> pods ) {

		int indexOfInfra = -1;

		for ( int i = 0; i < pod.containers.size(); i++ ) {
			if ( pod.containers[i].isInfra ) {
				indexOfInfra = i;
				break;
			}
		}

		void *cookie2 = blobmsg_open_table(&b, "");
		blobmsg_add_string(&b, "name", pod.name.c_str());
		blobmsg_add_string(&b, "id", pod.id.c_str());
		blobmsg_add_string(&b, "status", pod.status.c_str());
		blobmsg_add_string(&b, "infraid", pod.infraId.c_str());
		blobmsg_add_u8(&b, "hasinfra", !pod.infraId.empty());
		blobmsg_add_u8(&b, "running", pod.isRunning);
		blobmsg_add_u16(&b, "num_containers", pod.containers.size());

		void *cookie3 = blobmsg_open_array(&b, "containers");
		for ( const auto& container : pod.containers ) {
			void *cookie4 = blobmsg_open_table(&b, "");
			blobmsg_add_string(&b, "name", container.name.c_str());
			blobmsg_add_string(&b, "image", container.image.c_str());
			blobmsg_add_string(&b, "cmd", container.command.c_str());
			blobmsg_add_u8(&b, "infra", container.isInfra);
			blobmsg_add_string(&b, "pod", container.pod.c_str());
			blobmsg_add_u8(&b, "running", container.isRunning);
			blobmsg_add_u8(&b, "restarting", container.isRestarting);
			blobmsg_add_u32(&b, "pid", container.pid);
			blobmsg_add_string(&b, "state", container.state.c_str());
			blobmsg_add_string(&b, "started", common::time_str(container.startedAt).c_str());

			void *cookie5 = blobmsg_open_table(&b, "busy");
			blobmsg_add_u8(&b, "state", container.busyState.state());
			blobmsg_add_string(&b, "reason", container.busyState.description().c_str());
			blobmsg_close_table(&b, cookie5);

			std::time_t uptime = container.uptime;
			int d = uptime > 86400 ? uptime / 86400 : 0;
			if ( d != 0 ) uptime -= d * 86400;
			int h = uptime > 3600 ? uptime / 3600 : 0;
			if ( h != 0 ) uptime -= h * 3600;
			int m = uptime > 60 ? uptime / 60 : 0;
			if ( m != 0 ) uptime -= m * 60;

			void *cookie6 = blobmsg_open_table(&b, "uptime");
			blobmsg_add_u16(&b, "days", d);
			blobmsg_add_u16(&b, "hours", h);
			blobmsg_add_u16(&b, "minutes", m);
			blobmsg_add_u16(&b, "seconds", (int)uptime);
			blobmsg_close_table(&b, cookie6);

			void *cookie7 = blobmsg_open_table(&b, "cpu");
			blobmsg_add_string(&b, "load", container.cpu.text.c_str());
			blobmsg_add_u16(&b, "percent", (int)container.cpu.percent);
			blobmsg_close_table(&b, cookie7);

			void *cookie8 = blobmsg_open_table(&b, "ram");
			blobmsg_add_string(&b, "used", common::memToStr(container.ram.used, true).c_str());
			blobmsg_add_string(&b, "free", common::memToStr(container.ram.free, true).c_str());
			blobmsg_add_string(&b, "max", common::memToStr(container.ram.max, true).c_str());
			blobmsg_add_u16(&b, "percent", container.ram.percent);
			blobmsg_close_table(&b, cookie8);

			if ( !container.isInfra ) {

				bool canStart = !container.isRunning;
				bool canStop = !canStart;
				bool canRestart = canStop;

				if ( container.busyState.state() ||
					( indexOfInfra != -1 && ( !pod.isRunning || !pod.containers[indexOfInfra].isRunning ))) {
					canStart = false;
					canStop = false;
					canRestart = false;
				}

				void *cookie9 = blobmsg_open_table(&b, "actions");
				blobmsg_add_u8(&b, "start", canStart);
				blobmsg_add_u8(&b, "stop", canStop);
				blobmsg_add_u8(&b, "restart", canRestart);
				blobmsg_close_table(&b, cookie9);
			} else {

				void *cookie9 = blobmsg_open_table(&b, "actions");
				blobmsg_add_u8(&b, "start", false);
				blobmsg_add_u8(&b, "stop", false);
				blobmsg_add_u8(&b, "restart", false);
				blobmsg_close_table(&b, cookie9);
			}

			blobmsg_close_table(&b, cookie4);
		}
		blobmsg_close_array(&b, cookie3);
		blobmsg_close_table(&b, cookie2);
	}

	blobmsg_close_array(&b, cookie);
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int ubus_func_exec(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	struct blob_attr *tb[__PODMAN_EXEC_ARGS_MAX];
	blobmsg_parse(podman_exec_policy, ARRAY_SIZE(podman_exec_policy), tb, blob_data(msg), blob_len(msg));

	PODMAN_EXEC_ACTION_TYPE action = PODMAN_EXEC_ACTION_UNKNOWN;
	PODMAN_EXEC_GROUP_TYPE group = PODMAN_EXEC_GROUP_UNKNOWN;

	std::string _action = "";
	std::string _group = "";

	if ( tb[PODMAN_EXEC_ACTION] ) {
		_action = common::to_lower(std::string((char*)blobmsg_data(tb[PODMAN_EXEC_ACTION])));
		if ( _action == "start" ) action = PODMAN_EXEC_ACTION_START;
		else if ( _action == "stop" ) action = PODMAN_EXEC_ACTION_STOP;
		else if ( _action == "restart" ) action = PODMAN_EXEC_ACTION_RESTART;
	}

	if ( tb[PODMAN_EXEC_GROUP] ) {
		_group = common::to_lower(std::string((char*)blobmsg_data(tb[PODMAN_EXEC_GROUP])));
		if ( _group == "pod" ) group = PODMAN_EXEC_GROUP_POD;
		else if ( _group == "container" ) group = PODMAN_EXEC_GROUP_CONTAINER;
	}

	if ( action == PODMAN_EXEC_ACTION_UNKNOWN || group == PODMAN_EXEC_GROUP_UNKNOWN ) {
		log::debug << APP_NAME << ": ubus call podman::exec received" << std::endl;
		log::vverbose << APP_NAME << ": ubus_podman_exec error, unknown action or group" << std::endl;
		return UBUS_STATUS_INVALID_ARGUMENT;
	}

	std::string name = tb[PODMAN_EXEC_NAME] ?
		std::string((char*)blobmsg_data(tb[PODMAN_EXEC_NAME])) : "";
	std::string id = tb[PODMAN_EXEC_ID] ?
		std::string((char*)blobmsg_data(tb[PODMAN_EXEC_ID])) : "";

	if ( name.empty() && !id.empty())
		name = podman_data -> containerNameForID(id);

	if ( name.empty()) {
		log::debug << APP_NAME << ": ubus call podman::exec received" << std::endl;
		log::vverbose << APP_NAME << ": ubus_podman_exec error, missing name" << std::endl;
		return UBUS_STATUS_INVALID_ARGUMENT;
	}

	// TODO: add support for id instead of name
	// TODO: validate that name/id exists..

	Podman::Scheduler::Cmd cmd;

	if ( group == PODMAN_EXEC_GROUP_POD ) {
		if ( action == PODMAN_EXEC_ACTION_STOP )
			cmd.type = Podman::Scheduler::CmdType::POD_STOP;
		else if ( action == PODMAN_EXEC_ACTION_START )
			cmd.type = Podman::Scheduler::CmdType::POD_START;
		else if ( action == PODMAN_EXEC_ACTION_RESTART )
			cmd.type = Podman::Scheduler::CmdType::POD_RESTART;
	} else if ( group == PODMAN_EXEC_GROUP_CONTAINER ) {
		if ( action == PODMAN_EXEC_ACTION_STOP ) {
			podman_data -> setContainerBusyState(name, Podman::BusyStat::Value::STOPPING);
			cmd.type = Podman::Scheduler::CmdType::CONTAINER_STOP;
		} else if ( action == PODMAN_EXEC_ACTION_START ) {
			podman_data -> setContainerBusyState(name, Podman::BusyStat::Value::STARTING);
			cmd.type = Podman::Scheduler::CmdType::CONTAINER_START;
		} else if ( action == PODMAN_EXEC_ACTION_RESTART ) {
			podman_data -> setContainerBusyState(name, Podman::BusyStat::Value::RESTARTING);
			cmd.type = Podman::Scheduler::CmdType::CONTAINER_RESTART;
		}
	}

	cmd.name = name;
	podman_scheduler -> addCmd(cmd);

	log::debug << APP_NAME << ": ubus call podman::exec::" << _group << "::" << _action << " received" << std::endl;

	blob_buf_init(&b, 0);

	mutex.podman.lock();
	if ( podman_data -> status != Podman::PODMAN_STATUS::RUNNING ) {
		blobmsg_add_string(&b, "message", podman_data -> status == Podman::PODMAN_STATUS::UNAVAILABLE ?
			"Podman unavailable" : "Podman state unknown");
		mutex.podman.unlock();
		ubus_send_reply(ctx, req, b.head);
		return 0;
	}
	mutex.podman.unlock();

	blobmsg_add_string(&b, "action", _action.c_str());
	blobmsg_add_string(&b, "group", _group.c_str());
	blobmsg_add_string(&b, "name", name.c_str());
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int ubus_func_logs(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::logs received" << std::endl;

	struct blob_attr *tb[__PODMAN_ID_ARGS_MAX];
	blobmsg_parse(podman_id_policy, ARRAY_SIZE(podman_id_policy), tb, blob_data(msg), blob_len(msg));

	std::string _id = "";
	std::string _name = "";

	if ( tb[PODMAN_CONTAINER_NAME] )
		_name = std::string((char*)blobmsg_data(tb[PODMAN_CONTAINER_NAME]));

	if ( tb[PODMAN_CONTAINER_ID ] )
		_id = common::to_lower(std::string((char*)blobmsg_data(tb[PODMAN_CONTAINER_ID])));

	if ( _name.empty() && _id.empty()) {
		log::vverbose << APP_NAME << ": ubus_podman_logs error, missing name or id" << std::endl;
		return UBUS_STATUS_INVALID_ARGUMENT;
	}

	std::vector<std::string> logs;
	bool valid = false;

	mutex.podman.lock();

	for ( const auto& pod : podman_data -> pods ) {
		for ( const auto& cntr : pod.containers ) {

			if (( !_name.empty() && cntr.name == _name ) ||
				( !_id.empty() && cntr.id == _id )) {
					logs = cntr.logs;
					valid = true;
					break;
			}
		}

		if ( valid )
			break;
	}

	mutex.podman.unlock();

	if ( !valid ) {
		std::string _identifier = _name.empty() ? ( _id.empty() ? "UNKNOWN" : _id ) : _name;
		log::vverbose << APP_NAME << ": ubus_podman_logs error, id or name " << _identifier << " is not a valid container" << std::endl;
		return UBUS_STATUS_INVALID_ARGUMENT;
	}

	blob_buf_init(&b, 0);
	void *cookie = blobmsg_open_array(&b, "logs");
	for ( const auto& entry : logs )
		blobmsg_add_string(&b, "", entry.c_str());
	blobmsg_close_array(&b, cookie);
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int ubus_func_running(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::running received" << std::endl;

	struct blob_attr *tb[__PODMAN_ID_ARGS_MAX];
	blobmsg_parse(podman_id_policy, ARRAY_SIZE(podman_id_policy), tb, blob_data(msg), blob_len(msg));

	std::string _id = "";
	std::string _name = "";

	if ( tb[PODMAN_CONTAINER_NAME] )
		_name = std::string((char*)blobmsg_data(tb[PODMAN_CONTAINER_NAME]));

	if ( tb[PODMAN_CONTAINER_ID ] )
		_id = common::to_lower(std::string((char*)blobmsg_data(tb[PODMAN_CONTAINER_ID])));

	bool result = false;
	std::string id_type = "none";
	bool valid = false;

	mutex.podman.lock();

	for ( const auto& pod : podman_data -> pods ) {

		if (( !_name.empty() && pod.name == _name ) ||
			( !_id.empty() && pod.id == _id )) {
			result = pod.isRunning;
			id_type = "pod";
			valid = true;
			break;
		}

		for ( const auto& cntr : pod.containers ) {
			if (( !_name.empty() && cntr.name == _name ) ||
				( !_id.empty() && cntr.id == _id )) {
					result = cntr.isRunning;
					id_type = "container";
					valid = true;
					break;
			}
		}

		if ( valid )
			break;
	}

	mutex.podman.unlock();

	blob_buf_init(&b, 0);
	blobmsg_add_u8(&b, "running", result);
	blobmsg_add_u8(&b, "exists", valid);
	blobmsg_add_string(&b, "type", id_type.c_str());
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

/*
// template:

int ubus_func_x(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::func received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);
	blobmsg_add_u8(&b, "bool", true);
	blobmsg_add_string(&b, "string", "string");
	ubus_send_reply(ctx, req, b.head);
	return 0;
}
*/
