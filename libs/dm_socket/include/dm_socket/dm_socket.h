#ifndef _DM_SOCKET_H_
#define _DM_SOCKET_H_

#include <stdbool.h>

enum dm_command_type
{
    DM_COMMAND_TYPE_UNKNOWN = -1,
    DM_COMMAND_TYPE_AT,
    DM_COMMAND_TYPE_OTA,
    DM_COMMAND_TYPE_LOG,
};


// command id for DM_COMMAND_TYPE_OTA
enum dm_ota_command
{
    XW_OTA_VERSION = 0,
    XW_OTA_FIRMWARE,
    XW_HOST_ID,
    XW_ANT_ID,
    XW_SEC_ID,
    XW_AP_ID,
    XW_CP_ID,
    XW_OTA_UPDATE_HOST,
    XW_OTA_UPDATE_ANT,
    XW_OTA_UPDATE_SEC,
    XW_OTA_UPDATE_AP,
    XW_OTA_UPDATE_CP,
    XW_OTA_UPDATE_SYNC,
    XW_HOST_OFFLINE,
    XW_OTA_UPDATE_HOST_QUERY,
    XW_OTA_UPDATE_ANT_QUERY,
    XW_OTA_UPDATE_SEC_QUERY,
    XW_OTA_UPDATE_AP_QUERY,
    XW_OTA_UPDATE_CP_QUERY,
    XW_OTA_UPDATE_SYNC_QUERY,
};

// command id for DM_COMMAND_TYPE_LOG
enum dm_log_command
{
    XW_LOG_UPDATE_SET = 0,
    XW_LOG_LVL_SET,
    XW_LOG_UPLOAD_REQ_SET,
    XW_LOG_CLEANUP,
    XW_LOG_UPDATE_GET,
    XW_LOG_LVL_GET,
    XW_LOG_UPLOAD_REQ_GET,
    XW_LOG_DIR_GET,
    XW_LOG_UPLOAD,
    XW_LOG_UPLOAD_REQ,
    XW_LOG_UPLOAD_DONE,
};

typedef struct dm_socket_callbacks_t
{
    void (*at_command_handler)(int command_type, const char* data);
    void (*ota_command_handler)(int command_type, int command_id, const char* args, void* data /* for future use */);
    void (*log_command_handler)(int command_type, int command_id, const char* args, void* data /* for future use */);
    void* reserved[6];
} dm_socket_callbacks;

int dm_socket_init(const char* receive_sock_path, const char* send_sock_path, dm_socket_callbacks* callbacks);
void dm_socket_destroy();

#endif // _DM_SOCKET_H_
