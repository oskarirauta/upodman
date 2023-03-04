#include <iostream>
#include <thread>
#include <algorithm>
#include <string>

#include "constants.hpp"
#include "log.hpp"
#include "ubus_podman.hpp"
#include "ubus.hpp"

std::string ubus_socket = "";
struct ubus_context *ctx;
#ifndef b
struct blob_buf b;
#endif

static const struct ubus_method podman_methods[] = {
	{ .name = "status", .handler = ubus_func_status },
	{ .name = "info", .handler = ubus_func_info },
	{ .name = "networks", .handler = ubus_func_networks },
	{ .name = "list", .handler = ubus_func_list },
	{ .name = "exec", .handler = ubus_func_exec, .policy = podman_exec_policy, .n_policy = ARRAY_SIZE(podman_exec_policy) },
	{ .name = "logs", .handler = ubus_func_logs, .policy = podman_id_policy, .n_policy = ARRAY_SIZE(podman_id_policy) },
	{ .name = "running", .handler = ubus_func_running, .policy = podman_id_policy, .n_policy = ARRAY_SIZE(podman_id_policy) }
};

static struct ubus_object_type podman_object_type = {
	.name = "podman",
	.id = 0,
	.methods = podman_methods,
	.n_methods = ARRAY_SIZE(podman_methods),
};

static struct ubus_object podman_object = {
	.name = "podman",
	.type = &podman_object_type,
	.methods = podman_methods,
	.n_methods = ARRAY_SIZE(podman_methods),
};

int ubus_create(void) {

	log::debug << APP_NAME << ": creating ubus objects" << std::endl;

	if ( int ret = ubus_add_object(ctx, &podman_object); ret != 0 ) {
		log::error << APP_NAME << ": failed to add podman object to ubus" << std::endl;
		log::error << "error: " << ubus_strerror(ret) << std::endl;
		return ret;
	}

	return 0;
}
