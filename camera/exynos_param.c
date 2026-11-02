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

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <ctype.h>

#define LOG_TAG "exynos_param"
#include <utils/Log.h>

#include "exynos_camera.h"

int list_head_insert(struct list_head *list, struct list_head *prev,
	struct list_head *next)
{
	if (list == NULL)
		return -EINVAL;

	list->prev = prev;
	list->next = next;

	if(prev != NULL)
		prev->next = list;
	if(next != NULL)
		next->prev = list;

	return 0;
}

void list_head_remove(struct list_head *list)
{
	if(list == NULL)
		return;

	if(list->next != NULL)
		list->next->prev = list->prev;
	if(list->prev != NULL)
		list->prev->next = list->next;
}

int exynos_param_register(struct exynos_camera *exynos_camera, char *key,
	union exynos_param_data data, enum exynos_param_type type)
{
	struct list_head *list_end;
	struct list_head *list;
	struct exynos_param *param;

	if (exynos_camera == NULL || key == NULL)
		return -EINVAL;

	param = (struct exynos_param *) calloc(1, sizeof(struct exynos_param));
	if (param == NULL)
		return -ENOMEM;

	param->key = strdup(key);
	switch (type) {
		case EXYNOS_PARAM_INT:
			param->data.integer = data.integer;
			break;
		case EXYNOS_PARAM_FLOAT:
			param->data.floating = data.floating;
			break;
		case EXYNOS_PARAM_STRING:
			param->data.string = strdup(data.string);
			break;
		default:
			ALOGE("%s: Invalid type", __func__);
			goto error;
	}
	param->type = type;

	list_end = (struct list_head *) exynos_camera->params;
	while (list_end != NULL && list_end->next != NULL)
		list_end = list_end->next;

	list = (struct list_head *) param;
	list_head_insert(list, list_end, NULL);

	if (exynos_camera->params == NULL)
		exynos_camera->params = param;

	return 0;

error:
	if (param != NULL) {
		if (param->key != NULL)
			free(param->key);

		free(param);
	}

	return -1;
}

void exynos_param_unregister(struct exynos_camera *exynos_camera,
	struct exynos_param *param)
{
	struct list_head *list;

	if (exynos_camera == NULL || param == NULL)
		return;

	list = (struct list_head *) exynos_camera->params;
	while (list != NULL) {
		if ((void *) list == (void *) param) {
			list_head_remove(list);

			if ((void *) list == (void *) exynos_camera->params)
				exynos_camera->params = (struct exynos_param *) list->next;

			if (param->type == EXYNOS_PARAM_STRING && param->data.string != NULL)
				free(param->data.string);

			memset(param, 0, sizeof(struct exynos_param));
			free(param);

			break;
		}

list_continue:
		list = list->next;
	}
}

struct exynos_param *exynos_param_find_key(struct exynos_camera *exynos_camera,
	char *key)
{
	struct exynos_param *param;
	struct list_head *list;

	if (exynos_camera == NULL || key == NULL)
		return NULL;

	list = (struct list_head *) exynos_camera->params;
	while (list != NULL) {
		param = (struct exynos_param *) list;
		if (param->key == NULL)
			goto list_continue;

		if (strcmp(param->key, key) == 0)
			return param;

list_continue:
		list = list->next;
	}

	return NULL;
}

int exynos_param_data_set(struct exynos_camera *exynos_camera, char *key,
	union exynos_param_data data, enum exynos_param_type type)
{
	struct exynos_param *param;

	if (exynos_camera == NULL || key == NULL)
		return -EINVAL;

	if (strchr(key, '=') || strchr(key, ';'))
		return -EINVAL;

	if (type == EXYNOS_PARAM_STRING && data.string != NULL &&
		(strchr(data.string, '=') || strchr(data.string, ';')))
		return -EINVAL;

	param = exynos_param_find_key(exynos_camera, key);
	if (param == NULL) {
		// The key isn't in the list yet
		exynos_param_register(exynos_camera, key, data, type);
		return 0;
	}

