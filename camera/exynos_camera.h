/*
 * Copyright (C) 2013 Paul Kocialkowski
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include <videodev2.h>
#include <videodev2_exynos_media.h>
#include <videodev2_exynos_camera.h>

#include <Exif.h>

#include <hardware/hardware.h>
#include <hardware/camera.h>

#ifndef _EXYNOS_CAMERA_H_
#define _EXYNOS_CAMERA_H_

#define EXYNOS_CAMERA_MAX_PRESETS_COUNT		2
#define EXYNOS_CAMERA_MAX_V4L2_NODES_COUNT	4
#define EXYNOS_CAMERA_MIN_BUFFERS_COUNT		3
#define EXYNOS_CAMERA_MAX_BUFFERS_COUNT		8

#define EXYNOS_CAMERA_MSG_ENABLED(msg) \
	(exynos_camera->messages_enabled & msg)
#define EXYNOS_CAMERA_CALLBACK_DEFINED(cb) \
	(exynos_camera->callbacks.cb != NULL)

/*
 * Structures
 */

enum m5mo_af_status {
	M5MO_AF_STATUS_FAIL = 0,
	M5MO_AF_STATUS_IN_PROGRESS,
	M5MO_AF_STATUS_SUCCESS,
	M5MO_AF_STATUS_1ST_SUCCESS = 4,
};

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

enum exynos_param_type {
	EXYNOS_PARAM_INT,
	EXYNOS_PARAM_FLOAT,
	EXYNOS_PARAM_STRING,
};

union exynos_param_data {
	int integer;
	float floating;
	char *string;
};

struct exynos_param {
	struct list_head list;

	char *key;
	union exynos_param_data data;
	enum exynos_param_type type;
};

struct exynos_camera_params {
	char *preview_size_values;
	char *preview_size;
	char *preview_format_values;
	char *preview_format;
	char *preview_frame_rate_values;
	int preview_frame_rate;
	char *preview_fps_range_values;
	char *preview_fps_range;

	char *picture_size_values;
	char *picture_size;
	char *picture_format_values;
	char *picture_format;
	char *jpeg_thumbnail_size_values;
	int jpeg_thumbnail_width;
	int jpeg_thumbnail_height;
	int jpeg_thumbnail_quality;
	int jpeg_quality;

	int video_snapshot_supported;
	int full_video_snap_supported;

	char *recording_size;
	char *recording_size_values;
	char *recording_format;

	char *focus_mode;
	char *focus_mode_values;
	char *focus_distances;
	char *focus_areas;
	int max_num_focus_areas;
        char *touch_af_aec;

	int zoom_supported;
	int smooth_zoom_supported;
	char *zoom_ratios;
	int zoom;
	int max_zoom;

	char *flash_mode;
	char *flash_mode_values;

	int exposure_compensation;
	float exposure_compensation_step;
	int min_exposure_compensation;
	int max_exposure_compensation;

	char *whitebalance;
	char *whitebalance_values;

	char *scene_mode;
	char *scene_mode_values;

	char *effect;
	char *effect_values;

	char *iso;
	char *iso_values;
};

struct exynos_camera_preset {
	char *name;
	int facing;
	int orientation;

	int rotation;
	int hflip;
	int vflip;

	int picture_format;

	float focal_length;
	float horizontal_view_angle;
	float vertical_view_angle;

	int metering;

	struct exynos_camera_params params;
};

struct exynos_v4l2_node {
	int id;
	char *node;
};

struct exynox_camera_config {
	struct exynos_camera_preset *presets;
	int presets_count;

	struct exynos_v4l2_node *v4l2_nodes;
	int v4l2_nodes_count;
};

struct exynos_camera_callbacks {
	camera_notify_callback notify;
	camera_data_callback data;
	camera_data_timestamp_callback data_timestamp;
	camera_request_memory request_memory;
	void *user;
};

struct exynos_camera {
	int v4l2_fds[EXYNOS_CAMERA_MAX_V4L2_NODES_COUNT];

	struct exynox_camera_config *config;
	struct exynos_param *params;

	struct exynos_camera_callbacks callbacks;
	int messages_enabled;

