/*
 * Copyright (C) 2013 Paul Kocialkowski
 *
 * Based on crespo libcamera and exynos4 hal libcamera:
 * Copyright 2008, The Android Open Source Project
 * Copyright 2010, Samsung Electronics Co. LTD
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

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <Exif.h>

#define LOG_TAG "exynos_camera"
#include <utils/Log.h>
#include <cutils/properties.h>

#include "exynos_camera.h"

/*
 * FIXME: This EXIF implementation doesn't work very well, it needs to be fixed.
 */

int exynos_exif_attributes_create_static(struct exynos_camera *exynos_camera,
	exif_attribute_t *exif_attributes)
{
	unsigned char gps_version[] = { 0x02, 0x02, 0x00, 0x00 };
	char property[PROPERTY_VALUE_MAX];
	uint32_t av;

	if (exynos_camera == NULL || exif_attributes == NULL)
		return -EINVAL;

	// Device
	property_get("ro.product.brand", property, EXIF_DEF_MAKER);
	strncpy((char *) exif_attributes->maker, property,
		sizeof(exif_attributes->maker) - 1);
	exif_attributes->maker[sizeof(exif_attributes->maker) - 1] = '\0';

	property_get("ro.product.model", property, EXIF_DEF_MODEL);
	strncpy((char *) exif_attributes->model, property,
		sizeof(exif_attributes->model) - 1);
	exif_attributes->model[sizeof(exif_attributes->model) - 1] = '\0';

	property_get("ro.build.id", property, EXIF_DEF_SOFTWARE);
	strncpy((char *) exif_attributes->software, property,
		sizeof(exif_attributes->software) - 1);
	exif_attributes->software[sizeof(exif_attributes->software) - 1] = '\0';

	exif_attributes->ycbcr_positioning = EXIF_DEF_YCBCR_POSITIONING;

	exif_attributes->fnumber.num = EXIF_DEF_FNUMBER_NUM;
	exif_attributes->fnumber.den = EXIF_DEF_FNUMBER_DEN;

	exif_attributes->exposure_program = EXIF_DEF_EXPOSURE_PROGRAM;

	memcpy(exif_attributes->exif_version, EXIF_DEF_EXIF_VERSION,
		sizeof(exif_attributes->exif_version));

	av = APEX_FNUM_TO_APERTURE((double) exif_attributes->fnumber.num /
		exif_attributes->fnumber.den);
	exif_attributes->aperture.num = av;
	exif_attributes->aperture.den = EXIF_DEF_APEX_DEN;
	exif_attributes->max_aperture.num = av;
	exif_attributes->max_aperture.den = EXIF_DEF_APEX_DEN;

	strcpy((char *) exif_attributes->user_comment, EXIF_DEF_USERCOMMENTS);
	exif_attributes->color_space = EXIF_DEF_COLOR_SPACE;
	exif_attributes->exposure_mode = EXIF_DEF_EXPOSURE_MODE;

	// GPS version
	memcpy(exif_attributes->gps_version_id, gps_version, sizeof(gps_version));

	exif_attributes->compression_scheme = EXIF_DEF_COMPRESSION;
	exif_attributes->x_resolution.num = EXIF_DEF_RESOLUTION_NUM;
	exif_attributes->x_resolution.den = EXIF_DEF_RESOLUTION_DEN;
	exif_attributes->y_resolution.num = EXIF_DEF_RESOLUTION_NUM;
	exif_attributes->y_resolution.den = EXIF_DEF_RESOLUTION_DEN;
	exif_attributes->resolution_unit = EXIF_DEF_RESOLUTION_UNIT;

	return 0;
}

