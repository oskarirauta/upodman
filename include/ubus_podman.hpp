#pragma once

#include "ubus.hpp"

enum {
        PODMAN_EXEC_ACTION,
        PODMAN_EXEC_GROUP,
        PODMAN_EXEC_NAME,
	PODMAN_EXEC_ID,
        __PODMAN_EXEC_ARGS_MAX
};

enum {
	PODMAN_CONTAINER_NAME,
	PODMAN_CONTAINER_ID,
	__PODMAN_ID_ARGS_MAX
};

const struct blobmsg_policy podman_exec_policy[] = {
	[PODMAN_EXEC_ACTION] = { .name = "action", .type = BLOBMSG_TYPE_STRING },
	[PODMAN_EXEC_GROUP] = { .name = "group", .type = BLOBMSG_TYPE_STRING },
	[PODMAN_EXEC_NAME] = { .name = "name", .type = BLOBMSG_TYPE_STRING },
	[PODMAN_EXEC_ID] = { .name = "id", .type = BLOBMSG_TYPE_STRING },
};

const struct blobmsg_policy podman_id_policy[] = {
	[PODMAN_CONTAINER_NAME] = { .name = "name", .type = BLOBMSG_TYPE_STRING },
	[PODMAN_CONTAINER_ID] = { .name = "id", .type = BLOBMSG_TYPE_STRING },
};

int ubus_func_status(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

int ubus_func_info(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

int ubus_func_networks(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

int ubus_func_list(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg);

int ubus_func_exec(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

int ubus_func_logs(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

int ubus_func_running(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

/*
// template

int ubus_func_x(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);
*/
