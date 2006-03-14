/*
 * Copyright (C) 2001-2006 Bart Massey, Jamey Sharp, and Josh Triplett.
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
#include <sys/types.h>

/* TODO: check for stdint in config? (HAVE_STDINT) fallback? */
#include <stdint.h>

typedef uint8_t  BYTE;
typedef uint8_t  BOOL;
typedef uint8_t  CARD8;
typedef uint16_t CARD16;
typedef uint32_t CARD32;
typedef int8_t   INT8;
typedef int16_t  INT16;
typedef int32_t  INT32;

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


/**
 * @file xcb.h
 */

/**
 * @defgroup XCB_Core_Api XCB Core API
 * @brief Core API of the XCB library.
 *
 * @{
 */

/* Pre-defined constants */

/** Current protocol version */
#define X_PROTOCOL 11

/** Current minor version */
#define X_PROTOCOL_REVISION 0

/** X_TCP_PORT + display number = server port for TCP transport */
#define X_TCP_PORT 6000

#define XCB_TYPE_PAD(T,I) (-(I) & (sizeof(T) > 4 ? 3 : sizeof(T) - 1))


/* Opaque structures */

/**
 * @brief XCB Connection structure.
 *
 * A structure that contain all data that  XCB needs to communicate with an X server.
 */
typedef struct XCBConnection XCBConnection;  /**< Opaque structure containing all data that  XCB needs to communicate with an X server. */


/* Other types */

/**
 * @brief Generic iterator.
 *
 * A generic iterator structure.
 */
typedef struct {
    void *data;   /**< Data of the current iterator */
    int rem;    /**< remaining elements */
    int index;  /**< index of the current iterator */
} XCBGenericIter;

/**
 * @brief Generic reply.
 *
 * A generic reply structure.
 */
typedef struct {
    BYTE   response_type;  /**< Type of the response */
    CARD8  pad0;           /**< Padding */
    CARD16 sequence;       /**< Sequence number */
    CARD32 length;         /**< Length of the response */
} XCBGenericRep;

/**
 * @brief Generic event.
 *
 * A generic event structure.
 */
typedef struct {
    BYTE   response_type;  /**< Type of the response */
    CARD8  pad0;           /**< Padding */
    CARD16 sequence;       /**< Sequence number */
    CARD32 pad[7];         /**< Padding */
    CARD32 full_sequence;
} XCBGenericEvent;

/**
 * @brief Generic error.
 *
 * A generic error structure.
 */
typedef struct {
    BYTE   response_type;  /**< Type of the response */
    BYTE   error_code;     /**< Error code */
    CARD16 sequence;       /**< Sequence number */
    CARD32 pad[7];         /**< Padding */
    CARD32 full_sequence;
} XCBGenericError;

/**
 * @brief Generic cookie.
 *
 * A generic cookie structure.
 */
typedef struct {
    unsigned int sequence;  /**< Sequence number */
} XCBVoidCookie;


/* Include the generated xproto and xcb_types headers. */
#include "xcb_types.h"
#include "xproto.h"


/* xcb_auth.c */

/**
 * @brief Container for authorization information.
 *
 * A container for authorization information to be sent to the X server.
 */
typedef struct XCBAuthInfo {
    int   namelen;  /**< Length of the string name (as returned by strlen). */
    char *name;     /**< String containing the authentication protocol name, such as "MIT-MAGIC-COOKIE-1" or "XDM-AUTHORIZATION-1". */
    int   datalen;  /**< Length of the data member. */
    char *data;   /**< Data interpreted in a protocol-specific manner. */
} XCBAuthInfo;

/**
 * @brief Gets authorization information.
 * @param fd: The file descriptor.
 * @param info: The authorisation info to set.
 * @return @c 0 on failure, 1 otherwise.
 *
 * @deprecated This function is deprecated. It must not be used in
 * newly written code.
 */
int XCBGetAuthInfo(int fd, XCBAuthInfo *info) deprecated;


/* xcb_out.c */

/**
 * @brief Forces any buffered output to be written to the server.
 * @param c: The connection to the X server.
 * @return > @c 0 on success, <= @c 0 otherwise.
 *
 * Forces any buffered output to be written to the server. Blocks
 * until the write is complete.
 */
int XCBFlush(XCBConnection *c);

