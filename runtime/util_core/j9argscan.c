/*******************************************************************************
 * Copyright IBM Corp. and others 1991
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <locale.h>
#if !defined(OMR_OS_WINDOWS)
#include <langinfo.h>
#endif /* !defined(OMR_OS_WINDOWS) */
#if defined(OSX)
#include <xlocale.h>
#endif /* defined(OSX) */
#include "j9.h"
#include "j9argscan.h"
#include "j9port.h"
#include "jvminit.h"
#include "omrutil.h"

/* Returns the trimmed input string, removing leading and trailing whitespace.
 * Returned string is malloc-ed and must be freed.
 * @param portLibrary The portLibrary to use
 * @param input The buffer to trim.
 */
char * trim(J9PortLibrary *portLibrary, char * input)
{
	PORT_ACCESS_FROM_PORT(portLibrary);
	BOOLEAN whitespace = TRUE;
	uintptr_t index;
	char * buf = (char *)j9mem_allocate_memory(strlen(input) + 1, OMRMEM_CATEGORY_VM);

	while (whitespace == TRUE) {
		char c = input[0];
		switch (c) {
		case '\t':
		case '\n':
		case '\r':
		case ' ':
			input++;
			break;

		default:
			whitespace = FALSE;
		}
	}

	strcpy(buf, input);
	index = strlen(buf) - 1;
	while (whitespace == TRUE) {
		char c = buf[index];
		switch (c) {
		case '\t':
		case '\n':
		case '\r':
		case ' ':
			buf[index] = 0;
			index--;
			break;

		default:
			whitespace = TRUE;
		}
	}

	return buf;
}

/* Answer string up to specified delimiter or end of line */
/* Return value is malloc-ed and must be freed by the sender */
char *scan_to_delim(J9PortLibrary *portLibrary, char **scan_start, char delimiter)
{
	char *scan_string = *scan_start, *subString;
	uintptr_t i = 0;

	PORT_ACCESS_FROM_PORT(portLibrary);

	while (scan_string[i] && (scan_string[i] != delimiter))
		i++;

	subString = j9mem_allocate_memory(i + 1 /* for null */, OMRMEM_CATEGORY_VM);
	if (subString) {
		memcpy(subString, scan_string, i);
		subString[i] = 0;
		*scan_start = (scan_string[i] ? &scan_string[i+1] : &scan_string[i]);
	}
	return subString;
}

/*
 * Print an error message indicating that an option was not recognized
 */
void scan_failed(J9PortLibrary * portLibrary, const char* module, const char *scan_start)
{
	PORT_ACCESS_FROM_PORT(portLibrary);
	j9tty_printf(PORTLIB, "<%s: unrecognized option --> '%s'>\n", module, scan_start);
}

