/*
 * Copyright (c) 2025 Sukrit Buddeewong (sukrit.omu@gmail.com)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_AIROC_WIFI_FIRMWARE_STORE_IN_FILE_SYSTEM

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <wiced_resource.h>
#include <wiced_resource_platform.h>
#include "airoc_wiced_file_system.h"

LOG_MODULE_REGISTER(wicedfs, LOG_LEVEL_INF);

wicedfs_handle_t resource_fs_handle = {
	.mount_point = "/lfs1"
};



const char wifi_firmware_image_location[] = CONFIG_AIROC_WIFI_FIRMWARE_BLOB_LOCATION; 
const char wifi_clm_blob_location[] = CONFIG_AIROC_WIFI_CLM_BLOB_LOCATION;
const char wifi_nvram_data_location[] = CONFIG_AIROC_WIFI_NVRAM_BLOB_LOCATION;

const resource_hnd_t wifi_firmware_image = { RESOURCE_IN_FILESYSTEM, (unsigned long)FW_IMAGE_SIZE, {.fs={.filename=(const char *)&wifi_firmware_image_location , .offset=0}}};
const resource_hnd_t wifi_firmware_clm_blob = { RESOURCE_IN_FILESYSTEM, (unsigned long)CLM_IMAGE_SIZE, {.fs={.filename=(const char *)&wifi_clm_blob_location , .offset=0}}};
const resource_hnd_t wifi_nvram_image = { RESOURCE_IN_FILESYSTEM, (unsigned long)NVRAM_IMAGE_SIZE, {.fs={.filename=(const char *)&wifi_nvram_data_location , .offset=0}}};


int wicedfs_fopen(wicedfs_handle_t *fs_handle, wicedfs_file_t *file, const char *filename)
{
	char full_path[256];
	int ret;

	if (!fs_handle || !file || !filename) {
		LOG_ERR("Invalid parameters to wicedfs_fopen");
		return -1;
	}

	/* Build full path: mount_point + filename */
	if (filename[0] == '/') {
		/* Absolute path provided */
		strncpy(full_path, filename, sizeof(full_path) - 1);
	} else {
		/* Relative path - prepend mount point */
		snprintf(full_path, sizeof(full_path), "%s/%s",
			 fs_handle->mount_point, filename);
	}
	full_path[sizeof(full_path) - 1] = '\0';

	/* Initialize Zephyr file structure */
	fs_file_t_init(&file->zephyr_file);

	/* Open file for reading */
	ret = fs_open(&file->zephyr_file, full_path, FS_O_READ);
	if (ret < 0) {
		LOG_ERR("Failed to open file '%s': %d", full_path, ret);
		file->is_open = 0;
		return -1;
	}

	file->is_open = 1;
	LOG_DBG("Opened file: %s", full_path);
	return 0;
}

int wicedfs_fseek(wicedfs_file_t *file, long offset, int whence)
{
	int ret;

	if (!file || !file->is_open) {
		LOG_ERR("Invalid file handle or file not open");
		return -1;
	}

	ret = fs_seek(&file->zephyr_file, offset, whence);
	if (ret < 0) {
		LOG_ERR("Seek failed: %d", ret);
		return -1;
	}

	LOG_DBG("Seek to offset %ld (whence=%d)", offset, whence);
	return 0;
}

uint32_t wicedfs_fread(void *ptr, uint32_t size, uint32_t count, wicedfs_file_t *file)
{
	ssize_t bytes_read;
	uint32_t total_bytes = size * count;

	if (!file || !file->is_open || !ptr) {
		LOG_ERR("Invalid parameters to wicedfs_fread");
		return 0;
	}

	if (total_bytes == 0) {
		return 0;
	}

	bytes_read = fs_read(&file->zephyr_file, ptr, total_bytes);
	if (bytes_read < 0) {
		LOG_ERR("Read failed: %d", (int)bytes_read);
		return 0;
	}

	uint32_t items_read = (uint32_t)bytes_read / size;
	LOG_DBG("Read %u bytes (%u items)", (uint32_t)bytes_read, items_read);
	return items_read;
}

int wicedfs_fclose(wicedfs_file_t *file)
{
	int ret;

	if (!file) {
		LOG_ERR("Invalid file handle");
		return -1;
	}

	if (!file->is_open) {
		LOG_WRN("Attempting to close file that is not open");
		return -1;
	}

	ret = fs_close(&file->zephyr_file);
	file->is_open = 0;

	if (ret < 0) {
		LOG_ERR("Close failed: %d", ret);
		return -1;
	}

	LOG_DBG("File closed successfully");
	return 0;
}

#endif /* CONFIG_AIROC_WIFI_FIRMWARE_STORE_IN_FILE_SYSTEM */