int exynos_exif_attributes_create_gps(struct exynos_camera *exynos_camera,
	exif_attribute_t *exif_attributes)
{
	float gps_latitude_float, gps_longitude_float, gps_altitude_float;
	int gps_timestamp_int;
	char *gps_processing_method_string;
	long gps_latitude, gps_longitude;
	long gps_altitude, gps_timestamp;
	double gps_latitude_abs, gps_longitude_abs, gps_altitude_abs;

	struct tm time_info;

	if (exynos_camera == NULL || exif_attributes == NULL)
		return -EINVAL;

	gps_latitude_float = exynos_param_float_get(exynos_camera, "gps-latitude");
	gps_longitude_float = exynos_param_float_get(exynos_camera, "gps-longitude");
	gps_altitude_float = exynos_param_float_get(exynos_camera, "gps-altitude");
	if (gps_altitude_float == -1)
		gps_altitude_float = (float) exynos_param_int_get(exynos_camera, "gps-altitude");
	gps_timestamp_int = exynos_param_int_get(exynos_camera, "gps-timestamp");
	gps_processing_method_string = exynos_param_string_get(exynos_camera, "gps-processing-method");

	if (gps_latitude_float == -1 || gps_longitude_float == -1 ||
		gps_altitude_float == -1 || gps_timestamp_int <= 0 ||
		gps_processing_method_string == NULL) {
		exif_attributes->enableGps = false;
		return 0;
	}

	gps_latitude = (long) (gps_latitude_float * 10000000) / 1;
	gps_longitude = (long) (gps_longitude_float * 10000000) / 1;
	gps_altitude = (long) (gps_altitude_float * 100) / 1;
	gps_timestamp = (long) gps_timestamp_int;

	if (gps_latitude == 0 || gps_longitude == 0) {
		exif_attributes->enableGps = false;
		return 0;
	}

	if (gps_latitude > 0)
		strcpy((char *) exif_attributes->gps_latitude_ref, "N");
	else
		strcpy((char *) exif_attributes->gps_latitude_ref, "S");

	if (gps_longitude > 0)
		strcpy((char *) exif_attributes->gps_longitude_ref, "E");
	else
		strcpy((char *) exif_attributes->gps_longitude_ref, "W");

	if (gps_altitude > 0)
		exif_attributes->gps_altitude_ref = 0;
	else
		exif_attributes->gps_altitude_ref = 1;


	gps_latitude_abs = fabs(gps_latitude);
	gps_longitude_abs = fabs(gps_longitude);
	gps_altitude_abs = fabs(gps_altitude);

	exif_attributes->gps_latitude[0].num = (uint32_t) gps_latitude_abs;
	exif_attributes->gps_latitude[0].den = 10000000;
	exif_attributes->gps_latitude[1].num = 0;
	exif_attributes->gps_latitude[1].den = 1;
	exif_attributes->gps_latitude[2].num = 0;
	exif_attributes->gps_latitude[2].den = 1;

	exif_attributes->gps_longitude[0].num = (uint32_t) gps_longitude_abs;
	exif_attributes->gps_longitude[0].den = 10000000;
	exif_attributes->gps_longitude[1].num = 0;
	exif_attributes->gps_longitude[1].den = 1;
	exif_attributes->gps_longitude[2].num = 0;
	exif_attributes->gps_longitude[2].den = 1;

	exif_attributes->gps_altitude.num = (uint32_t) gps_altitude_abs;
	exif_attributes->gps_altitude.den = 100;

	gmtime_r(&gps_timestamp, &time_info);

	exif_attributes->gps_timestamp[0].num = time_info.tm_hour;
	exif_attributes->gps_timestamp[0].den = 1;
	exif_attributes->gps_timestamp[1].num = time_info.tm_min;
	exif_attributes->gps_timestamp[1].den = 1;
	exif_attributes->gps_timestamp[2].num = time_info.tm_sec;
	exif_attributes->gps_timestamp[2].den = 1;
	snprintf((char *) exif_attributes->gps_datestamp, sizeof(exif_attributes->gps_datestamp),
		"%04d:%02d:%02d", time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday);

	exif_attributes->enableGps = true;

	return 0;
}