/**
 * @brief Returns the maximum request length field from the connection
 * setup data.
 * @param c: The connection to the X server.
 * @return The maximum request length field.
 *
 * In the absence of the BIG-REQUESTS extension, returns the
 * maximum request length field from the connection setup data, which
 * may be as much as 65535. If the server supports BIG-REQUESTS, then
 * the maximum request length field from the reply to the
 * BigRequestsEnable request will be returned instead.
 *
 * Note that this length is measured in four-byte units, making the
 * theoretical maximum lengths roughly 256kB without BIG-REQUESTS and
 * 16GB with.
 */
CARD32 XCBGetMaximumRequestLength(XCBConnection *c);


/* xcb_in.c */

/**
 * @brief Returns the next event or error from the server.
 * @param c: The connection to the X server.
 * @return The next event from the server.
 *
 * @deprecated This function is deprecated. It must not be used in
 * newly written code.
 */
XCBGenericEvent *XCBWaitEvent(XCBConnection *c) deprecated;

/**
 * @brief Returns the next event or error from the server.
 * @param c: The connection to the X server.
 * @return The next event from the server.
 *
 * Returns the next event or error from the server, or returns null in
 * the event of an I/O error. Blocks until either an event or error
 * arrive, or an I/O error occurs.
 */
XCBGenericEvent *XCBWaitForEvent(XCBConnection *c);

/**
 * @brief Returns the next event or error from the server.
 * @param c: The connection to the X server.
 * @param error: A pointer to an int to be filled in with the I/O
 * error status of the operation.
 * @return The next event from the server.
 *
 * Returns the next event or error from the server, if one is
 * available, or returns @c NULL otherwise. If no event is available, that
 * might be because an I/O error like connection close occurred while
 * attempting to read the next event. The @p error parameter is a
 * pointer to an int to be filled in with the I/O error status of the
 * operation. If @p error is @c NULL, terminates the application when an
 * I/O error occurs.
 */
XCBGenericEvent *XCBPollForEvent(XCBConnection *c, int *error);

/**
 * @brief Returns the last sequence number that the server is known to
 * have processed.
 * @param c: The connection to the X server.
 * @return The last sequence number.
 *
 * Returns the last sequence number that the server is known to have
 * processed. This function enables applications to determine whether
 * forcing a cookie is going to block.
 *
 * @todo review that function.
 */
unsigned int XCBGetRequestRead(XCBConnection *c);


/* xcb_ext.c */

/**
 * @typedef typedef struct XCBExtension XCBExtension
 */
typedef struct XCBExtension XCBExtension;  /**< Opaque structure used as key for XCBGetExtensionData. */

/**
 * @brief Caches reply information from QueryExtension requests.
 * @param c: The connection.
 * @param ext: The extension data.
 * @return A pointer to a XCBConnSetupSuccessRep pointer.
 *
 * This function is the primary interface to the "extension cache",
 * which caches reply information from QueryExtension
 * requests. Invoking this function may cause a call to
 * XCBQueryExtension to retrieve extension information from the
 * server, and may block until extension data is received from the
 * server.
 *
 * The result must not be freed. This storage is managed by the cache
 * itself.
 */
const XCBQueryExtensionRep *XCBGetExtensionData(XCBConnection *c, XCBExtension *ext);

/**
 * @brief Prefetch of extension data into the extension cache
 * @param c: The connection.
 * @param ext: The extension data.
 *
 * This function allows a "prefetch" of extension data into the
 * extension cache. Invoking the function may cause a call to
 * XCBQueryExtension, but will not block waiting for the
 * reply. XCBGetExtensionData will return the prefetched data after
 * possibly blocking while it is retrieved.
 */
void XCBPrefetchExtensionData(XCBConnection *c, XCBExtension *ext);


/* xcb_conn.c */

/**
 * @brief Access the data returned by the server.
 * @param c: The connection.
 * @return A pointer to a XCBConnSetupSuccessRep pointer.
 *
 * Accessor for the data returned by the server when the XCBConnection
 * was initialized. This data includes
 * - the server's required format for images,
 * - a list of available visuals,
 * - a list of available screens,
 * - the server's maximum request length (in the absence of the
 * BIG-REQUESTS extension),
 * - and other assorted information.
 *
 * See the X protocol specification for more details.
 *
 * The result must not be freed.
 */
const XCBConnSetupSuccessRep *XCBGetSetup(XCBConnection *c);

/**
 * @brief Access the file descriptor of the connection.
 * @param c: The connection.
 * @return The file descriptor.
 *
 * Accessor for the file descriptor that was passed to the
 * XCBConnectToFD call that returned @p c.
 */
int XCBGetFileDescriptor(XCBConnection *c);

