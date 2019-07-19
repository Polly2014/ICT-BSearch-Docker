/**
 * smartfile.c
 *
 * @brief Large file fragmentation and small file concatenation to form a proper file size for index build/search
 *
 *  Created on: Dec 15, 2016
 *      Author: Abe Wu
 */

#ifndef LIB_SMARTFILE_H_
#define LIB_SMARTFILE_H_

#include "parson.h"

#ifndef MIN
# define MIN(_a, _b) (((_a) < (_b)) ? (_a) : (_b))
#endif /* MIN */
#ifndef MAX
# define MAX(_a, _b) (((_a) > (_b)) ? (_a) : (_b))
#endif /* MAX */

#define exit_result(code, strfmt...) {	\
	char errmsg[1024];\
	snprintf(errmsg, sizeof(errmsg), strfmt);\
	fprintf(stdout, "{\"code\": %d, \"message\":\"%s: %s\"}\n", code, program_name, errmsg);\
	exit(code);\
}

#define MAX_RAW_FILE_LENGTH 0xFFFFFFFFLL

typedef struct {
	off_t offset;		/*<< starting position in the file */
	off_t length;		/*<< the length of the data that to be considered in the file */
	off_t offset_in_list;	/*<< the starting position is what offset of in the file list as a virtual file? */
	const char *name;	/*<< full path of the file */
	FILE  *fps;			/*<< file pointer for each file */
} *file_info, file_info_t;

typedef struct {
	size_t 		count;		/*<< the number of files in the list */
	off_t 		total_len;	/*<< the total length of data in the list */
	file_info 	files;		/*<< array of each file's info */
} *file_list, file_list_t;


/**
 * @brief Open the json config file and check whether it matches the schema
 */
JSON_Value *smfile_open_config(const char *filepath);

/**
 * @brief create the file list from the configure
 */
file_list smfile_list_create(JSON_Value *config);

/**
 * @brief get a specific file from the global offset in the file list
 */
file_info smfile_list_get(file_list flist, off_t offset);

/**
 * @breif similar function to fread on the filelist
 */
size_t smfile_list_read(void *data, size_t size, size_t ntiems, file_list flist, off_t offset);

const char *smfile_get_index_file(JSON_Value *config);

#endif