int exynos_exif_attributes_create_params(struct exynos_camera *exynos_camera,
	exif_attribute_t *exif_attributes)
{
	uint32_t av, tv, bv, sv, ev;
	time_t time_data;
	struct tm *time_info;
	int rotation;
	int shutter_speed;
	int exposure_time;
	int iso_speed;
	int exposure;
	int flash_results;

	int rc;

	if (exynos_camera == NULL || exif_attributes == NULL)
		return -EINVAL;

	// Picture size
	exif_attributes->width = exynos_camera->picture_width;
	exif_attributes->height = exynos_camera->picture_height;

	// Thumbnail
	exif_attributes->widthThumb = exynos_camera->jpeg_thumbnail_width;
	exif_attributes->heightThumb = exynos_camera->jpeg_thumbnail_height;
	exif_attributes->enableThumb = true;

	// Orientation
	rotation = exynos_param_int_get(exynos_camera, "rotation");
	switch (rotation) {
		case 90:
			exif_attributes->orientation = EXIF_ORIENTATION_90;
			break;
		case 180:
			exif_attributes->orientation = EXIF_ORIENTATION_180;
			break;
		case 270:
			exif_attributes->orientation = EXIF_ORIENTATION_270;
			break;
		case 0:
		default:
			exif_attributes->orientation = EXIF_ORIENTATION_UP;
			break;
	}

	// Time
	time(&time_data);
	time_info = localtime(&time_data);
	strftime((char *) exif_attributes->date_time, sizeof(exif_attributes->date_time),
		"%Y:%m:%d %H:%M:%S", time_info);

	exif_attributes->focal_length.num = exynos_camera->camera_focal_length;
	exif_attributes->focal_length.den = EXIF_DEF_FOCAL_LEN_DEN;

	shutter_speed = 100;
	rc = exynos_v4l2_g_ctrl(exynos_camera, 0, V4L2_CID_CAMERA_EXIF_TV,
		&shutter_speed);
	if (rc < 0)
		ALOGE("%s: g ctrl failed!", __func__);

	exif_attributes->shutter_speed.num = shutter_speed;
	exif_attributes->shutter_speed.den = 100;

	/* the exposure_time value read from the camera doesn't work
	 * exposure_time = shutter_speed;
	 * rc = exynos_v4l2_g_ctrl(exynos_camera, 0, V4L2_CID_CAMERA_EXIF_EXPTIME,
	 * 	&exposure_time);
	 * if (rc < 0)
	 *	ALOGE("%s: g ctrl failed!", __func__);
	 */

	// calculate exposure time from the shutter speed value instead
	exif_attributes->exposure_time.num = 1;
	exif_attributes->exposure_time.den = APEX_SHUTTER_TO_EXPOSURE(shutter_speed);

	rc = exynos_v4l2_g_ctrl(exynos_camera, 0, V4L2_CID_CAMERA_EXIF_ISO,
		&iso_speed);
	if (rc < 0)
		ALOGE("%s: g ctrl failed!", __func__);

	exif_attributes->iso_speed_rating = iso_speed;

	rc = exynos_v4l2_g_ctrl(exynos_camera, 0, V4L2_CID_CAMERA_EXIF_FLASH,
		&flash_results);
	if (rc < 0)
		ALOGE("%s: g ctrl failed!", __func__);

	exif_attributes->flash = flash_results;

	rc = exynos_v4l2_g_ctrl(exynos_camera, 0, V4L2_CID_CAMERA_EXIF_BV,
		(int *) &bv);
	if (rc < 0) {
		ALOGE("%s: g ctrl failed!", __func__);
		goto bv_static;
	}

	rc = exynos_v4l2_g_ctrl(exynos_camera, 0, V4L2_CID_CAMERA_EXIF_EBV,
		(int *) &ev);
	if (rc < 0) {
		ALOGE("%s: g ctrl failed!", __func__);
		goto bv_static;
	}

	goto bv_ioctl;

bv_static:
	exposure = exynos_param_int_get(exynos_camera, "exposure-compensation");
	if (exposure < 0)
		exposure = EV_DEFAULT;

	av = APEX_FNUM_TO_APERTURE((double) exif_attributes->fnumber.num /
		exif_attributes->fnumber.den);
	tv = APEX_EXPOSURE_TO_SHUTTER((double) exif_attributes->exposure_time.num /
		exif_attributes->exposure_time.den);
	sv = APEX_ISO_TO_FILMSENSITIVITY(iso_speed);
	bv = av + tv - sv;
	ev = exposure - EV_DEFAULT;

bv_ioctl:
	exif_attributes->brightness.num = bv;
	exif_attributes->brightness.den = EXIF_DEF_APEX_DEN;

	if (exynos_camera->scene_mode == SCENE_MODE_BEACH_SNOW) {
		exif_attributes->exposure_bias.num = EXIF_DEF_APEX_DEN;
		exif_attributes->exposure_bias.den = EXIF_DEF_APEX_DEN;
	} else {
		exif_attributes->exposure_bias.num = ev * EXIF_DEF_APEX_DEN;
		exif_attributes->exposure_bias.den = EXIF_DEF_APEX_DEN;
	}

	switch (exynos_camera->camera_metering) {
		case METERING_CENTER:
			exif_attributes->metering_mode = EXIF_METERING_CENTER;
			break;
		case METERING_MATRIX:
			exif_attributes->metering_mode = EXIF_METERING_AVERAGE;
			break;
		case METERING_SPOT:
			exif_attributes->metering_mode = EXIF_METERING_SPOT;
			break;
		default:
			exif_attributes->metering_mode = EXIF_METERING_AVERAGE;
			break;
	}

	if (exynos_camera->whitebalance == WHITE_BALANCE_AUTO ||
		exynos_camera->whitebalance == WHITE_BALANCE_BASE)
		exif_attributes->white_balance = EXIF_WB_AUTO;
	else
		exif_attributes->white_balance = EXIF_WB_MANUAL;

	switch (exynos_camera->scene_mode) {
		case SCENE_MODE_PORTRAIT:
			exif_attributes->scene_capture_type = EXIF_SCENE_PORTRAIT;
			break;
		case SCENE_MODE_LANDSCAPE:
			exif_attributes->scene_capture_type = EXIF_SCENE_LANDSCAPE;
			break;
		case SCENE_MODE_NIGHTSHOT:
			exif_attributes->scene_capture_type = EXIF_SCENE_NIGHT;
			break;
		default:
			exif_attributes->scene_capture_type = EXIF_SCENE_STANDARD;
			break;
	}

	rc = exynos_exif_attributes_create_gps(exynos_camera, exif_attributes);
	if (rc < 0) {
		ALOGE("%s: Failed to create GPS attributes", __func__);
		return -1;
	}

	return 0;
}

