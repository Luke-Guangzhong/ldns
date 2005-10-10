/*
 * mx is a small programs that prints out the mx records
 * for a particulary domain
 * (c) NLnet Labs, 2005
 * See the file LICENSE for the license
 */

#include "config.h"

#include <ldns/dns.h>

int
usage(FILE *fp, char *prog) {
	fprintf(fp, "%s domain\n", prog);
	fprintf(fp, "  print out the mx for domain\n");
	return 0;
}

int
main(int argc, char *argv[])
{
	ldns_resolver *res;
	ldns_rdf *domain;
	ldns_pkt *p;
	ldns_rr_list *mx;
	
	p = NULL;
	mx = NULL;
	domain = NULL;
	res = NULL;
	
	if (argc != 2) {
		usage(stdout, argv[0]);
		exit(EXIT_FAILURE);
	} else {
		/* create a rdf from the command line arg */
		domain = ldns_dname_new_frm_str(argv[1]);
		if (!domain) {
			usage(stdout, argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	/* create a new resolver from /etc/resolv.conf */
	res = ldns_resolver_new_frm_file(NULL);

	if (!res) {
		exit(EXIT_FAILURE);
	}

	/* use the resolver to send it a query for the mx 
	 * records of the domain given on the command line
	 */
	p = ldns_resolver_query(res, domain, LDNS_RR_TYPE_MX, LDNS_RR_CLASS_IN, LDNS_RD);

	ldns_rdf_deep_free(domain);
	
        if (!p)  {
		exit(EXIT_FAILURE);
        } else {
		/* retrieve the MX records from the answer section of that
		 * packet
		 */
		mx = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_MX, LDNS_SECTION_ANSWER);
		if (!mx) {
			fprintf(stderr, 
					" *** invalid answer name %s after MX query for %s\n",
					argv[1], argv[1]);
                        ldns_pkt_free(p);
                        ldns_resolver_deep_free(res);
			exit(EXIT_FAILURE);
		} else {
			/* sort the list nicely */
			/* ldns_rr_list_sort(mx); */
			/* print the rrlist to stdout */
			ldns_rr_list_print(stdout, mx);
			ldns_rr_list_deep_free(mx);
		}
        }
        ldns_pkt_free(p);
        ldns_resolver_deep_free(res);
        return 0;
}
