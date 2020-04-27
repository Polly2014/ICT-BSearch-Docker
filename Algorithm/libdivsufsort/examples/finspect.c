/*
 * finspect.c
 *
 *  Created on: Feb 24, 2020
 *      Author: Abe Wu, Polly
 *
 *
Input:
{
    "file_path": "full path name",
    "tr_class": "class of the tr",
    "td_class_offset": "class of the td for offset",
    "td_class_hex": "class of the td for hex",
    "td_class_ascii": "class of the td for ascii",
    "td_class_bit": "class of the td for bit",
    "matched_prefix": "<prefix>",
    "matched_suffix": "<suffix>",
    "matches": [
        {
            "offset": 25067362,
            "offset_bit": 7,
            "length_bit": 20
        },
        {
            "offset": 25301856,
            "offset_bit": 7,
            "length_bit": 20
        }
    ]
}

   Output: Does not include thead, only output the part in tbody
	<tr class="tr_class">
		<td class="td_class_offset">0x00112228</td>
		<td class="td_class_hex">61 62 63 64 65 66 67 68</td>
		<td class="td_class_ascii">abcdefgh</td>
		<td class="td_class_bit">01100001 01100010 01100011 01100100  01100101 01100110 01100111 01101000</td>
	</tr>
	<tr class="tr_class">
		<td class="td_class_offset">0x00112230</td>
		<td class="td_class_hex">61 62 6<prefix>3 64 65 6<suffix>6 67 68</td>
		<td class="td_class_ascii">ab<prefix>cdef<suffix>gh</td>
		<td class="td_class_bit">01100001 01100010 0110<prefix>0011 01100100  01100101 011<suffix>00110 01100111 01101000</td>
	</tr>
	<tr class="tr_class">
		<td class="td_class_offset">0x00112238</td>
		<td class="td_class_hex">61 62 63 64 65 66 67 68</td>
		<td class="td_class_ascii">abcdefgh</td>
		<td class="td_class_bit">01100001 01100010 01100011 01100100  01100101 01100110 01100111 01101000</td>
	</tr>
 *
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <time.h>
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
#include <ctype.h>
#include <divsufsort.h>
#include "lfs.h"
#include "mmtregex.h"
#include "smartfile.h"

typedef struct {
	const char *tr_class ;
	const char *td_class_offset;
	const char *td_class_offset_hex;
	const char *td_class_hex;
	const char *td_class_ascii;
	const char *td_class_bit;
	const char *matched_prefix;
	const char *matched_suffix;
	off_t preload_lines; // Number of lines (each 8 bytes) before the match
	off_t postload_lines; // Number of lines (each 8 bytes) before the match
} html_css_t, *html_css;

static char config_schema[] = "\
{\
    \"file_path\": \"se.c\",\
    \"css\": {\
	    \"tr_class\": \"tr_class\",\
	    \"td_class_offset\": \"offset_class\",\
	    \"td_class_offset_hex\": \"offset_hex_class\",\
	    \"td_class_hex\": \"hex_class\",\
	    \"td_class_ascii\": \"ascii_class\",\
	    \"td_class_bit\": \"bit_class\",\
	    \"matched_prefix\": \"<font color='red'>\",\
	    \"matched_suffix\": \"</font>\",\
	    \"preload_lines\": 2,\
	    \"postload_lines\": 1\
	},\
    \"matches\": [\
        {\
            \"offset\": 0,\
            \"offset_bit\": 7,\
            \"length_bit\": 24\
        },\
        {\
            \"offset\": 0,\
            \"offset_bit\": 0,\
            \"length_bit\": 24\
		}\
	]\
}";


JSON_Value *schema = NULL;

/* Print one match */
static int print_uint32_per_line = 2;
static void print_match_html(html_css css, FILE *fp, off_t flength, off_t offset, int offset_bit, int length_bit)
{
	off_t line, first_line, match_start_line, match_end_line, last_line, read_length;

	// THe offset of the match (start, end) in a line
	off_t match_start_bit, match_end_bit, match_start_byte, match_end_byte;
	unsigned char *buf;

    int i, j, bit_offset, bytes_per_line, bits_per_line;

	// Sanity check
	if (!fp || !css || css->preload_lines > 1024 || css->postload_lines > 1024 || offset >= flength || offset_bit < 0 || offset_bit > 7 || length_bit > 65536) {
		return;
	}

    bytes_per_line = print_uint32_per_line * 4;
    bits_per_line = bytes_per_line * 8;

    //printf("@%s: buf = %p, buf_offset = %d, start = %d, end = %d\n", __FUNCTION__, buf, buf_offset, start, end);



    /* format like:
     * 0x00112233-0x0011223A
     * File      Address     HEX                      ASCII     Binary
     * cap.dat   0x00112230  61 62 63 64 65 66 67 68  abcdefgh  01100001 01100010 01100011 01100100  01100101 01100110 01100111 01101000
     * cap.dat   0x00112238 ...
     */
    // printf("\t<tr class='%s'>\n", css->tr_class);

//    printf("<td>");
    // Match start at this line
    match_start_line = offset / bytes_per_line;
    match_start_bit = (offset * 8 + offset_bit) % bits_per_line;
    match_start_byte = match_start_bit / 8;
//    printf("start = %d / %d / %d", match_start_bit, match_start_byte, match_start_line);

    // Match end at this line
    match_end_line = (offset * 8 + offset_bit + length_bit - 1) / bits_per_line;
    match_end_bit = (offset * 8 + offset_bit + length_bit - 1) % bits_per_line;
    if (match_end_bit == 0) {
    	match_end_line --;
    	match_end_bit = 8 * bytes_per_line;
    }
    match_end_byte = match_end_bit / 8;
//    printf("<br>end = %d / %d / %d", match_end_bit, match_end_byte, match_end_line);

//    printf("</td>");
    // The first line we read
    first_line = (match_start_line > css->preload_lines) ? (match_start_line - css->preload_lines) : 0;

    // The last line we read
    last_line = match_end_line + css->postload_lines;
    last_line = (last_line * bytes_per_line > flength) ? flength / bytes_per_line : last_line;

    // Allocate buffer
    read_length = (last_line - first_line + 1) * bytes_per_line;
    buf = malloc(read_length);
    if (!buf) return;
    bzero(buf, read_length);

    // Read from file
	if (fseeko(fp, first_line * bytes_per_line, SEEK_SET) != 0) {
		return;
	}
	/* Due to file boundary, we have to read data as size of 1 */
	if (fread(buf, 1, read_length, fp) != read_length) {
		/* read file error */
		return;
	}

    /* bit values */
    // printf("\t\t<td class='%s'>\n", css->td_class_bit);
    for (line = first_line; line <= last_line; line++)
    {
        if (line > first_line) printf("<br>");
        for (i = 0; i < bytes_per_line; i++)
        {
            for (j = 0; j < 8; j++)
            {
                bit_offset = ((line * bytes_per_line + i) * 8 + j) % bits_per_line;
				/* Emphasize the matched part */
				if ((line == match_start_line && bit_offset == match_start_bit)
					|| (line > match_start_line && line <= match_end_line && bit_offset == 0)) {
					printf("%s", css->matched_prefix);
				}

				/* printf the bit code */
				char bit = (buf[(line - first_line) * bytes_per_line + i] & (1 << (7 - j))) ? '1' : '0';
				printf("%c", bit);
				if (j == 7) {
					printf(" ");
				}

				if ((line >= match_start_line && line < match_end_line && (bit_offset == bits_per_line - 1))
					|| (line == match_end_line && bit_offset == match_end_bit)) {
					printf("%s", css->matched_suffix);
				}
            }
        }
    }
    // printf("</td>\n");

    // printf("</tr>\n");
    printf("\n");

    free(buf);
}



