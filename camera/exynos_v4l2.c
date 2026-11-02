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
#include <errno.h>
#include <malloc.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>

#define LOG_TAG "exynos_v4l2"
#include <utils/Log.h>

#include "exynos_camera.h"

/*
 * Utils
 */

int exynos_v4l2_find_index(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	int index;
	int i;

	if (exynos_camera == NULL || exynos_camera->config == NULL ||
		exynos_camera->config->v4l2_nodes == NULL)
		return -EINVAL;

	if (exynos_v4l2_id > exynos_camera->config->v4l2_nodes_count)
		return -1;

	index = -1;
	for (i = 0; i < exynos_camera->config->v4l2_nodes_count; i++) {
		if (exynos_camera->config->v4l2_nodes[i].id == exynos_v4l2_id &&
			exynos_camera->config->v4l2_nodes[i].node != NULL) {
			index = i;
		}
	}

	return index;
}

int exynos_v4l2_find_fd(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	int index;

	if (exynos_camera == NULL)
		return -EINVAL;

	index = exynos_v4l2_find_index(exynos_camera, exynos_v4l2_id);
	if (index < 0)
		return -1;

	return exynos_camera->v4l2_fds[index];
}

/*
 * File ops
 */

int exynos_v4l2_open(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	char *node;
	int index;
	int fd;

	if (exynos_camera == NULL || exynos_camera->config == NULL ||
		exynos_camera->config->v4l2_nodes == NULL)
		return -EINVAL;

	index = exynos_v4l2_find_index(exynos_camera, exynos_v4l2_id);
	if (index < 0) {
		ALOGE("%s: Unable to find v4l2 node #%d", __func__, exynos_v4l2_id);
		return -1;
	}

	node = exynos_camera->config->v4l2_nodes[index].node;
	fd = open(node, O_RDWR);
	if (fd < 0) {
		ALOGE("%s: Unable to open v4l2 node #%d", __func__, exynos_v4l2_id);
		return -1;
	}

	exynos_camera->v4l2_fds[index] = fd;

	return 0;
}

void exynos_v4l2_close(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	int index;

	if (exynos_camera == NULL || exynos_camera->config == NULL ||
		exynos_camera->config->v4l2_nodes == NULL)
		return;

	index = exynos_v4l2_find_index(exynos_camera, exynos_v4l2_id);
	if (index < 0) {
		ALOGE("%s: Unable to find v4l2 node #%d", __func__, exynos_v4l2_id);
		return;
	}

	if (exynos_camera->v4l2_fds[index] > 0)
		close(exynos_camera->v4l2_fds[index]);

	exynos_camera->v4l2_fds[index] = -1;
}

int exynos_v4l2_ioctl(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int request, void *data)
{
	int fd;

	if (exynos_camera == NULL)
		return -EINVAL;

	fd = exynos_v4l2_find_fd(exynos_camera, exynos_v4l2_id);
	if (fd < 0) {
		ALOGE("%s: Unable to find v4l2 fd #%d", __func__, exynos_v4l2_id);
		return -1;
	}

	return ioctl(fd, request, data);
}

int exynos_v4l2_poll(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	struct pollfd events;
	int fd;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	fd = exynos_v4l2_find_fd(exynos_camera, exynos_v4l2_id);
	if (fd < 0) {
		ALOGE("%s: Unable to find v4l2 fd #%d", __func__, exynos_v4l2_id);
		return -1;
	}

	memset(&events, 0, sizeof(events));
	events.fd = fd;
	events.events = POLLIN | POLLERR;

	rc = poll(&events, 1, 1000);
	if (rc < 0 || events.revents & POLLERR) {
		ALOGE("%s: poll failed", __func__);
		return -1;
	}

	return rc;
}

/*
 * VIDIOC
 */

int exynos_v4l2_qbuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int memory, int index)
{
	struct v4l2_buffer buffer;
	int rc;

	if (exynos_camera == NULL || index < 0)
		return -EINVAL;

	buffer.type = type;
	buffer.memory = memory;
	buffer.index = index;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_QBUF, &buffer);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}

int exynos_v4l2_qbuf_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int index)
{
	return exynos_v4l2_qbuf(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE,
		V4L2_MEMORY_MMAP, index);
}

int exynos_v4l2_qbuf_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int index)
{
	return exynos_v4l2_qbuf(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT,
		V4L2_MEMORY_USERPTR, index);
}

int exynos_v4l2_dqbuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int memory)
{
	struct v4l2_buffer buffer;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	memset(&buffer, 0, sizeof(buffer));
	buffer.type = type;
	buffer.memory = memory;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_DQBUF, &buffer);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return buffer.index;
}

int exynos_v4l2_dqbuf_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	return exynos_v4l2_dqbuf(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE,
		V4L2_MEMORY_MMAP);
}