	gralloc_module_t *gralloc;

	// Picture
	pthread_t picture_thread;
	pthread_mutex_t picture_mutex;
	int picture_thread_running;

	int picture_enabled;
	camera_memory_t *picture_memory;
	int picture_buffer_length;

	// Auto-focus
	pthread_t auto_focus_thread;
	pthread_mutex_t auto_focus_mutex;
	int auto_focus_thread_running;

	int auto_focus_enabled;

	// Preview
	pthread_t preview_thread;
	pthread_mutex_t preview_mutex;
	pthread_mutex_t preview_lock_mutex;
	int preview_thread_running;
	int preview_thread_started;
	int preview_waiting_for_window;

	int preview_enabled;
	struct preview_stream_ops *preview_window;
	camera_memory_t *preview_memory;
	int preview_buffers_count;
	int preview_frame_size;
	int preview_params_set;

	// Recording
	pthread_mutex_t recording_mutex;

	int recording_enabled;
	int recording_msg_start;
	int recording_msg_start_result;
	int recording_msg_stop;
	camera_memory_t *recording_memory;
	int recording_buffers_count;

	// Camera params
	int camera_rotation;
	int camera_hflip;
	int camera_vflip;
	int camera_picture_format;
	int camera_focal_length;
	int camera_metering;

	int camera_sensor_mode;

	int preview_requested_width;
	int preview_requested_height;
	int touch_focus_blocked;

	// Params
	int preview_width;
	int preview_height;
	int preview_format;
	float preview_format_bpp;
	int preview_fps;
	int picture_width;
	int picture_height;
	int picture_format;
	int jpeg_thumbnail_width;
	int jpeg_thumbnail_height;
	int jpeg_thumbnail_quality;
	int jpeg_quality;
	int recording_width;
	int recording_height;
	int recording_format;
	int focus_mode;
	int focus_x;
	int focus_y;
	int zoom;
	int flash_mode;
	int exposure_compensation;
	int whitebalance;
	int scene_mode;
	int effect;
	int iso;
	int metering;
};

struct exynos_camera_addrs {
	unsigned int type;
	unsigned int y;
	unsigned int cbcr;
	unsigned int index;
	unsigned int reserved;
};

// This is because the linux header uses anonymous union
struct exynos_v4l2_ext_control {
	__u32 id;
	__u32 size;
	__u32 reserved2[1];
	union {
		__s32 value;
		__s64 value64;
		char *string;
	} data;
} __attribute__ ((packed));

/*
 * Camera
 */

int exynos_camera_params_init(struct exynos_camera *exynos_camera, int id);
int exynos_camera_params_apply(struct exynos_camera *exynos_camera, bool doInit);
int exynos_camera_params_get_focus_mode(struct exynos_camera *exynos_camera);

int exynos_camera_auto_focus_start(struct exynos_camera *exynos_camera);
void exynos_camera_auto_focus_stop(struct exynos_camera *exynos_camera);

int exynos_camera_picture(struct exynos_camera *exynos_camera);
int exynos_camera_picture_start(struct exynos_camera *exynos_camera);

int exynos_camera_preview(struct exynos_camera *exynos_camera);
int exynos_camera_preview_start(struct exynos_camera *exynos_camera);
void exynos_camera_preview_stop(struct exynos_camera *exynos_camera);

int exynos_camera_recording_start(struct exynos_camera *exynos_camera);
void exynos_camera_recording_stop(struct exynos_camera *exynos_camera);

/*
 * EXIF
 */

int exynos_exif_attributes_create_static(struct exynos_camera *exynos_camera,
	exif_attribute_t *exif_attributes);
int exynos_exif_attributes_create_params(struct exynos_camera *exynos_camera,
	exif_attribute_t *exif_attributes);

int exynos_exif_create(struct exynos_camera *exynos_camera,
	exif_attribute_t *exif_attributes,
	camera_memory_t *jpeg_thumbnail_data_memory, int jpeg_thumbnail_size,
	camera_memory_t **exif_data_memory_p, int *exif_size_p);

/*
 * Param
 */

int exynos_param_int_get(struct exynos_camera *exynos_camera,
	char *key);
