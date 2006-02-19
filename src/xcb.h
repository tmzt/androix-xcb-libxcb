/*
 * Copyright (C) 2001-2004 Bart Massey, Jamey Sharp, and Josh Triplett.
 * All Rights Reserved.
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

#ifndef __XCB_H
#define __XCB_H
#include <X11/Xmd.h>
#include <X11/X.h>
#include <sys/uio.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define deprecated __attribute__((__deprecated__))
#else
#define deprecated
#endif

/* Pre-defined constants */

/* current protocol version */
#define X_PROTOCOL 11

/* current minor version */
#define X_PROTOCOL_REVISION 0

/* X_TCP_PORT + display number = server port for TCP transport */
#define X_TCP_PORT 6000

#define XCB_TYPE_PAD(T,I) (-(I) & (sizeof(T) > 4 ? 3 : sizeof(T) - 1))


/* Opaque structures */

typedef struct XCBConnection XCBConnection;


/* Other types */

typedef struct {
    void *data;
    int rem;
    int index;
} XCBGenericIter;

typedef struct {
    BYTE response_type;
    CARD8 pad0;
    CARD16 sequence;
    CARD32 length;
} XCBGenericRep;

typedef struct {
    BYTE response_type;
    CARD8 pad0;
    CARD16 sequence;
} XCBGenericEvent;

typedef struct {
    BYTE response_type;
    BYTE error_code;
    CARD16 sequence;
} XCBGenericError;

typedef struct {
    unsigned int sequence;
} XCBVoidCookie;


/* Include the generated xproto and xcb_types headers. */
#include "xcb_types.h"
#include "xproto.h"


/* xcb_auth.c */

typedef struct XCBAuthInfo {
    int namelen;
    char *name;
    int datalen;
    char *data;
} XCBAuthInfo;

int XCBGetAuthInfo(int fd, XCBAuthInfo *info) deprecated;


/* xcb_out.c */

int XCBFlush(XCBConnection *c);
CARD32 XCBGetMaximumRequestLength(XCBConnection *c);


/* xcb_in.c */

XCBGenericEvent *XCBWaitEvent(XCBConnection *c) deprecated;
XCBGenericEvent *XCBWaitForEvent(XCBConnection *c);
XCBGenericEvent *XCBPollForEvent(XCBConnection *c, int *error);
unsigned int XCBGetRequestRead(XCBConnection *c);


/* xcb_ext.c */

typedef struct XCBExtension XCBExtension;

/* Do not free the returned XCBQueryExtensionRep - on return, it's aliased
 * from the cache. */
const XCBQueryExtensionRep *XCBGetExtensionData(XCBConnection *c, XCBExtension *ext);

void XCBPrefetchExtensionData(XCBConnection *c, XCBExtension *ext);


/* xcb_conn.c */

XCBConnSetupSuccessRep *XCBGetSetup(XCBConnection *c);
int XCBGetFileDescriptor(XCBConnection *c);

XCBConnection *XCBConnectToFD(int fd, XCBAuthInfo *auth_info);
void XCBDisconnect(XCBConnection *c);


/* xcb_util.c */

int XCBParseDisplay(const char *name, char **host, int *display, int *screen);
int XCBOpen(const char *host, int display) deprecated;
int XCBOpenTCP(const char *host, unsigned short port) deprecated;
int XCBOpenUnix(const char *file) deprecated;

XCBConnection *XCBConnectBasic(void) deprecated;
XCBConnection *XCBConnect(const char *displayname, int *screenp);
XCBConnection *XCBConnectToDisplayWithAuthInfo(const char *display, XCBAuthInfo *auth, int *screen);

int XCBSync(XCBConnection *c, XCBGenericError **e);


#ifdef __cplusplus
}
#endif

#endif