	if (param->type != type)
		ALOGE("%s: Mismatching types for key %s", __func__, key);

	if (param->type == EXYNOS_PARAM_STRING && param->data.string != NULL)
		free(param->data.string);

	switch (type) {
		case EXYNOS_PARAM_INT:
			param->data.integer = data.integer;
			break;
		case EXYNOS_PARAM_FLOAT:
			param->data.floating = data.floating;
			break;
		case EXYNOS_PARAM_STRING:
			param->data.string = strdup(data.string);
			break;
		default:
			ALOGE("%s: Invalid type", __func__);
			return -1;
	}
	param->type = type;

	return 0;
}

int exynos_param_data_get(struct exynos_camera *exynos_camera, char *key,
	union exynos_param_data *data, enum exynos_param_type type)
{
	struct exynos_param *param;

	if (exynos_camera == NULL || key == NULL || data == NULL)
		return -EINVAL;

	param = exynos_param_find_key(exynos_camera, key);
	if (param == NULL || param->type != type)
		return -1;

	memcpy(data, &param->data, sizeof(param->data));

	return 0;
}

int exynos_param_int_get(struct exynos_camera *exynos_camera,
	char *key)
{
	union exynos_param_data data;
	int rc;

	if (exynos_camera == NULL || key == NULL)
		return -EINVAL;

	rc = exynos_param_data_get(exynos_camera, key, &data, EXYNOS_PARAM_INT);
	if (rc < 0) {
		ALOGE("%s: Unable to get data for key %s", __func__, key);
		return -1;
	}

	return data.integer;
}

float exynos_param_float_get(struct exynos_camera *exynos_camera,
	char *key)
{
	union exynos_param_data data;
	int rc;

	if (exynos_camera == NULL || key == NULL)
		return -EINVAL;

	rc = exynos_param_data_get(exynos_camera, key, &data, EXYNOS_PARAM_FLOAT);
	if (rc < 0) {
		ALOGE("%s: Unable to get data for key %s", __func__, key);
		return -1;
	}

	return data.floating;
}

char *exynos_param_string_get(struct exynos_camera *exynos_camera,
	char *key)
{
	union exynos_param_data data;
	int rc;

	if (exynos_camera == NULL || key == NULL)
		return NULL;

	rc = exynos_param_data_get(exynos_camera, key, &data, EXYNOS_PARAM_STRING);
	if (rc < 0) {
		ALOGE("%s: Unable to get data for key %s", __func__, key);
		return NULL;
	}

	return data.string;
}

int exynos_param_int_set(struct exynos_camera *exynos_camera,
	char *key, int integer)
{
	union exynos_param_data data;
	int rc;

	if (exynos_camera == NULL || key == NULL)
		return -EINVAL;

	data.integer = integer;

	rc = exynos_param_data_set(exynos_camera, key, data, EXYNOS_PARAM_INT);
	if (rc < 0) {
		ALOGE("%s: Unable to set data for key %s", __func__, key);
		return -1;
	}

	return 0;
}

int exynos_param_float_set(struct exynos_camera *exynos_camera,
	char *key, float floating)
{
	union exynos_param_data data;
	int rc;

	if (exynos_camera == NULL || key == NULL)
		return -EINVAL;

	data.floating = floating;

	rc = exynos_param_data_set(exynos_camera, key, data, EXYNOS_PARAM_FLOAT);
	if (rc < 0) {
		ALOGE("%s: Unable to set data for key %s", __func__, key);
		return -1;
	}

	return 0;
}

int exynos_param_string_set(struct exynos_camera *exynos_camera,
	char *key, char *string)
{
	union exynos_param_data data;
	int rc;

	if (exynos_camera == NULL || key == NULL || string == NULL)
		return -EINVAL;

	data.string = string;

	rc = exynos_param_data_set(exynos_camera, key, data, EXYNOS_PARAM_STRING);
	if (rc < 0) {
		ALOGE("%s: Unable to set data for key %s", __func__, key);
		return -1;
	}

	return 0;
}

