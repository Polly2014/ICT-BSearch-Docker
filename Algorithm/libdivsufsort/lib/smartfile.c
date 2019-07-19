/**
 * smartfile.c
 *
 * @brief Large file fragmentation and small file concatenation to form a proper file size for index build/search
 *
 *  Created on: Dec 15, 2016
 *      Author: Abe Wu
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parson.h"
#include "smartfile.h"

/**
 * The json file structure
 */
static char config_schema[] = "{\
		\"index_file\":\"\",\
		\"raw_files\":[\
			\{\
				\"name\":\"\",\
				\"offset\":0,\
				\"length\":0\
			},\
			\{\
				\"name\":\"\",\
				\"offset\":0,\
				\"length\":0\
			}\
		]\
	}";


JSON_Value *schema = NULL;

/**
 * @brief Open the json config file and check whether it matches the schema
 */
JSON_Value *smfile_open_config(const char *filepath)
{
	JSON_Value *config;
	config = json_parse_file(filepath);
	if (schema == NULL) {
		schema = json_parse_string(config_schema);
	}
	if (config == NULL || schema == NULL) {
		return NULL;
	} else if (json_validate(schema, config) != JSONSuccess) {
		json_value_free(config);
		return NULL;
	}

	return config;
}

file_list smfile_list_create(JSON_Value *config)
{
	size_t i;
	file_list flist;
	JSON_Object *afile;
	JSON_Array *json_array = json_object_get_array(json_object(config), "raw_files");
	file_info curfile;

	/* allocate memory*/
	flist = malloc(sizeof(*flist));
	memset(flist, 0, sizeof(*flist));

	/* set the number of files */
	flist->count = json_array_get_count(json_array);
	if ((flist->count = json_array_get_count(json_array)) == 0) {
		return flist;
	}

	/* Allocate memory for file info and handles */
	flist->files = malloc(sizeof(*flist->files) * flist->count);
	memset(flist->files, 0, sizeof(*flist->files) * flist->count);

	/* fill file info */
	for (i = 0, flist->total_len = 0; i < flist->count; i++) {
		afile = json_array_get_object(json_array, i);
		curfile = flist->files + i;
		curfile->name = json_object_get_string(afile, "name");
		curfile->offset = (off_t) json_object_get_number(afile, "offset");
		curfile->length = (off_t) json_object_get_number(afile, "length");
		curfile->offset_in_list = flist->total_len;
		curfile->fps = NULL;
		flist->total_len += curfile->length;
	}

	return flist;
}

file_info smfile_list_get(file_list flist, off_t offset)
{
	off_t left, middle, right;
	file_info files;

	if (flist == NULL || (files = flist->files) == NULL || flist->count == 0 || offset >= flist->total_len) {
		return NULL;
	}

	/* Binary search to find the appropriate file to read */
	for (left = 0, right = flist->count - 1; left < right; /* idle here */) {
		middle = (left + right) / 2;
		if (offset < files[middle].offset_in_list) {
			right = middle - 1;
		} else if (offset < files[middle].offset_in_list + files[middle].length) {
			/* middle.offset <= offset < middle.offset + length: middle is what we are looking for */
			left = right = middle;
		} else {
			left = middle + 1;
		}
	}

	return files + left;
}

/**
 * @breif similar function to fread on the filelist
 */
size_t smfile_list_read(void *data, size_t size, size_t nitems, file_list flist, off_t offset)
{
	size_t byte_left, byte_read, cur_off, actual_read;
	file_info curfile;

	curfile = smfile_list_get(flist, offset);
	cur_off = offset - curfile->offset_in_list;
	for (byte_left = size * nitems; byte_left > 0 && curfile < flist->files + flist->count; curfile++) {
		if (curfile->fps == NULL) {
			/* This file is not opened yet, open it */
			if ((curfile->fps = fopen(curfile->name, "rb")) == NULL) {
				return (nitems*size - byte_left) / size; /*<< the return unit is number of items */
			}
		}

		/* determine how many bytes we need to read from current file */
		byte_read = curfile->length - cur_off;
		byte_read = MIN(byte_read, byte_left);

		/* read from cur_off to length */
		if (fseeko(curfile->fps, curfile->offset + cur_off, SEEK_SET) != 0) {
			/* should log the errno */
			return (nitems*size - byte_left) / size; /*<< the return unit is number of items */
		}
		/* Due to file boundary, we have to read data as size of 1 */
		if ((actual_read = fread(data, 1, byte_read, curfile->fps)) != byte_read) {
			/* read file error */
			return (nitems*size - byte_left - actual_read) / size;
		}

		/* Decrement the byte_left */
		data += byte_read;
		byte_left -= byte_read;

		/* Only the first file has the cur_off not zero */
		cur_off = 0;
	}

	return (nitems*size - byte_left) / size;
}

const char *smfile_get_index_file(JSON_Value *config)
{
	return json_object_get_string(json_object(config), "index_file");
}
