/* Copyright (C) 2001-2004 Bart Massey and Jamey Sharp.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 */

/* Authorization systems for the X protocol. */

#include <assert.h>
#include <X11/Xauth.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>

#include "xcb.h"
#include "xcbint.h"

#ifdef HAS_AUTH_XA1
#include "xcb_des.h"
#endif

enum auth_protos {
#ifdef HAS_AUTH_XA1
    AUTH_XA1,
#endif
    AUTH_MC1,
    N_AUTH_PROTOS
};

static char *authnames[N_AUTH_PROTOS] = {
#ifdef HAS_AUTH_XA1
    "XDM-AUTHORIZATION-1",
#endif
    "MIT-MAGIC-COOKIE-1",
};

#ifdef HAS_AUTH_XA1

static int next_nonce(void)
{
    static int nonce = 0;
    static pthread_mutex_t nonce_mutex = PTHREAD_MUTEX_INITIALIZER;
    int ret;
    pthread_mutex_lock(&nonce_mutex);
    ret = nonce++;
    pthread_mutex_unlock(&nonce_mutex);
    return ret;
}

/*
 * This code and the code it calls is taken from libXdmcp,
 * specifically from Wrap.c, Wrap.h, and Wraphelp.c.  The US
 * has changed, thank goodness, and it should be OK to bury
 * DES code in an open source product without a maze of
 * twisty wrapper functions stored offshore.  Or maybe
 * not. --Bart Massey 2003/11/5
 */

static void
Wrap (
    des_cblock	        input,
    des_cblock          key,
    des_cblock          output,
    int			bytes)
{
    int			i, j;
    int			len;
    des_cblock          tmp;
    des_cblock          expand_key;
    des_key_schedule	schedule;

    XCBDESKeyToOddParity (key, expand_key);
    XCBDESKeySchedule (expand_key, schedule);
    for (j = 0; j < bytes; j += 8)
    {
	len = 8;
	if (bytes - j < len)
	    len = bytes - j;
	/* block chaining */
	for (i = 0; i < len; i++)
	{
	    if (j == 0)
		tmp[i] = input[i];
	    else
		tmp[i] = input[j + i] ^ output[j - 8 + i];
	}
	for (; i < 8; i++)
	{
	    if (j == 0)
		tmp[i] = 0;
	    else
		tmp[i] = 0 ^ output[j - 8 + i];
	}
	XCBDESEncrypt (tmp, (output + j), schedule, 1);
    }
}

#endif

static size_t memdup(char **dst, void *src, size_t len)
{
    if(len)
	*dst = malloc(len);
    else
	*dst = 0;
    if(!*dst)
	return 0;
    memcpy(*dst, src, len);
    return len;
}

static int authname_match(enum auth_protos kind, char *name, int namelen)
{
    if(strlen(authnames[kind]) != namelen)
	return 0;
    if(memcmp(authnames[kind], name, namelen))
	return 0;
    return 1;
}

static Xauth *get_authptr(struct sockaddr *sockname, unsigned int socknamelen)
{
    char *addr = 0;
    int addrlen = 0;
    unsigned short family;
    char hostnamebuf[256];   /* big enough for max hostname */
    char dispbuf[40];   /* big enough to hold more than 2^64 base 10 */
    char *display;
    int authnamelens[N_AUTH_PROTOS];
    int i;

    family = FamilyLocal; /* 256 */
    switch (sockname->sa_family) {
    case AF_INET:
	/*block*/ {
             struct sockaddr_in *si = (struct sockaddr_in *) sockname;
	     assert(sizeof(*si) == socknamelen);
	     addr = (char *) &si->sin_addr;
	     addrlen = 4;
	     if (ntohl(si->sin_addr.s_addr) != 0x7f000001)
		 family = FamilyInternet; /* 0 */
	     snprintf(dispbuf, sizeof(dispbuf), "%d", ntohs(si->sin_port) - X_TCP_PORT);
	     display = dispbuf;
        }
	break;
    case AF_UNIX:
	/*block*/ { 
	    struct sockaddr_un *su = (struct sockaddr_un *) sockname;
	    assert(sizeof(*su) >= socknamelen);
	    display = strrchr(su->sun_path, 'X');
	    if (display == 0)
		return 0;   /* sockname is mangled somehow */
	    display++;
	}
	break;
    default:
        return 0;   /* cannot authenticate this family */
    }
    if (family == FamilyLocal) {
        if (gethostname(hostnamebuf, sizeof(hostnamebuf)) == -1)
            return 0;   /* do not know own hostname */
        addr = hostnamebuf;
        addrlen = strlen(addr);
    }

    for (i = 0; i < N_AUTH_PROTOS; i++)
	authnamelens[i] = strlen(authnames[i]);
    return XauGetBestAuthByAddr (family,
                                 (unsigned short) addrlen, addr,
                                 (unsigned short) strlen(display), display,
                                 N_AUTH_PROTOS, authnames, authnamelens);
}