float exynos_param_float_get(struct exynos_camera *exynos_camera,
	char *key);
char *exynos_param_string_get(struct exynos_camera *exynos_camera,
	char *key);

int exynos_param_int_set(struct exynos_camera *exynos_camera,
	char *key, int integer);
int exynos_param_float_set(struct exynos_camera *exynos_camera,
	char *key, float floating);
int exynos_param_string_set(struct exynos_camera *exynos_camera,
	char *key, char *string);

char *exynos_params_string_get(struct exynos_camera *exynos_camera);
int exynos_params_string_set(struct exynos_camera *exynos_camera, char *string);

/*
 * V4L2
 */

// Utils
int exynos_v4l2_find_index(struct exynos_camera *exynos_camera, int exynos_v4l2_id);
int exynos_v4l2_find_fd(struct exynos_camera *exynos_camera, int exynos_v4l2_id);

// File ops
int exynos_v4l2_open(struct exynos_camera *exynos_camera, int id);
void exynos_v4l2_close(struct exynos_camera *exynos_camera, int id);
int exynos_v4l2_ioctl(struct exynos_camera *exynos_camera, int id, int request, void *data);
int exynos_v4l2_poll(struct exynos_camera *exynos_camera, int exynos_v4l2_id);

// VIDIOC
int exynos_v4l2_qbuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int memory, int index);
int exynos_v4l2_qbuf_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int index);
int exynos_v4l2_qbuf_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int index);
int exynos_v4l2_dqbuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int memory);
int exynos_v4l2_dqbuf_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id);
int exynos_v4l2_dqbuf_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id);
int exynos_v4l2_reqbufs(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int memory, int count);
int exynos_v4l2_reqbufs_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int count);
int exynos_v4l2_reqbufs_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int count);
int exynos_v4l2_querybuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int memory, int index);
int exynos_v4l2_querybuf_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int index);
int exynos_v4l2_querybuf_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int index);
int exynos_v4l2_querycap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int flags);
int exynos_v4l2_querycap_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id);
int exynos_v4l2_querycap_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id);
int exynos_v4l2_streamon(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type);
int exynos_v4l2_streamon_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id);
int exynos_v4l2_streamon_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id);
int exynos_v4l2_streamoff(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type);
int exynos_v4l2_streamoff_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id);
int exynos_v4l2_streamoff_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id);
int exynos_v4l2_g_fmt(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int *width, int *height, int *fmt);
int exynos_v4l2_g_fmt_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int *width, int *height, int *fmt);
int exynos_v4l2_g_fmt_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int *width, int *height, int *fmt);
int exynos_v4l2_s_fmt_pix(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int width, int height, int fmt, int priv);
int exynos_v4l2_s_fmt_pix_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int width, int height, int fmt, int priv);
int exynos_v4l2_s_fmt_pix_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int width, int height, int fmt, int priv);
int exynos_v4l2_s_fmt_win(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int left, int top, int width, int height);
int exynos_v4l2_enum_fmt(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int fmt);
int exynos_v4l2_enum_fmt_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int fmt);
int exynos_v4l2_enum_fmt_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int fmt);
int exynos_v4l2_enum_input(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int id);
int exynos_v4l2_s_input(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int id);
int exynos_v4l2_g_ext_ctrls(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	struct v4l2_ext_control *control, int count);
int exynos_v4l2_g_ctrl(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int id, int *value);
int exynos_v4l2_s_ctrl(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int id, int value);
int exynos_v4l2_s_parm(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, struct v4l2_streamparm *streamparm);
int exynos_v4l2_s_parm_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	struct v4l2_streamparm *streamparm);
int exynos_v4l2_s_parm_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	struct v4l2_streamparm *streamparm);
int exynos_v4l2_s_crop(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int left, int top, int width, int height);
int exynos_v4l2_s_crop_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int left, int top, int width, int height);
int exynos_v4l2_s_crop_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int left, int top, int width, int height);
int exynos_v4l2_g_fbuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	void **base, int *width, int *height, int *fmt);
int exynos_v4l2_s_fbuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	void *base, int width, int height, int fmt);

#endif
