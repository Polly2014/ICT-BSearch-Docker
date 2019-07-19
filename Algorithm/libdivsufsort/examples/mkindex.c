/*
 * mksary.c for libdivsufsort
 * Copyright (c) 2003-2008 Yuta Mori All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MEMORY_H
# include <memory.h>
#endif
#if HAVE_STDDEF_H
# include <stddef.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_IO_H && HAVE_FCNTL_H
# include <io.h>
# include <fcntl.h>
#endif
#include <time.h>
#include <divsufsort.h>
#include "lfs.h"
#include "parson.h"
#include "smartfile.h"

#define GRAM_BITS 24
#define GRAM_ARRAY_SIZE (1<<GRAM_BITS)

/**
 * This function calculates the 3-gram starting position of the suffix array
 * @param[in] T the data stream that be calculated, it should be at least 8 bytes larger than n
 * @param[out] GRAM the 3-gram starting position
 * @param[in] size of the data stream
 */
saint_t gram_pos(const sauchar_t *T, saidx_t *GRAM, saidx_t n)
{
	saidx_t i;

	/* sanity check */
	if (T == NULL || GRAM == NULL) {
		return -1;
	}

	memset(GRAM, 0, (size_t)GRAM_ARRAY_SIZE * sizeof(*GRAM));
	for (i = 0; i < n; i++) {
		GRAM[(T[i]<<16) + (T[i+1]<<8) + T[i+2]]++;
	}
	for (i = 0; i < GRAM_ARRAY_SIZE - 1; i++) {
		GRAM[i+1] += GRAM[i];
	}
	for (i = GRAM_ARRAY_SIZE - 1; i > 0; i--) {
		GRAM[i] = GRAM[i-1];
	}
	GRAM[0] = 0;

	return 0;
}

const char *program_name;

int
main(int argc, const char *argv[])
{
	FILE *fp, *ofp;
	size_t i;
	sauchar_t *T;
	saidx_t *SA, *GRAM;
	LFS_OFF_T n, curp;

	JSON_Value *config;
	file_list raw_files;
	file_info infile;
	const char *index_file;

	program_name = argv[0];

	/* Check arguments. */
	if (argc != 2) {
		exit_result(1, "Invalid argument");
	}

	/* Check JSON format. */
	config = smfile_open_config(argv[1]);
	if (config == NULL) {
		exit_result(2, "json config file %s error", argv[1]);
	}

	/* Convert JSON file into a structure */
	raw_files = smfile_list_create(config);
	index_file = smfile_get_index_file(config);
	if ((n = raw_files->total_len) > MAX_RAW_FILE_LENGTH) {
		/* We only support 4gB file length at maximum */
		exit_result(6, "overall input file length too huge: %zu, should less than: %zu", raw_files->total_len, MAX_RAW_FILE_LENGTH);
	}

	/* Allocate 5blocksize bytes of memory. */
	T = (sauchar_t *)malloc((size_t)(n+8) * sizeof(sauchar_t));
	SA = (saidx_t *)malloc((size_t)n * sizeof(saidx_t));
	GRAM = (saidx_t *)malloc((size_t)GRAM_ARRAY_SIZE * sizeof(saidx_t));
	if(T == NULL || SA == NULL || GRAM == NULL) {
		exit_result(7, "cannot allocate memory");
	}
	memset(T+n, 8, 0);

	/* Open each input file and read */
	for (i = 0, curp = 0; i < raw_files->count; i++) {
		infile = raw_files->files + i;
		if ((fp = LFS_FOPEN(infile->name, "rb")) == NULL) {
			exit_result(4, "open input file failed: %s", infile->name);
		}
		LFS_FSEEK(fp, infile->offset, SEEK_SET);
		if (fread(T + curp, sizeof(sauchar_t), (size_t)infile->length, fp) != (size_t)infile->length) {
			exit_result(4, "%s %s: offset %zd, length %zd",
					(ferror(fp) || !feof(fp)) ? "Cannot read from" : "Unexpected EOF in",
					infile->name, infile->offset, infile->length);
		}
		curp += infile->length;
		fclose(fp);
	}

	/* open index file to write */
	if ((ofp = LFS_FOPEN(index_file, "wb")) == NULL) {
		exit_result(4, "cannot open index file %s", index_file);
	}

	/* Construct&write the suffix array */
	if(divsufsort(T, SA, (saidx_t)n) != 0) {
		exit_result(7, "cannot allocate memory");
	}
	if (fwrite(SA, sizeof(saidx_t), (size_t)n, ofp) != (size_t)n) {
		exit_result(4, "cannot write to index file %s", index_file);
	}

	/* Construct&write the 3-gram position array */
	gram_pos(T, GRAM, n);
	if (fwrite(GRAM, sizeof(saidx_t), (size_t)GRAM_ARRAY_SIZE, ofp) != (size_t)GRAM_ARRAY_SIZE) {
		exit_result(4, "cannot write to index file %s", index_file);
	}

	fclose(ofp);
	free(SA);
	free(GRAM);
	free(T);

	exit_result(0, "Success");
}
