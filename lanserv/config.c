/*
 * config.c
 *
 * MontaVista IPMI code for reading lanserv configuration files.
 *
 * Author: MontaVista Software, Inc.
 *         Corey Minyard <minyard@mvista.com>
 *         source@mvista.com
 *
 * Copyright 2002,2003,2004,2005 MontaVista Software Inc.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * Lesser General Public License (GPL) Version 2 or the modified BSD
 * license below.  The following disclamer applies to both licenses:
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * GNU Lesser General Public Licence
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Modified BSD Licence
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. The name of the author may not be used to endorse or promote
 *      products derived from this software without specific prior
 *      written permission.
 */

#include <config.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <netdb.h>

#include <OpenIPMI/serv.h>
#include <OpenIPMI/lanserv.h>
#include <OpenIPMI/serserv.h>

/*
 * To parse more complex expressions, we really need to know what the
 * save state is.  So we, unfortunately, have to create our own
 * version of strtok so we know what it is.
 */
char *
mystrtok(char *str, const char *delim, char **next)
{
    char *pos;
    char *curr;

    if (str)
	curr = str;
    else
	curr = *next;

    /* Skip initial delimiters. */
    for (;;) {
	const char *c = delim;
	if (*curr == '\0') {
	    *next = curr;
	    return NULL;
	}

	while (*c != '\0') {
	    if (*c == *curr)
		break;
	    c++;
	}
	if (*c == '\0')
	    break;
	curr++;
    }

    pos = curr;
    /* Now collect until there is a delimiter. */
    for (;;) {
	const char *c = delim;
	if (*curr == '\0') {
	    *next = curr;
	}
	while (*c != '\0') {
	    if (*c == *curr) {
		*curr = '\0';
		*next = curr + 1;
		goto out;
	    }
	    c++;
	}
	curr++;
    }
 out:
    return pos;
}

int
get_delim_str(char **rtokptr, char **rval, char **err)
{
    char *tokptr = *rtokptr;
    char endc;
    char *rv;

    while (isspace(*tokptr))
	tokptr++;
    if (*tokptr == '\0') {
	*err = "missing string value";
	return -1;
    }
    if (*tokptr != '"' && *tokptr != '\'') {
	*err = "string value must start with '\"' or '''";
	return -1;
    }
    endc = *tokptr;
    tokptr++;
    rv = tokptr;
    while (*tokptr != endc) {
	if (*tokptr == '\0') {
	    *err = "End of line in string";
	    return -1;
	}
	tokptr++;
    }
    *tokptr = '\0';
    *rtokptr = tokptr + 1;
    rv = strdup(rv);
    if (!rv) {
	*err = "Out of memory copying string";
	return -1;
    }
    *rval = rv;
    return 0;
}

int
get_bool(char **tokptr, unsigned int *rval, char **err)
{
    char *tok = mystrtok(NULL, " \t\n", tokptr);

    if (!tok)
	return -1;
    if (strcasecmp(tok, "true") == 0)
	*rval = 1;
    else if (strcasecmp(tok, "false") == 0)
	*rval = 0;
    else if (strcasecmp(tok, "on") == 0)
	*rval = 1;
    else if (strcasecmp(tok, "off") == 0)
	*rval = 0;
    else {
	*err = "Invalid boolean value, must be 'true', 'on', 'false', or 'off'";
	return -1;
    }

    return 0;
}

int
get_uint(char **tokptr, unsigned int *rval, char **err)
{
    char *end;
    char *tok = mystrtok(NULL, " \t\n", tokptr);

    *rval = strtoul(tok, &end, 0);
    if (*end != '\0') {
	*err = "Invalid integer value";
	return -1;
    }
    return 0;
}

int
get_priv(char **tokptr, unsigned int *rval, char **err)
{
    char *tok = mystrtok(NULL, " \t\n", tokptr);

    if (!tok) {
	*err = "No privilege specified, must be 'callback', 'user',"
	    " 'operator', or 'admin'";
	return -1;
    }
    if (strcmp(tok, "callback") == 0)
	*rval = IPMI_PRIVILEGE_CALLBACK;
    else if (strcmp(tok, "user") == 0)
	*rval = IPMI_PRIVILEGE_USER;
    else if (strcmp(tok, "operator") == 0)
	*rval = IPMI_PRIVILEGE_OPERATOR;
    else if (strcmp(tok, "admin") == 0)
	*rval = IPMI_PRIVILEGE_ADMIN;
    else {
	*err = "Invalid privilege specified, must be 'callback', 'user',"
	    " 'operator', or 'admin'";
	return -1;
    }

    return 0;
}

int
get_auths(char **tokptr, unsigned int *rval, char **err)
{
    char *tok = mystrtok(NULL, " \t\n", tokptr);
    int  val = 0;

    while (tok) {
	if (strcmp(tok, "none") == 0)
	    val |= (1 << IPMI_AUTHTYPE_NONE);
	else if (strcmp(tok, "md2") == 0)
	    val |= (1 << IPMI_AUTHTYPE_MD2);
	else if (strcmp(tok, "md5") == 0)
	    val |= (1 << IPMI_AUTHTYPE_MD5);
	else if (strcmp(tok, "straight") == 0)
	    val |= (1 << IPMI_AUTHTYPE_STRAIGHT);
	else {
	    *err = "Invalid authorization type, must be 'none', 'md2',"
		" 'md5', or 'straight'";
	    return -1;
	}

	tok = mystrtok(NULL, " \t\n", tokptr);
    }

    *rval = val;

    return 0;
}