int exynos_v4l2_dqbuf_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	return exynos_v4l2_dqbuf(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT,
		V4L2_MEMORY_USERPTR);
}

int exynos_v4l2_reqbufs(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int memory, int count)
{
	struct v4l2_requestbuffers requestbuffers;
	int rc;

	if (exynos_camera == NULL || count < 0)
		return -EINVAL;

	requestbuffers.type = type;
	requestbuffers.count = count;
	requestbuffers.memory = memory;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_REQBUFS, &requestbuffers);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return requestbuffers.count;
}

int exynos_v4l2_reqbufs_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int count)
{
	return exynos_v4l2_reqbufs(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE,
		V4L2_MEMORY_MMAP, count);
}

int exynos_v4l2_reqbufs_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int count)
{
	return exynos_v4l2_reqbufs(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT,
		V4L2_MEMORY_USERPTR, count);
}

int exynos_v4l2_querybuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int memory, int index)
{
	struct v4l2_buffer buffer;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	memset(&buffer, 0, sizeof(buffer));
	buffer.type = type;
	buffer.memory = memory;
	buffer.index = index;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_QUERYBUF, &buffer);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return buffer.length;
}

int exynos_v4l2_querybuf_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int index)
{
	return exynos_v4l2_querybuf(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE,
		V4L2_MEMORY_MMAP, index);
}

int exynos_v4l2_querybuf_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int index)
{
	return exynos_v4l2_querybuf(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT,
		V4L2_MEMORY_USERPTR, index);
}

int exynos_v4l2_querycap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int flags)
{
	struct v4l2_capability cap;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_QUERYCAP, &cap);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	if (!(cap.capabilities & flags))
		return -1;

	return 0;
}

int exynos_v4l2_querycap_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	return exynos_v4l2_querycap(exynos_camera, exynos_v4l2_id, V4L2_CAP_VIDEO_CAPTURE);
}

int exynos_v4l2_querycap_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	return exynos_v4l2_querycap(exynos_camera, exynos_v4l2_id, V4L2_CAP_VIDEO_OUTPUT);
}

int exynos_v4l2_streamon(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type)
{
	enum v4l2_buf_type buf_type;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	buf_type = type;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_STREAMON, &buf_type);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}

int exynos_v4l2_streamon_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	return exynos_v4l2_streamon(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE);
}

int exynos_v4l2_streamon_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	return exynos_v4l2_streamon(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT);
}

int exynos_v4l2_streamoff(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type)
{
	enum v4l2_buf_type buf_type;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	buf_type = type;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_STREAMOFF, &buf_type);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}

int exynos_v4l2_streamoff_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	return exynos_v4l2_streamoff(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE);
}

int exynos_v4l2_streamoff_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id)
{
	return exynos_v4l2_streamoff(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT);
}

int exynos_v4l2_g_fmt(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int *width, int *height, int *fmt)
{
	struct v4l2_format format;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	format.type = type;
	format.fmt.pix.field = V4L2_FIELD_NONE;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_G_FMT, &format);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	if (width != NULL)
		*width = format.fmt.pix.width;
	if (height != NULL)
		*height = format.fmt.pix.height;
	if (fmt != NULL)
		*fmt = format.fmt.pix.pixelformat;

	return 0;
}

int exynos_v4l2_g_fmt_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int *width, int *height, int *fmt)
{
	return exynos_v4l2_g_fmt(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE,
		width, height, fmt);
}

int exynos_v4l2_g_fmt_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int *width, int *height, int *fmt)
{
	return exynos_v4l2_g_fmt(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT,
		width, height, fmt);
}

int exynos_v4l2_s_fmt_pix(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int width, int height, int fmt, int priv)
{
	struct v4l2_format format;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	memset(&format, 0, sizeof(format));
	format.type = type;
	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	format.fmt.pix.pixelformat = fmt;
	format.fmt.pix.field = V4L2_FIELD_NONE;
	format.fmt.pix.priv = priv;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_S_FMT, &format);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}

int exynos_v4l2_s_fmt_pix_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int width, int height, int fmt, int priv)
{
	return exynos_v4l2_s_fmt_pix(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE,
		width, height, fmt, priv);
}

int exynos_v4l2_s_fmt_pix_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int width, int height, int fmt, int priv)
{
	return exynos_v4l2_s_fmt_pix(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT,
		width, height, fmt, priv);
}

int exynos_v4l2_s_fmt_win(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int left, int top, int width, int height)
{
	struct v4l2_format format;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
	format.fmt.win.w.left = left;
	format.fmt.win.w.top = top;
	format.fmt.win.w.width = width;
	format.fmt.win.w.height = height;


	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_S_FMT, &format);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}

int exynos_v4l2_enum_fmt(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int fmt)
{
	struct v4l2_fmtdesc fmtdesc;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	fmtdesc.type = type;
	fmtdesc.index = 0;

	do {
		rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_ENUM_FMT, &fmtdesc);
		if (rc < 0) {
			ALOGE("%s: ioctl failed", __func__);
			return -1;
		}

		if (fmtdesc.pixelformat == (unsigned int) fmt)
			return 0;

		fmtdesc.index++;
	} while (rc >= 0);

	return -1;
}

int exynos_v4l2_enum_fmt_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int fmt)
{
	return exynos_v4l2_enum_fmt(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE,
		fmt);
}

int exynos_v4l2_enum_fmt_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int fmt)
{
	return exynos_v4l2_enum_fmt(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT,
		fmt);
}

int exynos_v4l2_enum_input(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int id)
{
	struct v4l2_input input;
	int rc;

	if (exynos_camera == NULL || id < 0)
		return -EINVAL;

	input.index = id;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_ENUMINPUT, &input);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	if (input.name[0] == '\0')
		return -1;

	return 0;
}

int exynos_v4l2_s_input(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int id)
{
	struct v4l2_input input;
	int rc;

	if (exynos_camera == NULL || id < 0)
		return -EINVAL;

	input.index = id;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_S_INPUT, &input);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}

int exynos_v4l2_g_ext_ctrls(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	struct v4l2_ext_control *control, int count)
{
	struct v4l2_ext_controls controls;
	int rc;

	if (exynos_camera == NULL || control == NULL)
		return -EINVAL;

	memset(&controls, 0, sizeof(controls));
	controls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
	controls.count = count;
	controls.controls = control;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_G_EXT_CTRLS, &controls);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}

int exynos_v4l2_g_ctrl(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int id, int *value)
{
	struct v4l2_control control;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	control.id = id;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_G_CTRL, &control);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	if (value != NULL)
		*value = control.value;

	return 0;
}

int exynos_v4l2_s_ctrl(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int id, int value)
{
	struct v4l2_control control;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	control.id = id;
	control.value = value;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_S_CTRL, &control);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return control.value;
}

int exynos_v4l2_s_parm(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, struct v4l2_streamparm *streamparm)
{
	int rc;

	if (exynos_camera == NULL || streamparm == NULL)
		return -EINVAL;

	streamparm->type = type;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_S_PARM, streamparm);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}

int exynos_v4l2_s_parm_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	struct v4l2_streamparm *streamparm)
{
	return exynos_v4l2_s_parm(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE,
		streamparm);
}

int exynos_v4l2_s_parm_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	struct v4l2_streamparm *streamparm)
{
	return exynos_v4l2_s_parm(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT,
		streamparm);
}

int exynos_v4l2_s_crop(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int type, int left, int top, int width, int height)
{
	struct v4l2_crop crop;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	crop.type = type;
	crop.c.left = left;
	crop.c.top = top;
	crop.c.width = width;
	crop.c.height = height;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_S_CROP, &crop);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}

int exynos_v4l2_s_crop_cap(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int left, int top, int width, int height)
{
	return exynos_v4l2_s_crop(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_CAPTURE,
		left, top, width, height);
}

int exynos_v4l2_s_crop_out(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	int left, int top, int width, int height)
{
	return exynos_v4l2_s_crop(exynos_camera, exynos_v4l2_id, V4L2_BUF_TYPE_VIDEO_OUTPUT,
		left, top, width, height);
}

int exynos_v4l2_g_fbuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	void **base, int *width, int *height, int *fmt)
{
	struct v4l2_framebuffer framebuffer;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_G_FBUF, &framebuffer);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	if (base != NULL)
		*base = framebuffer.base;
	if (width != NULL)
		*width = framebuffer.fmt.width;
	if (height != NULL)
		*height = framebuffer.fmt.height;
	if (fmt != NULL)
		*fmt = framebuffer.fmt.pixelformat;

	return 0;
}

int exynos_v4l2_s_fbuf(struct exynos_camera *exynos_camera, int exynos_v4l2_id,
	void *base, int width, int height, int fmt)
{
	struct v4l2_framebuffer framebuffer;
	int rc;

	if (exynos_camera == NULL)
		return -EINVAL;

	memset(&framebuffer, 0, sizeof(framebuffer));
	framebuffer.base = base;
	framebuffer.fmt.width = width;
	framebuffer.fmt.height = height;
	framebuffer.fmt.pixelformat = fmt;

	rc = exynos_v4l2_ioctl(exynos_camera, exynos_v4l2_id, VIDIOC_S_FBUF, &framebuffer);
	if (rc < 0) {
		ALOGE("%s: ioctl failed", __func__);
		return -1;
	}

	return 0;
}