char *exynos_params_string_get(struct exynos_camera *exynos_camera)
{
	struct exynos_param *param;
	struct list_head *list;
	char *string = NULL;
	char *s = NULL;
	int length = 0;
	int l = 0;

	if (exynos_camera == NULL)
		return NULL;

	list = (struct list_head *) exynos_camera->params;
	while (list != NULL) {
		param = (struct exynos_param *) list;
		if (param->key == NULL)
			goto list_continue_length;

		length += strlen(param->key);
		length++;

		switch (param->type) {
			case EXYNOS_PARAM_INT:
			case EXYNOS_PARAM_FLOAT:
				length += 16;
				break;
			case EXYNOS_PARAM_STRING:
				length += strlen(param->data.string);
				break;
			default:
				ALOGE("%s: Invalid type", __func__);
				return NULL;
		}

		length++;

list_continue_length:
		list = list->next;
	}

	if (length == 0)
		return NULL;

	string = calloc(1, length);
	s = string;

	list = (struct list_head *) exynos_camera->params;
	while (list != NULL) {
		param = (struct exynos_param *) list;
		if (param->key == NULL)
			goto list_continue;

		l = sprintf(s, "%s=", param->key);
		s += l;

		switch (param->type) {
			case EXYNOS_PARAM_INT:
				l = snprintf(s, 16, "%d", param->data.integer);
				s += l;
				break;
			case EXYNOS_PARAM_FLOAT:
				l = snprintf(s, 16, "%g", param->data.floating);
				s += l;
				break;
			case EXYNOS_PARAM_STRING:
				l = sprintf(s, "%s", param->data.string);
				s += l;
				break;
			default:
				ALOGE("%s: Invalid type", __func__);
				return NULL;
		}

		if (list->next != NULL) {
			*s = ';';
			s++;
		} else {
			*s = '\0';
			break;
		}

list_continue:
		list = list->next;
	}

	return string;
}

int exynos_params_string_set(struct exynos_camera *exynos_camera, char *string)
{
	union exynos_param_data data;
	enum exynos_param_type type;

	char *d = NULL;
	char *s = NULL;
	char *k = NULL;
	char *v = NULL;

	char *key;
	char *value;

	int rc;
	int i;

	if (exynos_camera == NULL || string == NULL)
		return -1;

	d = strdup(string);
	s = d;

	while (1) {
		k = strchr(s, '=');
		if (k == NULL)
			break;
		*k = '\0';
		key = s;

		v = strchr(k+1, ';');
		if (v != NULL)
			*v = '\0';
		value = k+1;

		k = value;
		if (isdigit(k[0]) || k[0] == '-') {
			type = EXYNOS_PARAM_INT;

			for (i=1 ; k[i] != '\0' ; i++) {
				if (k[i] == '.') {
					type = EXYNOS_PARAM_FLOAT;
				} else if (!isdigit(k[i])) {
					type = EXYNOS_PARAM_STRING;
					break;
				}
			}
		} else {
			type = EXYNOS_PARAM_STRING;
		}

		switch (type) {
			case EXYNOS_PARAM_INT:
				data.integer = atoi(value);
				break;
			case EXYNOS_PARAM_FLOAT:
				data.floating = atof(value);
				break;
			case EXYNOS_PARAM_STRING:
				data.string = value;
				break;
			default:
				ALOGE("%s: Invalid type", __func__);
				goto error;
		}

		rc = exynos_param_data_set(exynos_camera, key, data, type);
		if (rc < 0) {
			ALOGE("%s: Unable to set data for key %s", __func__, key);
			goto error;
		}

		if (v == NULL)
			break;

		s = v+1;
	}

	if (d != NULL)
		free(d);

	return 0;

error:
	if (d != NULL)
		free(d);

	return -1;
}