#ifdef HAS_AUTH_XA1
static void do_append(char *buf, int *idxp, void *val, size_t valsize) {
    memcpy(buf + *idxp, val, valsize);
    *idxp += valsize;
}
#endif
     
static int compute_auth(XCBAuthInfo *info, Xauth *authptr, struct sockaddr *sockname)
{
    if (authname_match(AUTH_MC1, authptr->name, authptr->name_length)) {
        info->datalen = memdup(&info->data, authptr->data, authptr->data_length);
        if(!info->datalen)
            return 0;
        return 1;
    }
#ifdef HAS_AUTH_XA1
#define APPEND(buf,idx,val) do_append((buf),&(idx),(val),sizeof(val))
    if (authname_match(AUTH_XA1, authptr->name, authptr->name_length)) {
	int j;

	info->data = malloc(192 / 8);
	if(!info->data)
	    return 0;

	for (j = 0; j < 8; j++)
	    info->data[j] = authptr->data[j];
	switch(sockname->sa_family) {
        case AF_INET:
            /*block*/ {
	    struct sockaddr_in *si = (struct sockaddr_in *) sockname;
	    APPEND(info->data, j, si->sin_addr.s_addr);
	    APPEND(info->data, j, si->sin_port);
	}
	break;
        case AF_UNIX:
            /*block*/ {
	    long fakeaddr = htonl(0xffffffff - next_nonce());
	    short fakeport = htons(getpid());
	    APPEND(info->data, j, fakeaddr);
	    APPEND(info->data, j, fakeport);
	}
	break;
        default:
            free(info->data);
            return 0;   /* do not know how to build this */
	}
	{
	    long now;
	    time(&now);
	    now = htonl(now);
	    APPEND(info->data, j, now);
	}
	assert(j <= 192 / 8);
	while (j < 192 / 8)
	    info->data[j++] = 0;
	info->datalen = j;
	Wrap (info->data, authptr->data + 8, info->data, info->datalen);
	return 1;
    }
#undef APPEND
#endif

    return 0;   /* Unknown authorization type */
}

int XCBGetAuthInfo(int fd, XCBAuthInfo *info)
{
    /* code adapted from Xlib/ConnDis.c, xtrans/Xtranssocket.c,
       xtrans/Xtransutils.c */
    char sockbuf[sizeof(struct sockaddr) + MAXPATHLEN];
    unsigned int socknamelen = sizeof(sockbuf);   /* need extra space */
    struct sockaddr *sockname = (struct sockaddr *) &sockbuf;
    Xauth *authptr = 0;
    int ret = 1;

    /* ensure info has reasonable contents */
    /* XXX This should be removed, but Jamey depends on it
       somehow but can't remember how.  Principle: don't touch
       someone else's data if you're borken. */
    info->namelen = info->datalen = 0;
    info->name = info->data = 0;

    if (getpeername(fd, sockname, &socknamelen) == -1)
        return 0;  /* can only authenticate sockets */

    authptr = get_authptr(sockname, socknamelen);
    if (authptr == 0)
        return 0;   /* cannot find good auth data */

    info->namelen = memdup(&info->name, authptr->name, authptr->name_length);
    if(info->namelen)
	ret = compute_auth(info, authptr, sockname);
    if(!ret)
    {
	free(info->name);
	info->name = 0;
	info->namelen = 0;
    }
    XauDisposeAuth(authptr);
    return ret;
}