int exynos_exif_write_data(void *exif_data, unsigned short tag,
	unsigned short type, unsigned int count, unsigned int *offset, void *start,
	void *data, int length)
{
	unsigned char *pointer;
	int size;

	if (exif_data == NULL || data == NULL || length <= 0)
		return -EINVAL;

	pointer = (unsigned char *) exif_data;

	memcpy(pointer, &tag, sizeof(tag));
	pointer += sizeof(tag);

	memcpy(pointer, &type, sizeof(type));
	pointer += sizeof(type);

	memcpy(pointer, &count, sizeof(count));
	pointer += sizeof(count);

	if (offset != NULL && start != NULL) {
		memcpy(pointer, offset, sizeof(*offset));
		pointer += sizeof(*offset);

		memcpy((void *) ((int) start + *offset), data, count * length);
		*offset += count * length;
	} else {
		memcpy(pointer, data, count * length);
		pointer += 4;
	}

	size = (int) pointer - (int) exif_data;
	return size;
}

int exynos_exif_create(struct exynos_camera *exynos_camera,
	exif_attribute_t *exif_attributes,
	camera_memory_t *jpeg_thumbnail_data_memory, int jpeg_thumbnail_size,
	camera_memory_t **exif_data_memory_p, int *exif_size_p)
{
	// Markers
	unsigned char exif_app1_marker[] = { 0xff, 0xe1 };
	unsigned char exif_app1_size[] = { 0x00, 0x00 };
	unsigned char exif_marker[] = { 0x45, 0x78, 0x69, 0x66, 0x00, 0x00 };
	unsigned char tiff_marker[] = { 0x49, 0x49, 0x2A, 0x00, 0x08, 0x00, 0x00, 0x00 };

	unsigned char user_comment_code[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };
	unsigned char exif_ascii_prefix[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };

	camera_memory_t *exif_data_memory = NULL;
	void *exif_data;
	int exif_data_size;
	int exif_size;

	void *exif_ifd_data_start, *exif_ifd_start, *exif_ifd_thumb, *exif_ifd_gps = NULL;

	void *exif_thumb_data;
	unsigned int exif_thumb_size;

	unsigned char *pointer;
	unsigned int offset;
	void *data;
	int count;

	unsigned int value;

	if (exynos_camera == NULL || exif_attributes == NULL ||
		jpeg_thumbnail_data_memory == NULL || jpeg_thumbnail_size <= 0 ||
		exif_data_memory_p == NULL || exif_size_p == NULL)
		return -EINVAL;

	exif_data_size = EXIF_FILE_SIZE + jpeg_thumbnail_size;

	if (exynos_camera->callbacks.request_memory != NULL) {
		exif_data_memory = exynos_camera->callbacks.request_memory(-1,
			exif_data_size, 1, exynos_camera->callbacks.user);
		if (exif_data_memory == NULL) {
			ALOGE("%s: exif memory request failed!", __func__);
			goto error;
		}
	} else {
		ALOGE("%s: No memory request function!", __func__);
		goto error;
	}

	exif_data = exif_data_memory->data;
	memset(exif_data, 0, exif_data_size);

	pointer = (unsigned char *) exif_data;
	exif_ifd_data_start = (void *) pointer;

	// Skip 4 bytes for APP1 marker
	pointer += 4;

	// Copy EXIF marker
	memcpy(pointer, exif_marker, sizeof(exif_marker));
	pointer += sizeof(exif_marker);

	// Copy TIFF marker
	memcpy(pointer, tiff_marker, sizeof(tiff_marker));
	exif_ifd_start = (void *) pointer;
	pointer += sizeof(tiff_marker);

	if (exif_attributes->enableGps)
		value = NUM_0TH_IFD_TIFF;
	else
		value = NUM_0TH_IFD_TIFF - 1;

	memcpy(pointer, &value, NUM_SIZE);
	pointer += NUM_SIZE;

	offset = 8 + NUM_SIZE + value * IFD_SIZE + OFFSET_SIZE;

	// Write EXIF data
	count = exynos_exif_write_data(pointer, EXIF_TAG_IMAGE_WIDTH,
		EXIF_TYPE_LONG, 1, NULL, NULL, &exif_attributes->width, sizeof(exif_attributes->width));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_IMAGE_HEIGHT,
		EXIF_TYPE_LONG, 1, NULL, NULL, &exif_attributes->height, sizeof(exif_attributes->height));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_MAKE,
		EXIF_TYPE_ASCII, strlen((char *) exif_attributes->maker) + 1,
		&offset, exif_ifd_start, &exif_attributes->maker, sizeof(char));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_MODEL,
		EXIF_TYPE_ASCII, strlen((char *) exif_attributes->model) + 1,
		&offset, exif_ifd_start, &exif_attributes->model, sizeof(char));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_ORIENTATION,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->orientation, sizeof(exif_attributes->orientation));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_SOFTWARE,
		EXIF_TYPE_ASCII, strlen((char *) exif_attributes->software) + 1,
		&offset, exif_ifd_start, &exif_attributes->software, sizeof(char));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_DATE_TIME,
		EXIF_TYPE_ASCII, 20, &offset, exif_ifd_start, &exif_attributes->date_time, sizeof(char));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_YCBCR_POSITIONING,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->ycbcr_positioning, sizeof(exif_attributes->ycbcr_positioning));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_EXIF_IFD_POINTER,
		EXIF_TYPE_LONG, 1, NULL, NULL, &offset, sizeof(offset));
	pointer += count;

	if (exif_attributes->enableGps) {
		exif_ifd_gps = (void *) pointer;
		pointer += IFD_SIZE;
	}

	exif_ifd_thumb = (void *) pointer;
	pointer += OFFSET_SIZE;

	pointer = (unsigned char *) exif_ifd_start;
	pointer += offset;

	value = NUM_0TH_IFD_EXIF;
	memcpy(pointer, &value, NUM_SIZE);
	pointer += NUM_SIZE;

	offset += NUM_SIZE + NUM_0TH_IFD_EXIF * IFD_SIZE + OFFSET_SIZE;

	count = exynos_exif_write_data(pointer, EXIF_TAG_EXPOSURE_TIME,
		EXIF_TYPE_RATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->exposure_time, sizeof(exif_attributes->exposure_time));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_FNUMBER,
		EXIF_TYPE_RATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->fnumber, sizeof(exif_attributes->fnumber));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_EXPOSURE_PROGRAM,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->exposure_program, sizeof(exif_attributes->exposure_program));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_ISO_SPEED_RATING,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->iso_speed_rating, sizeof(exif_attributes->iso_speed_rating));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_EXIF_VERSION,
		EXIF_TYPE_UNDEFINED, 4, NULL, NULL, &exif_attributes->exif_version, sizeof(char));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_DATE_TIME_ORG,
		EXIF_TYPE_ASCII, 20, &offset, exif_ifd_start, &exif_attributes->date_time, sizeof(char));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_DATE_TIME_DIGITIZE,
		EXIF_TYPE_ASCII, 20, &offset, exif_ifd_start, &exif_attributes->date_time, sizeof(char));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_SHUTTER_SPEED,
		EXIF_TYPE_SRATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->shutter_speed, sizeof(exif_attributes->shutter_speed));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_APERTURE,
		EXIF_TYPE_RATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->aperture, sizeof(exif_attributes->aperture));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_BRIGHTNESS,
		EXIF_TYPE_SRATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->brightness, sizeof(exif_attributes->brightness));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_EXPOSURE_BIAS,
		EXIF_TYPE_SRATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->exposure_bias, sizeof(exif_attributes->exposure_bias));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_MAX_APERTURE,
		EXIF_TYPE_RATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->max_aperture, sizeof(exif_attributes->max_aperture));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_METERING_MODE,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->metering_mode, sizeof(exif_attributes->metering_mode));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_FLASH,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->flash, sizeof(exif_attributes->flash));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_FOCAL_LENGTH,
		EXIF_TYPE_RATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->focal_length, sizeof(exif_attributes->focal_length));
	pointer += count;

	value = strlen((char *) exif_attributes->user_comment) + 1;
	memmove(exif_attributes->user_comment + sizeof(user_comment_code), exif_attributes->user_comment, value);
	memcpy(exif_attributes->user_comment, user_comment_code, sizeof(user_comment_code));

	count = exynos_exif_write_data(pointer, EXIF_TAG_USER_COMMENT,
		EXIF_TYPE_UNDEFINED, value + sizeof(user_comment_code), &offset, exif_ifd_start, &exif_attributes->user_comment, sizeof(char));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_COLOR_SPACE,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->color_space, sizeof(exif_attributes->color_space));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_PIXEL_X_DIMENSION,
		EXIF_TYPE_LONG, 1, NULL, NULL, &exif_attributes->width, sizeof(exif_attributes->width));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_PIXEL_Y_DIMENSION,
		EXIF_TYPE_LONG, 1, NULL, NULL, &exif_attributes->height, sizeof(exif_attributes->height));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_EXPOSURE_MODE,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->exposure_mode, sizeof(exif_attributes->exposure_mode));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_WHITE_BALANCE,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->white_balance, sizeof(exif_attributes->white_balance));
	pointer += count;

	count = exynos_exif_write_data(pointer, EXIF_TAG_SCENCE_CAPTURE_TYPE,
		EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->scene_capture_type, sizeof(exif_attributes->scene_capture_type));
	pointer += count;

	value = 0;
	memcpy(pointer, &value, OFFSET_SIZE);
	pointer += OFFSET_SIZE;

	// GPS
	if (exif_attributes->enableGps) {
		pointer = (unsigned char *) exif_ifd_gps;
		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_IFD_POINTER,
			EXIF_TYPE_LONG, 1, NULL, NULL, &offset, sizeof(offset));

		pointer = (unsigned char *) exif_ifd_start + offset;

		if (exif_attributes->gps_processing_method[0] == 0)
			value = NUM_0TH_IFD_GPS - 1;
		else
			value = NUM_0TH_IFD_GPS;

		memcpy(pointer, &value, NUM_SIZE);
		pointer += NUM_SIZE;

		offset += NUM_SIZE + value * IFD_SIZE + OFFSET_SIZE;

		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_VERSION_ID,
			EXIF_TYPE_BYTE, 4, NULL, NULL, &exif_attributes->gps_version_id, sizeof(char));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_LATITUDE_REF,
			EXIF_TYPE_ASCII, 2, NULL, NULL, &exif_attributes->gps_latitude_ref, sizeof(char));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_LATITUDE,
			EXIF_TYPE_RATIONAL, 3, &offset, exif_ifd_start, &exif_attributes->gps_latitude, sizeof(exif_attributes->gps_latitude[0]));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_LONGITUDE_REF,
			EXIF_TYPE_ASCII, 2, NULL, NULL, &exif_attributes->gps_longitude_ref, sizeof(char));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_LONGITUDE,
			EXIF_TYPE_RATIONAL, 3, &offset, exif_ifd_start, &exif_attributes->gps_longitude, sizeof(exif_attributes->gps_longitude[0]));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_ALTITUDE_REF,
			EXIF_TYPE_BYTE, 1, NULL, NULL, &exif_attributes->gps_altitude_ref, sizeof(char));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_ALTITUDE,
			EXIF_TYPE_RATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->gps_altitude, sizeof(exif_attributes->gps_altitude));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_TIMESTAMP,
			EXIF_TYPE_RATIONAL, 3, &offset, exif_ifd_start, &exif_attributes->gps_timestamp, sizeof(exif_attributes->gps_timestamp[0]));
		pointer += count;

		value = strlen((char *) exif_attributes->gps_processing_method);
		if (value > 0) {
			value = value > 100 ? 100 : value;

			data = calloc(1, value + sizeof(exif_ascii_prefix));
			memcpy(data, &exif_ascii_prefix, sizeof(exif_ascii_prefix));
			memcpy((void *) ((int) data + (int) sizeof(exif_ascii_prefix)), exif_attributes->gps_processing_method, value);

			count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_PROCESSING_METHOD,
				EXIF_TYPE_UNDEFINED, value + sizeof(exif_ascii_prefix), &offset, exif_ifd_start, data, sizeof(char));
			pointer += count;

			free(data);
		}

		count = exynos_exif_write_data(pointer, EXIF_TAG_GPS_DATESTAMP,
				EXIF_TYPE_ASCII, 11, &offset, exif_ifd_start, &exif_attributes->gps_datestamp, 1);
		pointer += count;

		value = 0;
		memcpy(pointer, &value, OFFSET_SIZE);
		pointer += OFFSET_SIZE;
	}

	if (exif_attributes->enableThumb && jpeg_thumbnail_data_memory != NULL && jpeg_thumbnail_size > 0) {
		exif_thumb_size = (unsigned int) jpeg_thumbnail_size;
		exif_thumb_data = (void *) jpeg_thumbnail_data_memory->data;

		value = offset;
		memcpy(exif_ifd_thumb, &value, OFFSET_SIZE);

		pointer = (unsigned char *) ((int) exif_ifd_start + (int) offset);

		value = NUM_1TH_IFD_TIFF;
		memcpy(pointer, &value, NUM_SIZE);
		pointer += NUM_SIZE;

		offset += NUM_SIZE + NUM_1TH_IFD_TIFF * IFD_SIZE + OFFSET_SIZE;

		count = exynos_exif_write_data(pointer, EXIF_TAG_IMAGE_WIDTH,
				EXIF_TYPE_LONG, 1, NULL, NULL, &exif_attributes->widthThumb, sizeof(exif_attributes->widthThumb));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_IMAGE_HEIGHT,
				EXIF_TYPE_LONG, 1, NULL, NULL, &exif_attributes->heightThumb, sizeof(exif_attributes->heightThumb));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_COMPRESSION_SCHEME,
				EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->compression_scheme, sizeof(exif_attributes->compression_scheme));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_ORIENTATION,
				EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->orientation, sizeof(exif_attributes->orientation));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_X_RESOLUTION,
				EXIF_TYPE_RATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->x_resolution, sizeof(exif_attributes->x_resolution));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_Y_RESOLUTION,
				EXIF_TYPE_RATIONAL, 1, &offset, exif_ifd_start, &exif_attributes->y_resolution, sizeof(exif_attributes->y_resolution));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_RESOLUTION_UNIT,
				EXIF_TYPE_SHORT, 1, NULL, NULL, &exif_attributes->resolution_unit, sizeof(exif_attributes->resolution_unit));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_JPEG_INTERCHANGE_FORMAT,
				EXIF_TYPE_LONG, 1, NULL, NULL, &offset, sizeof(offset));
		pointer += count;

		count = exynos_exif_write_data(pointer, EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LEN,
				EXIF_TYPE_LONG, 1, NULL, NULL, &exif_thumb_size, sizeof(exif_thumb_size));
		pointer += count;

		value = 0;
		memcpy(pointer, &value, OFFSET_SIZE);

		pointer = (unsigned char *) ((int) exif_ifd_start + (int) offset);

		memcpy(pointer, exif_thumb_data, exif_thumb_size);
		offset += exif_thumb_size;
	} else {
		value = 0;
		memcpy(exif_ifd_thumb, &value, OFFSET_SIZE);

	}

	pointer = (unsigned char *) exif_ifd_data_start;

	memcpy(pointer, exif_app1_marker, sizeof(exif_app1_marker));
	pointer += sizeof(exif_app1_marker);

	exif_size = offset + 10;
	value = exif_size - 2;
	exif_app1_size[0] = (value >> 8) & 0xff;
	exif_app1_size[1] = value & 0xff;

	memcpy(pointer, exif_app1_size, sizeof(exif_app1_size));

	*exif_data_memory_p = exif_data_memory;
	*exif_size_p = exif_size;

	return 0;

error:
	if (exif_data_memory != NULL && exif_data_memory->release != NULL)
		exif_data_memory->release(exif_data_memory);

	*exif_data_memory_p = NULL;
	*exif_size_p = 0;

	return -1;
}