/**
 * @brief Connects to the X server.
 * @param fd: The file descriptor.
 * @param auth_info: Authentication data.
 * @return A newly allocated XCBConnection structure.
 *
 * Connects to an X server, given the open socket @p fd and the
 * XCBAuthInfo @p auth_info. The file descriptor @p fd is
 * bidirectionally connected to an X server. XCBGetTCPFD and
 * XCBGetUnixFD return appropriate file descriptors. If the connection
 * should be unauthenticated, @p auth_info must be @c
 * NULL. XCBGetAuthInfo returns appropriate authentication data.
 */
XCBConnection *XCBConnectToFD(int fd, XCBAuthInfo *auth_info);

/**
 * @brief Closes the connection.
 * @param c: The connection.
 *
 * Closes the file descriptor and frees all memory associated with the
 * connection @c c.
 */
void XCBDisconnect(XCBConnection *c);


/* xcb_util.c */

/**
 * @brief Parses a display string name in the form documented by X(7x).
 * @param displayname: The name of the display.
 * @param hostp: A pointer to a malloc'd copy of the hostname.
 * @param displayp: A pointer to the display number.
 * @param screenp: A pointer to the screen number.
 * @return 0 on failure, non 0 otherwise.
 *
 * Parses the display string name @p display_name in the form
 * documented by X(7x). Has no side effects on failure. If
 * @p displayname is @c NULL or empty, it uses the environment
 * variable DISPLAY. @p hostp is a pointer to a newly allocated string
 * that contain the host name. @p displayp is set to the display
 * number and @p screenp to the preferred screen number. @p screenp
 * can be @c NULL. If @p displayname does not contain a screen number,
 * it is set to @c 0.
 */
int XCBParseDisplay(const char *name, char **host, int *display, int *screen);

/**
 * @brief Open a connection to the X server.
 * @param host: The host name.
 * @param display: The display number.
 * @return The file descriptor.
 *
 * @deprecated This function is deprecated. It must not be used in
 * newly written code.
 */
int XCBOpen(const char *host, int display) deprecated;

/**
 * @brief Open a connection to the X server.
 * @param host: The host name.
 * @param port: The TCP port.
 * @return The file descriptor.
 *
 * @deprecated This function is deprecated. It must not be used in
 * newly written code.
 */
int XCBOpenTCP(const char *host, unsigned short port) deprecated;

/**
 * @brief Connects to the X server.
 * @param file: The file name.
 * @return The file descriptor.
 *
 * @deprecated This function is deprecated. It must not be used in
 * newly written code.
 */
int XCBOpenUnix(const char *file) deprecated;

/**
 * @brief Connects to the X server.
 * @return A newly allocated XCBConnection structure.
 *
 * @deprecated This function is deprecated. It must not be used in
 * newly written code.
 */
XCBConnection *XCBConnectBasic(void) deprecated;

/**
 * @brief Connects to the X server.
 * @param displayname: The name of the display.
 * @param screenp: A pointer to a preferred screen number.
 * @return A newly allocated XCBConnection structure.
 *
 * Connects to the X server specified by @p displayname. If @p
 * displayname is @c NULL, uses the value of the DISPLAY environment
 * variable. If a particular screen on that server is preferred, the
 * int pointed to by @p screenp (if not @c NULL) will be set to that
 * screen; otherwise the screen will be set to 0.
 */
XCBConnection *XCBConnect(const char *displayname, int *screenp);

/**
 * @brief Connects to the X server, using an authorization information.
 * @param displayname: The name of the display.
 * @param auth: The authorization information.
 * @param screenp: A pointer to a preferred screen number.
 * @return A newly allocated XCBConnection structure.
 *
 * Connects to the X server specified by @p displayname, using the
 * authorization @p auth. If a particular screen on that server is
 * preferred, the int pointed to by @p screenp (if not @c NULL) will
 * be set to that screen; otherwise @p screenp will be set to 0.
 */
XCBConnection *XCBConnectToDisplayWithAuthInfo(const char *display, XCBAuthInfo *auth, int *screen);

/**
 * @brief Ensures that all events and errors are avalaible in XCB.
 * @param c: The connection to the X server.
 * @param e: A pointer to an error.
 * @return @c 1 on success, @c 0 otherwise.
 *
 * Blocks the calling thread for the duration of one round trip to the
 * server, ensuring that all events and errors caused by previous
 * requests are available to XCB.
 */
int XCBSync(XCBConnection *c, XCBGenericError **e);


/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif
