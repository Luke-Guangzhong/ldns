/*
 * str2host.c
 *
 * conversion routines from the presentation format
 * to the host format
 *
 * a Net::DNS like library for C
 *
 * (c) NLnet Labs, 2004
 *
 * See the file LICENSE for the license
 */
#include <config.h>

#include <ldns/str2host.h>
#include <ldns/dns.h>
#include <ldns/rdata.h>

#include "util.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#include <limits.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

ldns_status
ldns_str2rdf_int16(ldns_rdf **rd, const char *shortstr)
{
	char *end = NULL;    
	uint16_t *r;
	r = MALLOC(uint16_t);
	
	*r = htons((uint16_t)strtol((char *)shortstr, &end, 0));
	
	if(*end != 0) {
		FREE(r);
		return LDNS_STATUS_INT_EXP;
	} else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_INT16, sizeof(uint16_t), r);
		FREE(r);
		return LDNS_STATUS_OK;
	}
}

ldns_status
ldns_str2rdf_time(ldns_rdf **rd, const char *time)
{
	/* convert a time YYYYDDMMHHMMSS to wireformat */
	uint16_t *r = NULL;
	struct tm tm;
	uint32_t l;
	char *end;

	/* Try to scan the time... */
	r = (uint16_t*)MALLOC(uint32_t);

	if((char*)strptime(time, "%Y%m%d%H%M%S", &tm) == NULL) {
		/* handle it as 32 bits */
		l = htonl((uint32_t)strtol((char*)time, &end, 0));
		if(*end != 0) {
			FREE(r);
			return LDNS_STATUS_ERR;
		} else {
			memcpy(r, &l, sizeof(uint32_t));
			*rd = ldns_rdf_new_frm_data(
				LDNS_RDF_TYPE_INT32, sizeof(uint32_t), r);
			FREE(r);
			return LDNS_STATUS_OK;
		}
	} else {
		l = htonl(timegm(&tm));
		memcpy(r, &l, sizeof(uint32_t));
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_TIME, sizeof(uint32_t), r);
		FREE(r);
		return LDNS_STATUS_OK;
	}
}

ldns_status
ldns_str2rdf_period(ldns_rdf **rd,const char *period)
{
        uint32_t p;
        const char *end;

        /* Allocate required space... */
        p = ldns_str2period(period, &end);

        if (*end != 0) {
		return LDNS_STATUS_ERR;
        } else {
                p = (uint32_t) htonl(p);
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_PERIOD, sizeof(uint32_t), &p);
        }
	return LDNS_STATUS_OK;
}

ldns_status 
ldns_str2rdf_int32(ldns_rdf **rd, const char *longstr)
{
	char *end;  
	uint16_t *r = NULL;
	uint32_t l;

	r = (uint16_t*)MALLOC(uint32_t);
	l = htonl((uint32_t)strtol((char*)longstr, &end, 0));

	if(*end != 0) {
		FREE(r);
		return LDNS_STATUS_ERR;
        } else {
		memcpy(r, &l, sizeof(uint32_t));
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_INT32, sizeof(uint32_t), r);
		FREE(r);
		return LDNS_STATUS_OK;
	}
}

ldns_status
ldns_str2rdf_int8(ldns_rdf **rd, const char *bytestr)
{
	char *end;     
	uint8_t *r = NULL;

	r = MALLOC(uint8_t);
 
	*r = (uint8_t)strtol((char*)bytestr, &end, 0);

        if(*end != 0) {
		FREE(r);
		return LDNS_STATUS_ERR;
        } else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_INT8, sizeof(uint8_t), r);
		FREE(r);
		return LDNS_STATUS_OK;
        }
}

/**
 * No special care is taken, all dots are translated into
 * label seperators.
 * \todo make this more efficient...
 * we do 3 memcpy's in total...
 * label_chars2 is used for debugging. TODO: remove
 */
ldns_status
ldns_str2rdf_dname(ldns_rdf **d, const char *str)
{
	size_t len;

	uint8_t *s,*p,*q, *pq, val, label_len;
	uint8_t buf[MAX_DOMAINLEN + 1];
	*d = NULL;
	
	len = strlen((char*)str);
	if (len > MAX_DOMAINLEN) {
		return LDNS_STATUS_DOMAINNAME_OVERFLOW;
	}
	if (0 == len) {
		return LDNS_STATUS_DOMAINNAME_UNDERFLOW;
	} 
	
	/* root label */
	if (1 == len) {
		*d = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_DNAME, 1, "\0"); 
		return LDNS_STATUS_OK;
	}

	/* get on with the rest */

	/* s is on the current dot
	 * p on the previous one
	 * q builds the dname
	 */
	len = 0;
	q = buf+1;
	pq = buf;
	label_len = 0;
	for (s = p = (uint8_t *) str; *s; s++, q++) {
		*q = 0;
		switch (*s) {
		case '.':
			/* todo: check length (overflow und <1 */
			if (label_len > MAX_LABELLEN) {
				return LDNS_STATUS_LABEL_OVERFLOW;
			}
			if (label_len == 0) {
				return LDNS_STATUS_EMPTY_LABEL;
			}
			len += label_len + 1;
			*pq = label_len;
			label_len = 0;
			pq = q;
			p = s+1;
			break;
		case '\\':
			/* octet value or literal char */
			if (isdigit((int) s[1]) &&
			    isdigit((int) s[2]) &&
			    isdigit((int) s[3])) {
				/* cast this so it fits */
				val = (uint8_t) hexdigit_to_int((char) s[1]) * 100 +
				                hexdigit_to_int((char) s[2]) * 10 +
				                hexdigit_to_int((char) s[3]);
				*q = val;
				s += 3;
			} else {
				s++;
				*q = *s;
			}
			label_len++;
			break;
		default:
			*q = *s;
			label_len++;
		}
	}

	/* add root label if last char was not '.' */
	if (str[strlen(str)-1] != '.') {
		len += label_len + 1;
		*pq = label_len;
		*q = 0;
	}
	len++;

	/* s - buf_str works because no magic is done in the above for-loop */
	*d = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_DNAME, len, buf); 
	
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_a(ldns_rdf **rd, const char *str)
{
	in_addr_t address;
        if (inet_pton(AF_INET, (char*)str, &address) != 1) {
                return LDNS_STATUS_INVALID_IP4;
        } else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_A, sizeof(address), &address);
        }
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_aaaa(ldns_rdf **rd, const char *str)
{
	uint8_t address[LDNS_IP6ADDRLEN + 1];

	if (inet_pton(AF_INET6, (char*)str, address) != 1) {
		return LDNS_STATUS_INVALID_IP6;
	} else {
		/* MIEK */
		printf("str2rdf aaaa str [%s] %d\n", str, sizeof(address));
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_AAAA, sizeof(address), &address);
	}
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_str(ldns_rdf **rd, const char *str)
{
	if (strlen(str) > 255) {
		return LDNS_STATUS_INVALID_STR;
	}
	*rd = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_STR, strlen(str), str);
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_apl(ldns_rdf **rd, const char *str)
{
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

ldns_status
ldns_str2rdf_b64(ldns_rdf **rd, const char *str)
{
	uint8_t *buffer;
	int16_t i;
	
	buffer = XMALLOC(uint8_t, b64_ntop_calculate_size(strlen(str)));
	
	i = (uint16_t) b64_pton((const char*)str, buffer, 
	                        b64_ntop_calculate_size(strlen(str)));
	if (-1 == i) {
		return LDNS_STATUS_INVALID_B64;
	} else {
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_B64, (uint16_t) i, buffer);
	}
	FREE(buffer);

	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_hex(ldns_rdf **rd, const char *str)
{
        uint8_t *t;
        int i;
	size_t len;

	len = strlen(str);

        if (len % 2 != 0) {
                return LDNS_STATUS_INVALID_HEX;
        } else if (len > MAX_RDFLEN * 2) {
		return LDNS_STATUS_LABEL_OVERFLOW;
        } else {
		t = XMALLOC(uint8_t, (len / 2));
    
                /* Now process octet by octet... */
                while (*str) {
                        *t = 0;
                        for (i = 16; i >= 1; i -= 15) {
                                if (isxdigit(*str)) {
                                        *t += hexdigit_to_int(*str) * i;
                                } else {
                                        return LDNS_STATUS_ERR;
                                }
                                ++str;
                        }
                        ++t;
                }
		*rd = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_HEX, len / 2, t);
        }
        return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_nsec(ldns_rdf **rd, const char *str)
{
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

ldns_status
ldns_str2rdf_type(ldns_rdf **rd, const char *str)
{
	uint16_t type;
	type = htons(ldns_get_rr_type_by_name(str));
	/* ldns_rr_type is a 16 bit value */
	*rd = ldns_rdf_new_frm_data(
		LDNS_RDF_TYPE_TYPE, sizeof(uint16_t), &type);
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_class(ldns_rdf **rd, const char *str)
{
	uint16_t klass;
	klass = htons(ldns_get_rr_class_by_name(str));
	/* class is 16 bit */
	*rd = ldns_rdf_new_frm_data(
		LDNS_RDF_TYPE_CLASS, sizeof(uint16_t), &klass);
	return LDNS_STATUS_OK;
}

ldns_status
ldns_str2rdf_cert(ldns_rdf **rd, const char *str)
{
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

/* An alg field can either be specified as a 8 bits number
 * or by its symbolic name. Handle both
 */
ldns_status
ldns_str2rdf_alg(ldns_rdf **rd, const char *str)
{
	ldns_lookup_table *lt;
	ldns_status st;

	lt = ldns_lookup_by_name(ldns_algorithms, str);
	st = LDNS_STATUS_OK;

	if (lt) {
		/* it was given as a integer */
		*rd = ldns_rdf_new_frm_data(
			LDNS_RDF_TYPE_INT8, sizeof(uint8_t), &lt->id);
		if (!*rd) {
			st = LDNS_STATUS_ERR;
		}
	} else {
		/* try as-is (a number) */
		st = ldns_str2rdf_int8(rd, str);
	}
	return st;
}
		
ldns_status
ldns_str2rdf_unknown(ldns_rdf **rd, const char *str)
{
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

ldns_status
ldns_str2rdf_tsig(ldns_rdf **rd, const char *str)
{
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

ldns_status
ldns_str2rdf_service(ldns_rdf **rd, const char *str)
{
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

ldns_status
ldns_str2rdf_loc(ldns_rdf **rd, const char *str)
{
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

ldns_status
ldns_str2rdf_wks(ldns_rdf **rd, const char *str)
{
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}

ldns_status
ldns_str2rdf_nsap(ldns_rdf **rd, const char *str)
{
	rd = rd;
	str = str;
	return LDNS_STATUS_NOT_IMPL;
}
