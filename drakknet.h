#ifndef DRAKKNET_NET_H
#define DRAKKNET_NET_H

#include <stddef.h> // size_t

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// SOCKET
typedef struct {
#if defined(_WIN32)
    SOCKET handle;
#else
    int handle;
#endif
} drakknet_socket_t;

// SOCKADDRIN(IPV4) AND SOCKADDRIN6(IPV6)
typedef struct {
    struct sockaddr_storage storage;
    socklen_t len;
} drakknet_addr_t;

// ERRORS
typedef enum {
    DRAKKNET_OK                    = 0,
    DRAKKNET_ERR_SOCKET_CREATE     = 1,
    DRAKKNET_ERR_BIND              = 2,
    DRAKKNET_ERR_LISTEN            = 3,
    DRAKKNET_ERR_ACCEPT            = 4,
    DRAKKNET_ERR_CONNECT           = 5,
    DRAKKNET_ERR_SEND              = 6,
    DRAKKNET_ERR_RECV              = 7,
    DRAKKNET_ERR_INVALID_ARG       = 8,
    DRAKKNET_ERR_PLATFORM_INIT     = 9,
    DRAKKNET_ERR_WOULDBLOCK        = 10,
    DRAKKNET_ERR_SET_BLOCKING_MODE = 11,
    DRAKKNET_ERR_UNKNOWN           = 12,
    DRAKKNET_ERR_TUNG_TUNG_TUNG_TUNG_SAHUR = 13
} drakknet_error_t;

// BLOCKING
typedef enum {
    DRAKKNET_BLOCKING   = 0,
    DRAKKNET_UNBLOCKING = 1
} drakknet_blocking_mode_t;

/* Initializes the drakknet library. On Windows this calls WSAStartup
 * internally; on Linux/POSIX it is a no-op besides flipping the internal
 * initialized flag.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_PLATFORM_INIT if WSAStartup failed (Windows only). */
drakknet_error_t drakknet_init(void);

/* Releases any platform resources acquired by drakknet_init (WSACleanup on
 * Windows). Safe to call even if drakknet_init was never called or failed —
 * in that case it is a no-op.
 * Returns: nothing (void). */
void             drakknet_cleanup(void);

/* Creates a new socket, mirroring the standard socket() call.
 * domain:   address family, e.g. AF_INET or AF_INET6.
 * type:     socket type, e.g. SOCK_STREAM (TCP) or SOCK_DGRAM (UDP).
 * protocol: protocol, e.g. IPPROTO_TCP or IPPROTO_UDP (0 lets the OS pick).
 * out_sock: receives the created socket handle on success.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if out_sock is NULL.
 *          DRAKKNET_ERR_SOCKET_CREATE if the underlying socket() call failed. */
drakknet_error_t drakknet_socket(int domain, int type, int protocol, drakknet_socket_t* out_sock);

/* Closes a socket and releases its OS resources. The handle must not be
 * used again after this call.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_UNKNOWN if the underlying close/closesocket call failed. */
drakknet_error_t drakknet_close(drakknet_socket_t sock);

/* Shuts down part or all of a full-duplex connection without releasing the
 * socket handle itself (the socket remains valid and must still be closed
 * separately via drakknet_close).
 * how: SHUT_RD/SHUT_WR/SHUT_RDWR (POSIX) or SD_RECEIVE/SD_SEND/SD_BOTH (Windows).
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_UNKNOWN if the underlying shutdown() call failed. */
drakknet_error_t drakknet_shutdown(drakknet_socket_t sock, int how);

/* Builds a drakknet_addr_t from a separate IP string and port number.
 * The IP version (IPv4 vs IPv6) is detected automatically: IPv4 parsing is
 * tried first, then IPv6.
 * ip:       null-terminated IP string, e.g. "127.0.0.1" or "::1".
 * port:     port number in host byte order (converted internally).
 * out_addr: receives the populated address on success.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if ip/out_addr is NULL, or the string is
 *          neither a valid IPv4 nor a valid IPv6 address. */
drakknet_error_t drakknet_addr_from_string(const char* ip, unsigned short port, drakknet_addr_t* out_addr);

/* Builds a drakknet_addr_t from a single combined "ip:port" string, so the
 * caller does not need to split it manually. The port is taken from after
 * the last ':' in the string; everything before it is treated as the IP.
 * combined: null-terminated string, e.g. "127.0.0.1:8080" or "[::1]:8080".
 * out_addr: receives the populated address on success.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if combined/out_addr is NULL, no ':' is
 *          found, the port part is not a valid number in [0, 65535], or the
 *          IP part fails to parse as IPv4/IPv6. */
drakknet_error_t drakknet_addr_from_combined_string(const char* combined, drakknet_addr_t* out_addr);

/* Binds a socket to a local address, mirroring bind().
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if addr is NULL.
 *          DRAKKNET_ERR_BIND if the underlying bind() call failed. */
drakknet_error_t drakknet_bind(drakknet_socket_t sock, const drakknet_addr_t* addr);

/* Marks a bound socket as passive/listening, mirroring listen().
 * backlog: maximum length of the pending connections queue.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_LISTEN if the underlying listen() call failed. */
drakknet_error_t drakknet_listen(drakknet_socket_t sock, int backlog);

/* Accepts one pending incoming connection, mirroring accept(). Blocks until
 * a connection arrives.
 * out_client: receives the new connected socket on success.
 * out_addr:   optional; if non-NULL, receives the remote peer's address.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if out_client is NULL.
 *          DRAKKNET_ERR_ACCEPT if the underlying accept() call failed. */
drakknet_error_t drakknet_accept(drakknet_socket_t sock, drakknet_socket_t* out_client, drakknet_addr_t* out_addr);

/* Connects a socket to a remote address, mirroring connect(). Blocks until
 * the connection is established or fails.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if addr is NULL.
 *          DRAKKNET_ERR_CONNECT if the underlying connect() call failed. */
drakknet_error_t drakknet_connect(drakknet_socket_t sock, const drakknet_addr_t* addr);

/* Sends data on a connected socket, mirroring send().
 * buf:      data to send.
 * len:      number of bytes in buf.
 * flags:    passed through to the underlying send() call (0 for default behavior).
 * out_sent: receives the number of bytes actually sent (may be less than len).
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if buf/out_sent is NULL.
 *          DRAKKNET_ERR_SEND if the underlying send() call failed. */
drakknet_error_t drakknet_send(drakknet_socket_t sock, const void* buf, size_t len, int flags, size_t* out_sent);

/* Receives data on a connected socket, mirroring recv().
 * buf:          buffer to receive into.
 * len:          size of buf in bytes.
 * flags:        passed through to the underlying recv() call (0 for default behavior).
 * out_received: receives the number of bytes actually read; 0 means the
 *               peer has closed its side of the connection (not an error).
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if buf/out_received is NULL.
 *          DRAKKNET_ERR_RECV if the underlying recv() call failed. */
drakknet_error_t drakknet_recv(drakknet_socket_t sock, void* buf, size_t len, int flags, size_t* out_received);

/* Sends a datagram to a specific destination, mirroring sendto().
 * dest:     destination address for this datagram.
 * out_sent: receives the number of bytes actually sent.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if buf/dest/out_sent is NULL.
 *          DRAKKNET_ERR_SEND if the underlying sendto() call failed. */
drakknet_error_t drakknet_sendto(drakknet_socket_t sock, const void* buf, size_t len, int flags,
                                  const drakknet_addr_t* dest, size_t* out_sent);

/* Receives a datagram and the address of its sender, mirroring recvfrom().
 * out_src:      optional; if non-NULL, receives the sender's address.
 * out_received: receives the number of bytes actually read.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_INVALID_ARG if buf/out_received is NULL.
 *          DRAKKNET_ERR_RECV if the underlying recvfrom() call failed. */
drakknet_error_t drakknet_recvfrom(drakknet_socket_t sock, void* buf, size_t len, int flags,
                                    drakknet_addr_t* out_src, size_t* out_received);

/* Sets a socket option, mirroring setsockopt() (e.g. SO_REUSEADDR, TCP_NODELAY).
 * level:   option level, e.g. SOL_SOCKET or IPPROTO_TCP.
 * optname: option name, e.g. SO_REUSEADDR.
 * optval:  pointer to the option value.
 * optlen:  size of the value pointed to by optval.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_UNKNOWN if the underlying setsockopt() call failed. */
drakknet_error_t drakknet_setsockopt(drakknet_socket_t sock, int level, int optname,
                                      const void* optval, size_t optlen);

/* Switches a socket between blocking and non-blocking mode, mirroring
 * ioctlsocket(FIONBIO) on Windows and fcntl(F_GETFL/F_SETFL, O_NONBLOCK)
 * on POSIX.
 * mode: DRAKKNET_BLOCKING to restore blocking (default) behavior, where
 *       accept/connect/recv/recvfrom wait until an event occurs.
 *       DRAKKNET_UNBLOCKING to make accept/connect/recv/recvfrom return
 *       immediately with DRAKKNET_ERR_WOULDBLOCK instead of waiting when
 *       there is nothing to do yet.
 * Returns: DRAKKNET_OK on success.
 *          DRAKKNET_ERR_SET_BLOCKING_MODE if the underlying ioctlsocket/fcntl
 *          call failed. */
drakknet_error_t drakknet_set_blocking_mode(drakknet_socket_t sock, drakknet_blocking_mode_t mode);
#ifdef __cplusplus
}
#endif

#endif /* DRAKKNET_NET_H */