uintptr_t scan_double(char **scan_start, double *result)
{
	char *endPtr = NULL;
	int bruteParse = 1;
#if defined(OMR_OS_WINDOWS)
	_locale_t clocale = _create_locale(LC_ALL, "C");
	if (NULL != clocale) {
		*result = _strtod_l(*scan_start, &endPtr, clocale);
		_free_locale(clocale);
		bruteParse = 0;
	}
#elif defined(LINUX) || defined(OSX) || defined(LC_ALL_MASK) /* defined(OMR_OS_WINDOWS) */
	/* AIX 7.1+ has support for newlocale() */
	locale_t clocale = newlocale(LC_ALL_MASK, "C", NULL);
	if (NULL != clocale) {
		locale_t orglocale = uselocale(clocale);
		*result = strtod(*scan_start, &endPtr);
		uselocale(orglocale);
		freelocale(clocale);
		bruteParse = 0;
	}
#endif /* defined(OMR_OS_WINDOWS) */

	/* There is no per thread locale support on AIX 6.1 or z/OS so far. */
	if (0 != bruteParse) {
		/* 256 is more than enough to hold the string double. */
		char buffer[256];
		char *dest = *scan_start;
		/* Determine the locale radix character and substitute it in the value being parsed. */
#if defined(OMR_OS_WINDOWS)
		struct lconv *linfo = localeconv();
		char *localeRadix = linfo->decimal_point;
#else /* defined(OMR_OS_WINDOWS) */
		char *localeRadix = nl_langinfo(RADIXCHAR);
#endif /* defined(OMR_OS_WINDOWS) */
		if ((NULL != localeRadix) && ('.' != *localeRadix)) {
			char *radixPos = NULL;
			size_t len = strlen(*scan_start);
			if (len >= sizeof(buffer)) {
				return OPTION_MALFORMED;
			}
			strcpy(buffer, *scan_start);
			radixPos = strstr(buffer, ".");
			if (NULL != radixPos) {
				*radixPos = *localeRadix;
			}
			dest = buffer;
		}
		*result = strtod(dest, &endPtr);
		if (dest == buffer) {
			endPtr = *scan_start + (endPtr - buffer);
		}
	}

	if (ERANGE == errno) {
		if ((HUGE_VAL == *result) || (-HUGE_VAL == *result)) {
			/* overflow */
			return OPTION_OVERFLOW;
		} else {
			/* underflow - value is so small that it cannot be represented as double.
			 * treat it as zero.
			*/
			*result = 0.0;
			return OPTION_OK;
		}
	} else if ((0.0 == *result) && (endPtr == *scan_start)) {
		/* no conversion */
		return OPTION_MALFORMED;
	}
	*scan_start = endPtr;
	return OPTION_OK;
}

/* Scan the next unsigned number off of the argument string.
 * Store the result in *result
 * Answer 0 on success
 */
uintptr_t scan_udata(char **scan_start, uintptr_t* result)
{
	/* supporting 0x prefix might be nice (or octal) */
	uintptr_t total = 0, rc = 1;
	char *c = *scan_start;

	while (OMR_ISDIGIT(*c)) {
		uintptr_t digitValue = *c - '0';

		if (total > ((uintptr_t)-1) / 10 ) {
			return 2;
		}

		total *= 10;

		if ( total > ((uintptr_t)-1) - digitValue ) {
			return 2;
		}

		total += digitValue;

		rc = 0;	/* we found at least one digit */

		c++;
	}

	*scan_start = c;
	*result = total;
	return rc;
}

/* Scan the next unsigned number off of the argument string.
 * Store the result in *result
 * Answer 0 on success
 */
uintptr_t
scan_u64(char **scan_start, uint64_t* result)
{
	/* supporting 0x prefix might be nice (or octal) */
	uint64_t total = 0;
	uintptr_t rc = 1;
	char *c = *scan_start;

	while (OMR_ISDIGIT(*c)) {
		uintptr_t digitValue = *c - '0';

		if (total > ((uint64_t)-1) / 10 ) {
			return 2;
		}

		total *= 10;

		if ( total > ((uint64_t)-1) - digitValue ) {
			return 2;
		}

		total += digitValue;

		rc = 0;	/* we found at least one digit */

		c++;
	}

	*scan_start = c;
	*result = total;
	return rc;
}

/* Scan the next unsigned number off of the argument string.
 * Store the result in *result
 * Answer 0 on success
 */
uintptr_t
scan_u32(char **scan_start, uint32_t* result)
{
	/* supporting 0x prefix might be nice (or octal) */
	uint32_t total = 0;
	uintptr_t rc = 1;
	char *c = *scan_start;

	while (OMR_ISDIGIT(*c)) {
		uint32_t digitValue = *c - '0';

		if (total > ((uint32_t)-1) / 10 ) {
			return 2;
		}

		total *= 10;

		if ( total > ((uint32_t)-1) - digitValue ) {
			return 2;
		}

		total += digitValue;

		rc = 0;	/* we found at least one digit */

		c++;
	}

	*scan_start = c;
	*result = total;
	return rc;
}

/* Scan the next signed number off of the argument string.
 * Store the result in *result
 * Answer 0 on success
 */
uintptr_t scan_idata(char **scan_start, intptr_t *result)
{
	uintptr_t rc;
	char* new_scan_start = *scan_start;

	char c = *new_scan_start;

	if (c == '+' || c == '-') {
		new_scan_start++;
	}

	rc = scan_udata(&new_scan_start, (uintptr_t *)result);

	if (rc == 0) {
		if (*result < 0) {
			if ( ( (uintptr_t)*result  == (uintptr_t)1 << (sizeof(uintptr_t) * 8 - 1) ) && ( c == '-' ) ) {
				/* this is MIN_UDATA */
				rc = 0;
			} else {
				rc = 2;
			}
		} else if (c == '-') {
			*result = -(*result);
		}
	}

	if (rc == 0) {
		*scan_start = (char *)new_scan_start;
	}

	return rc;
}


/* Scan the next hexadecimal unsigned number off of the argument string.
 * Store the result in *result
 * Answer 0 on success
 */
uintptr_t scan_hex(char **scan_start, uintptr_t* result)
{
	return	scan_hex_caseflag(scan_start, TRUE, result);
}

/* Scan the next hexadecimal (only in lower cases if uppercaseAllowed is false) unsigned number off of the argument string.
 * Store the result in *result
 * Answer 0 on success
 */
uintptr_t scan_hex_caseflag(char **scan_start, BOOLEAN uppercaseAllowed, uintptr_t* result)
{
	uintptr_t total = 0, delta = 0, rc = 1;
	char *hex = *scan_start, x;

	try_scan(&hex, "0x");

	while ( (x = *hex) != '\0' ) {
		/*
		 * Decode hex digit
		 */
		if (x >= '0' && x <= '9') {
			delta = (x - '0');
		} else if (x >= 'a' && x <= 'f') {
			delta = 10 + (x - 'a');
		} else if (x >= 'A' && x <= 'F' && uppercaseAllowed) {
			delta = 10 + (x - 'A');
		} else {
			break;
		}

		total = (total << 4) + delta;

		hex++; rc = 0;
	}

	*scan_start = hex;
	*result = total;

	return rc;
}

/**
 * @Scan the hexadecimal uint64_t number and store the result in *result
 * @param[in] scan_start The string to be scanned
 * @param[in] uppercaseFalg Whether upper case letter is allowed
 * @param[out] result The result
 * @return the number of bits used to store the hexadecimal number or 0 on failure.
 */
uintptr_t
scan_hex_u64(char **scan_start, uint64_t* result)
{
	return	scan_hex_caseflag_u64(scan_start, TRUE, result);
}

/**
 * Scan the hexadecimal uint64_t number and store the result in *result 
 * @param[in] scan_start The string to be scanned
 * @param[in] uppercaseFalg Whether uppercase letter is allowed
 * @param[out] result The result
 * @return the number of bits used to store the hexadecimal number or 0 on failure.
 */
uintptr_t
scan_hex_caseflag_u64(char **scan_start, BOOLEAN uppercaseAllowed, uint64_t* result)
{
	uint64_t total = 0;
	uint64_t delta = 0;
	char *hex = *scan_start;
	uintptr_t bits = 0;

	try_scan(&hex, "0x");

	while (('\0' != *hex)
			&& (bits <= 60)
	) {
		/*
		 * Decode hex digit
		 */
		char x = *hex;
		if (x >= '0' && x <= '9') {
			delta = (x - '0');
		} else if (x >= 'a' && x <= 'f') {
			delta = 10 + (x - 'a');
		} else if (x >= 'A' && x <= 'F' && uppercaseAllowed) {
			delta = 10 + (x - 'A');
		} else {
			break;
		}

		total = (total << 4) + delta;
		bits += 4;
		hex++;
	}

	*scan_start = hex;
	*result = total;

	return bits;
}

