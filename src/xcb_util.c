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

/* Utility functions implementable using only public APIs. */

#include <assert.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "xcb.h"
#include "xcbext.h"

int XCBPopcount(CARD32 mask)
{
    unsigned long y;
    y = (mask >> 1) & 033333333333;
    y = mask - y - ((y >> 1) & 033333333333);
    return ((y + (y >> 3)) & 030707070707) % 077;
}

int XCBParseDisplay(const char *name, char **host, int *displayp, int *screenp)
{
    int len, display, screen;
    char *colon, *dot, *end;
    if(!name || !*name)
        name = getenv("DISPLAY");
    if(!name)
        return 0;
    colon = strrchr(name, ':');
    if(!colon)
        return 0;
    len = colon - name;
    ++colon;
    display = strtoul(colon, &dot, 10);
    if(dot == colon)
        return 0;
    if(*dot == '\0')
        screen = 0;
    else
    {
        if(*dot != '.')
            return 0;
        ++dot;
        screen = strtoul(dot, &end, 10);
        if(end == dot || *end != '\0')
            return 0;
    }
    /* At this point, the display string is fully parsed and valid, but
     * the caller's memory is untouched. */

    *host = malloc(len + 1);
    if(!*host)
        return 0;
    memcpy(*host, name, len);
    (*host)[len] = '\0';
    *displayp = display;
    if(screenp)
        *screenp = screen;
    return 1;
}

int XCBOpen(const char *host, const int display)
{
    int fd;

    if(*host)
    {
        /* display specifies TCP */
        unsigned short port = X_TCP_PORT + display;
        fd = XCBOpenTCP(host, port);
    }
    else
    {
        /* display specifies Unix socket */
        static const char base[] = "/tmp/.X11-unix/X";
        char file[sizeof(base) + 20];
        snprintf(file, sizeof(file), "%s%d", base, display);
        fd = XCBOpenUnix(file);
    }

    return fd;
}

int XCBOpenTCP(const char *host, const unsigned short port)
{
    int fd;
    struct sockaddr_in addr;
    struct hostent *hostaddr = gethostbyname(host);
    if(!hostaddr)
        return -1;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, hostaddr->h_addr_list[0], sizeof(addr.sin_addr));

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd == -1)
        return -1;
    if(connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        return -1;
    return fd;
}

int XCBOpenUnix(const char *file)
{
    int fd;
    struct sockaddr_un addr = { AF_UNIX };
    strcpy(addr.sun_path, file);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd == -1)
        return -1;
    if(connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        return -1;
    return fd;
}

XCBConnection *XCBConnect(const char *displayname, int *screenp)
{
    int fd, display = 0;
    char *host;
    XCBConnection *c;
    XCBAuthInfo auth;

    if(!XCBParseDisplay(displayname, &host, &display, screenp))
        return 0;
    fd = XCBOpen(host, display);
    free(host);
    if(fd == -1)
        return 0;

    XCBGetAuthInfo(fd, &auth);
    c = XCBConnectToFD(fd, &auth);
    free(auth.name);
    free(auth.data);
    return c;
}

XCBConnection *XCBConnectToDisplayWithAuthInfo(const char *displayname, XCBAuthInfo *auth, int *screenp)
{
    int fd, display = 0;
    char *host;

    if(!XCBParseDisplay(displayname, &host, &display, screenp))
        return 0;
    fd = XCBOpen(host, display);
    free(host);
    if(fd == -1)
        return 0;

    return XCBConnectToFD(fd, auth);
}

/* backwards compatible interface: remove before 1.0 release */
XCBConnection *XCBConnectBasic()
{
    XCBConnection *c = XCBConnect(0, 0);
    if(c)
        return c;
    perror("XCBConnect");
    abort();
}

int XCBSync(XCBConnection *c, XCBGenericError **e)
{
    XCBGetInputFocusRep *reply = XCBGetInputFocusReply(c, XCBGetInputFocus(c), e);
    free(reply);
    return reply != 0;
}

/* The functions beyond this point still use only public interfaces,
 * but are not themselves part of the public interface. So their
 * prototypes are in xcbint.h. */

#include "xcbint.h"

int _xcb_set_fd_flags(const int fd)
{
    long flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1)
        return 0;
    flags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flags) == -1)
        return 0;
    if(fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
        return 0;
    return 1;
}

int _xcb_readn(const int fd, void *buf, const int buflen, int *count)
{
    int n = read(fd, ((char *) buf) + *count, buflen - *count);
    if(n > 0)
        *count += n;
    return n;
}

int _xcb_read_block(const int fd, void *buf, const size_t len)
{
    int done = 0;
    while(done < len)
    {
        int ret = _xcb_readn(fd, buf, len, &done);
        if(ret < 0 && errno == EAGAIN)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            ret = select(fd + 1, &fds, 0, 0, 0);
        }
        if(ret <= 0)
            return ret;
    }
    return len;
}

int _xcb_write(const int fd, char (*buf)[], int *count)
{
    int n = write(fd, *buf, *count);
    if(n > 0)
    {
        *count -= n;
        if(*count)
            memmove(*buf, *buf + n, *count);
    }
    return n;
}

int _xcb_writev(const int fd, struct iovec *vec, int count)
{
    int n = writev(fd, vec, count);
    if(n > 0)
    {
        int rem = n;
        for(; count; --count, ++vec)
        {
            int cur = vec->iov_len;
            if(cur > rem)
                cur = rem;
            vec->iov_len -= cur;
            vec->iov_base = (char *) vec->iov_base + cur;
            rem -= cur;
            if(vec->iov_len)
                break;
        }
        assert(rem == 0);
    }
    return n;
}
