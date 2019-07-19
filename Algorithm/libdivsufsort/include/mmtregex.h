/*
 * mmtregex.h
 *
 *  Created on: Jul 27, 2016
 *      Author: Abe Wu
 */

#ifndef LIB_MMTREGEX_H_
#define LIB_MMTREGEX_H_

typedef struct mmtregex_s mmtregex_t, *mmtregex;

/* >0 means match */
#define MMTREGEX_MATCH_FAIL -1
#define MMTREGEX_MATCH_SUCCESS 0

/* Error code */
#define MMTREGEX_OK 0
#define MMTREGEX_EPARAM 1 /*<< Invalid input parameter */
#define MMTREGEX_EREGEX 2 /*<< regex is not supported */
#define MMTREGEX_ENOMEM 3 /*<< Not enough memory */

void delete_mmtregex(mmtregex preg);
mmtregex mmtregex_from_str(const uint8_t *regex_str, size_t regex_len);
int mmtregex_match(mmtregex preg, const uint8_t *str, size_t len);
int mmtregex_get_matched_length(mmtregex preg);
int mmtregex_get_maximum_length(mmtregex preg);
int mmtregex_get_next_match_char(mmtregex preg, uint32_t offset, uint8_t charbeg);

#endif /* LIB_MMTREGEX_H_ */