/********************************************************************
 * main function
 ********************************************************************/
/*
 * @param argv[1] the HTML CSS and the matched part
 * @param argv[2] the JSON config file for the raw files and the index file
 */
int main(int argc, const char *argv[]) {
	JSON_Value *config;
	JSON_Object *css_obj, *amatch;
	JSON_Array *matched_array;
	html_css_t css;
	FILE *fp;
	const char *filepath, *program_name;
	off_t file_length, matched_count, i;

	program_name = argv[0];

	/* Check arguments */
	if (argc != 2)
		exit_result(1, "Invalid argument");

	/* the first parameter is the matched result file */
	config = json_parse_file(argv[1]);
	if (schema == NULL) {
		schema = json_parse_string(config_schema);
	}
	if (config == NULL || schema == NULL) {
		exit_result(2, "json config file %s error", argv[1]);
	} else if (json_validate(schema, config) != JSONSuccess) {
		json_value_free(config);
		exit_result(2, "json config file %s does not match schema:", argv[1]);
	}
	css_obj = json_object_get_object(json_object(config), "css");

	// Init the HTML CSS set
	css.tr_class = json_object_get_string(css_obj, "tr_class");
	css.td_class_offset = json_object_get_string(css_obj, "td_class_offset");
	css.td_class_offset = json_object_get_string(css_obj, "td_class_offset_hex");
	css.td_class_hex = json_object_get_string(css_obj, "td_class_hex");
	css.td_class_ascii = json_object_get_string(css_obj, "td_class_ascii");
	css.td_class_bit = json_object_get_string(css_obj, "td_class_bit");
	css.matched_prefix = json_object_get_string(css_obj, "matched_prefix");
	css.matched_suffix = json_object_get_string(css_obj, "matched_suffix");
	css.preload_lines = json_object_get_number(css_obj, "preload_lines");
	css.postload_lines = json_object_get_number(css_obj, "postload_lines");

	/* Open the file */
	filepath = json_object_get_string(json_object(config), "file_path");
	if((fp = LFS_FOPEN(filepath, "rb")) == NULL) {
		exit_result(2, "cannot open raw file %s", filepath);
	}

	/* Get the file size. */
	if(LFS_FSEEK(fp, 0, SEEK_END) == 0) {
		file_length = LFS_FTELL(fp);
		rewind(fp);
		if(file_length < 0) {
			exit_result(2, "cannot ftell raw file %s", filepath);
		}
	} else {
		exit_result(2, "cannot fseek raw file %s", filepath);
	}

	/* set the number of files */
	matched_array = json_object_get_array(json_object(config), "matches");
	matched_count = json_array_get_count(matched_array);
	if ((matched_count = json_array_get_count(matched_array)) == 0) {
		exit_result(3, "no matched part");
	}

	/* Print content for each match */
//	printf("<html><body><table border='1'>");
	for (i = 0; i < matched_count; i++) {
		amatch = json_array_get_object(matched_array, i);;
		print_match_html(&css, fp, file_length,
			json_object_get_number(amatch, "offset"),
			json_object_get_number(amatch, "offset_bit"),
			json_object_get_number(amatch, "length_bit")
		);
	}
//	printf("</table></body></html>");

	/* Deallocate memory. */
	fclose(fp);

	return 0;
}