int
read_bytes(char **tokptr, unsigned char *data, char **err, unsigned int len)
{
    char *tok = mystrtok(NULL, " \t\n", tokptr);
    char *end;

    if (!tok) {
	*err = "Missing password or username";
	return -1;
    }
    if (*tok == '"') {
	int end;
	/* Ascii PW */
	tok++;
	end = strlen(tok) - 1;
	if (tok[end] != '"') {
	    *err = "ASCII password or username doesn't end in '\"'";
	    return -1;
	}
	tok[end] = '\0';
	strncpy((char *) data, tok, len);
	zero_extend_ascii(data, len);
    } else {
	unsigned int i;
	char         c[3];
	/* HEX pw */
	if (strlen(tok) != 32) {
	    *err = "HEX password or username not 32 HEX characters long";
	    return -1;
	}
	c[2] = '\0';
	for (i=0; i<len; i++) {
	    c[0] = *tok;
	    tok++;
	    c[1] = *tok;
	    tok++;
	    data[i] = strtoul(c, &end, 16);
	    if (*end != '\0') {
		*err = "Invalid HEX character in password or username";
		return -1;
	    }
	}
    }

    return 0;
}

int
get_sock_addr(char **tokptr, sockaddr_ip_t *addr, socklen_t *len,
	      char *def_port, int socktype, char **err)
{
    char *s, *p;

    s = mystrtok(NULL, " \t\n", tokptr);
    if (!s) {
	*err = "No IP address specified";
	return -1;
    }
    p = mystrtok(NULL, " \t\n", tokptr);

#ifdef HAVE_GETADDRINFO
    {
	struct addrinfo     hints, *res0;
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = socktype;
	hints.ai_flags = AI_PASSIVE;
	if (!p)
	    p = def_port;
	if (!p) {
	    *err = "No port specified";
	    return -1;
	}
	rv = getaddrinfo(s, p, &hints, &res0);
	if (rv) {
	    *err = "getaddrinfo err";
	    return -1;
	}
	memcpy(addr, res0->ai_addr, res0->ai_addrlen);
	*len = res0->ai_addrlen;
	freeaddrinfo(res0);
    }
#else
    /* System does not support getaddrinfo, just for IPv4*/
    {
	struct hostent     *ent;
	struct sockaddr_in *paddr;
        char               *end;

	ent = gethostbyname(s);
	if (!ent) {
	    *err = "Invalid IP address specified";
	    return -1;
	}

	paddr = (struct sockaddr_in *)addr;
	paddr->sin_family = AF_INET;
	if (p) {
	    paddr->sin_port = htons(strtoul(p, &end, 0));
	    if (*end != '\0') {
	        *err = "Invalid IP port specified";
	        return -1;
	    }
	} else {
	    paddr->sin_port = htons(623);
	}
	*len = sizeof(struct sockaddr_in);
    }
#endif
    return 0;
}

static int
get_user(char **tokptr, bmc_data_t *bmc, char **err)
{
    unsigned int num;
    unsigned int val;
    int          rv;

    rv = get_uint(tokptr, &num, err);
    if (rv)
	return rv;

    if (num > MAX_USERS) {
	*err = "User number larger than the allowed number of users";
	return -1;
    }

    rv = get_bool(tokptr, &val, err);
    if (rv)
	return rv;
    bmc->users[num].valid = val;

    rv = read_bytes(tokptr, bmc->users[num].username, err, 16);
    if (rv)
	return rv;

    rv = read_bytes(tokptr, bmc->users[num].pw, err, 20);
    if (rv)
	return rv;

    rv = get_priv(tokptr, &val, err);
    if (rv)
	return rv;
    bmc->users[num].privilege = val;

    rv = get_uint(tokptr, &val, err);
    if (rv)
	return rv;
    bmc->users[num].max_sessions = val;

    rv = get_auths(tokptr, &val, err);
    if (rv)
	return rv;
    bmc->users[num].allowed_auths = val;

    return 0;
}

int
read_config(bmc_data_t *bmc,
	    char       *config_file)
{
    FILE         *f = fopen(config_file, "r");
    int          line;
    unsigned int val;
    char         buf[MAX_CONFIG_LINE];
    char         *tok;
    char         *tokptr;
    int          err = 0;
    char         *errstr;

    if (!f) {
	fprintf(stderr, "Unable to open configuration file '%s'\n",
		config_file);
	return -1;
    }

    line = 0;
    while (fgets(buf, sizeof(buf), f) != NULL) {
	line++;

	tok = mystrtok(buf, " \t\n", &tokptr);
	if (!tok || (tok[0] == '#'))
	    continue;

	if (strcmp(tok, "startlan") == 0) {
	    err = get_uint(&tokptr, &val, &errstr);
	    if (!err && (val >= IPMI_MAX_CHANNELS)) {
		err = -1;
		errstr = "Channel number out of range";
	    }
	    if (!err && bmc->channels[val]) {
		err = -1;
		errstr = "Channel already in use";
	    }
	    if (!err) {
		err = lanserv_read_config(bmc, f, &line, val);
		if (err)
		    return err;
	    }
	} else if (strcmp(tok, "user") == 0) {
	    err = get_user(&tokptr, bmc, &errstr);
	} else if (strcmp(tok, "serial") == 0) {
	    err = serserv_read_config(&tokptr, bmc, &errstr);
	} else if (strcmp(tok, "startcmd") == 0) {
	    err = get_delim_str(&tokptr, &bmc->startcmd, &errstr);
	} else if (strcmp(tok, "console") == 0) {
	    err = get_sock_addr(&tokptr,
				&bmc->console_addr, &bmc->console_addr_len,
				NULL, SOCK_STREAM, &errstr);
	} else {
	    errstr = "Invalid configuration option";
	    err = -1;
	}

	if (err) {
	    fprintf(stderr, "Error on line %d: %s\n", line, errstr);
	    return err;
	}
    }

    fclose(f);
    return 0;
}