/**
 * Scan the next unsigned number off of the argument string, and parses for memory sizes.
 * Specify the size in TiBs, GiBs, MiBs, or KiBs (with T,t,G,g,M,m,K,k suffixes) or in bytes (no suffix).
 * @param[in] scan_start The string to be scanned
 * @param[out] result The result
 * @return returns 0 on success, 1 if the argument string is not a number, or 2 if overflow occurs.
 */
uintptr_t
scan_u64_memory_size(char **scan_start, uint64_t* result)
{
	uintptr_t rc = scan_u64(scan_start, result);

	if (0 == rc) {
		if (try_scan(scan_start, "T") || try_scan(scan_start, "t")) {
			if (*result <= (((U_64)-1) >> 40)) {
				*result <<= 40;
			} else {
				rc = 2;
			}
		} else if (try_scan(scan_start, "G") || try_scan(scan_start, "g")) {
			if (*result <= (((U_64)-1) >> 30)) {
				*result <<= 30;
			} else {
				rc = 2;
			}
		} else if (try_scan(scan_start, "M") || try_scan(scan_start, "m")) {
			if (*result <= (((U_64)-1) >> 20)) {
				*result <<= 20;
			} else {
				rc = 2;
			}
		} else if (try_scan(scan_start, "K") || try_scan(scan_start, "k")) {
			if (*result <= (((U_64)-1) >> 10)) {
				*result <<= 10;
			} else {
				rc = 2;
			}
		}
	}
	return rc;
}

/**
 * Scan the next unsigned number off of the argument string, and parses for memory sizes.
 * Specify the size in TiBs, GiBs, MiBs, or KiBs (with T,t,G,g,M,m,K,k suffixes) or in bytes (no suffix).
 * @param[in] scan_start The string to be scanned
 * @param[out] result The result
 * @return returns 0 on success, 1 if the argument string is not a number, or 2 if overflow occurs.
 */
uintptr_t
scan_udata_memory_size(char **scan_start, uintptr_t* result)
{
	uintptr_t rc = scan_udata(scan_start, result);

	if (0 == rc) {
		/* Scan Memory String, and check for overflow */
		if (try_scan(scan_start, "T") || try_scan(scan_start, "t")) {
			if (0 != *result) {
	#if defined(J9VM_ENV_DATA64)
				if (*result <= (((UDATA)-1) >> 40)) {
					*result <<= 40;
				} else
	#endif /* defined(J9VM_ENV_DATA64) */
				{
					rc = 2;
				}
			}
		} else if (try_scan(scan_start, "G") || try_scan(scan_start, "g")) {
			if (*result <= (((UDATA)-1) >> 30)) {
				*result <<= 30;
			} else {
				rc = 2;
			}
		} else if (try_scan(scan_start, "M") || try_scan(scan_start, "m")) {
			if (*result <= (((UDATA)-1) >> 20)) {
				*result <<= 20;
			} else {
				rc = 2;
			}
		} else if (try_scan(scan_start, "K") || try_scan(scan_start, "k")) {
			if (*result <= (((UDATA)-1) >> 10)) {
				*result <<= 10;
			} else {
				rc = 2;
			}
		}
	}

	return rc;
}

/*
 * Print an error message indicating that an option was not recognized
 */
void scan_failed_incompatible(J9PortLibrary * portLibrary, char* module, char *scan_start)
{
	PORT_ACCESS_FROM_PORT(portLibrary);
	j9tty_printf(PORTLIB, "<%s: incompatible option --> '%s'>\n", module, scan_start);
}


/*
 * Print an error message indicating that an option was not recognized
 */
void scan_failed_unsupported(J9PortLibrary * portLibrary, char* module, char *scan_start)
{
	PORT_ACCESS_FROM_PORT(portLibrary);
	j9tty_printf(PORTLIB, "<%s: system configuration does not support option --> '%s'>\n", module, scan_start);
}



