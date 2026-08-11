#include "drakknet.h"

#include <stdlib.h>
#include <string.h>   /* memset */

#if defined(_WIN32)
    #include <windows.h>
    #include <winerror.h>
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>   /* close() */
    #include <errno.h>
#endif

// init flag
static int g_initialized = 0;

// wouldblock checker
static int is_wouldblock_error(void) {
#if defined(_WIN32)
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS; // errno == EINPROGRESS - connect()
#endif
}

drakknet_error_t drakknet_init(void) {
#if defined(_WIN32)
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        return DRAKKNET_ERR_PLATFORM_INIT;
    }
#endif
    g_initialized = 1;
    return DRAKKNET_OK;
}

void drakknet_cleanup(void) {
    if (!g_initialized) {
        return;
    }
#if defined(_WIN32)
    WSACleanup();
#endif
    g_initialized = 0;
}

// SOCKET LIFECYCLE
drakknet_error_t drakknet_socket(int domain, int type, int protocol, drakknet_socket_t* out_sock) {
    if (out_sock == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

#if defined(_WIN32)
    SOCKET s = socket(domain, type, protocol);
    if (s == INVALID_SOCKET) {
        return DRAKKNET_ERR_SOCKET_CREATE;
    }
#else
    int s = socket(domain, type, protocol);
    if (s == -1) {
        return DRAKKNET_ERR_SOCKET_CREATE;
    }
#endif

    out_sock->handle = s;
    return DRAKKNET_OK;
}

drakknet_error_t drakknet_close(drakknet_socket_t sock) {
#if defined(_WIN32)
    if (closesocket(sock.handle) != 0) {
        return DRAKKNET_ERR_UNKNOWN;
    }
#else
    if (close(sock.handle) != 0) {
        return DRAKKNET_ERR_UNKNOWN;
    }
#endif
    return DRAKKNET_OK;
}

drakknet_error_t drakknet_shutdown(drakknet_socket_t sock, int how) {
    if (shutdown(sock.handle, how) != 0) {
        return DRAKKNET_ERR_UNKNOWN;
    }
    return DRAKKNET_OK;
}

// ADDR BUILDER

drakknet_error_t drakknet_addr_from_string(const char* ip, unsigned short port, drakknet_addr_t* out_addr) {
    if (ip == NULL || out_addr == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

    memset(&out_addr->storage, 0, sizeof(out_addr->storage));

    struct sockaddr_in* addr4 = (struct sockaddr_in*)&out_addr->storage;
    if (inet_pton(AF_INET, ip, &addr4->sin_addr) == 1) {
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);
        out_addr->len = sizeof(struct sockaddr_in);
        return DRAKKNET_OK;
    }

    struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&out_addr->storage;
    if (inet_pton(AF_INET6, ip, &addr6->sin6_addr) == 1) {
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);
        out_addr->len = sizeof(struct sockaddr_in6);
        return DRAKKNET_OK;
    }

    return DRAKKNET_ERR_INVALID_ARG; 
}

drakknet_error_t drakknet_addr_from_combined_string(const char* combined, drakknet_addr_t* out_addr) {
    if (combined == NULL || out_addr == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

    const char* last_colon = strrchr(combined, ':');
    if (last_colon == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

    char* endptr;
    long port_val = strtol(last_colon + 1, &endptr, 10);
    if (*endptr != '\0' || port_val < 0 || port_val > 65535) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

    size_t ip_len = (size_t)(last_colon - combined);
    if (ip_len >= 64) { // IPV6 AND IPV4 SHORTER
        return DRAKKNET_ERR_INVALID_ARG;
    }
    char ip_buf[64];
    memcpy(ip_buf, combined, ip_len);
    ip_buf[ip_len] = '\0';

    return drakknet_addr_from_string(ip_buf, (unsigned short)port_val, out_addr);
}

// TCP
drakknet_error_t drakknet_bind(drakknet_socket_t sock, const drakknet_addr_t* addr) {
    if (addr == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }
    if (bind(sock.handle, (const struct sockaddr*)&addr->storage, addr->len) != 0) {
        return DRAKKNET_ERR_BIND;
    }
    return DRAKKNET_OK;
}

drakknet_error_t drakknet_listen(drakknet_socket_t sock, int backlog) {
    if (listen(sock.handle, backlog) != 0) {
        return DRAKKNET_ERR_LISTEN;
    }
    return DRAKKNET_OK;
}

drakknet_error_t drakknet_accept(drakknet_socket_t sock, drakknet_socket_t* out_client, drakknet_addr_t* out_addr) {
    if (out_client == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

    struct sockaddr_storage tmp_storage;
    socklen_t tmp_len = sizeof(tmp_storage);

#if defined(_WIN32)
    SOCKET client = accept(sock.handle, (struct sockaddr*)&tmp_storage, &tmp_len);
    if (client == INVALID_SOCKET) {
        if (is_wouldblock_error()) {
            return DRAKKNET_ERR_WOULDBLOCK;
        }
        return DRAKKNET_ERR_ACCEPT;
    }
#else
    int client = accept(sock.handle, (struct sockaddr*)&tmp_storage, &tmp_len);
    if (client == -1) {
        if (is_wouldblock_error()) {
            return DRAKKNET_ERR_WOULDBLOCK;
        }
        return DRAKKNET_ERR_ACCEPT;
    }
#endif

    out_client->handle = client;

    if (out_addr != NULL) {
        out_addr->storage = tmp_storage;
        out_addr->len = tmp_len;
    }

    return DRAKKNET_OK;
}

drakknet_error_t drakknet_connect(drakknet_socket_t sock, const drakknet_addr_t* addr) {
    if (addr == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }
    if (connect(sock.handle, (const struct sockaddr*)&addr->storage, addr->len) != 0) {
        if (is_wouldblock_error()) {
            return DRAKKNET_ERR_WOULDBLOCK;
        }
        return DRAKKNET_ERR_CONNECT;
    }
    return DRAKKNET_OK;
}

drakknet_error_t drakknet_send(drakknet_socket_t sock, const void* buf, size_t len, int flags, size_t* out_sent) {
    if (buf == NULL || out_sent == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

#if defined(_WIN32)
    int result = send(sock.handle, (const char*)buf, (int)len, flags);
    if (result == SOCKET_ERROR) {
        if (is_wouldblock_error()) {
            return DRAKKNET_ERR_WOULDBLOCK;
        }
        return DRAKKNET_ERR_SEND;
    }
#else
    ssize_t result = send(sock.handle, buf, len, flags);
    if (result == -1) {
        if (is_wouldblock_error()) {
            return DRAKKNET_ERR_WOULDBLOCK;
        }
        return DRAKKNET_ERR_SEND;
    }
#endif

    *out_sent = (size_t)result;
    return DRAKKNET_OK;
}

drakknet_error_t drakknet_recv(drakknet_socket_t sock, void* buf, size_t len, int flags, size_t* out_received) {
    if (buf == NULL || out_received == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

#if defined(_WIN32)
    int result = recv(sock.handle, (char*)buf, (int)len, flags);
    if (result == SOCKET_ERROR) {
        if (is_wouldblock_error()) {
            return DRAKKNET_ERR_WOULDBLOCK;
        }
        return DRAKKNET_ERR_RECV;
    }
#else
    ssize_t result = recv(sock.handle, buf, len, flags);
    if (result == -1) {
        if (is_wouldblock_error()) {
            return DRAKKNET_ERR_WOULDBLOCK;
        }
        return DRAKKNET_ERR_RECV;
    }
#endif

    *out_received = (size_t)result;
    return DRAKKNET_OK;
}

// UDP
drakknet_error_t drakknet_sendto(drakknet_socket_t sock, const void* buf, size_t len, int flags,
                                  const drakknet_addr_t* dest, size_t* out_sent) {
    if (buf == NULL || dest == NULL || out_sent == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

#if defined(_WIN32)
    int result = sendto(sock.handle, (const char*)buf, (int)len, flags,
                         (const struct sockaddr*)&dest->storage, dest->len);
    if (result == SOCKET_ERROR) {
        return DRAKKNET_ERR_SEND;
    }
#else
    ssize_t result = sendto(sock.handle, buf, len, flags,
                             (const struct sockaddr*)&dest->storage, dest->len);
    if (result == -1) {
        return DRAKKNET_ERR_SEND;
    }
#endif

    *out_sent = (size_t)result;
    return DRAKKNET_OK;
}

drakknet_error_t drakknet_recvfrom(drakknet_socket_t sock, void* buf, size_t len, int flags,
                                    drakknet_addr_t* out_src, size_t* out_received) {
    if (buf == NULL || out_received == NULL) {
        return DRAKKNET_ERR_INVALID_ARG;
    }

    struct sockaddr_storage tmp_storage;
    socklen_t tmp_len = sizeof(tmp_storage);

#if defined(_WIN32)
    int result = recvfrom(sock.handle, (char*)buf, (int)len, flags,
                           (struct sockaddr*)&tmp_storage, &tmp_len);
    if (result == SOCKET_ERROR) {
        if (is_wouldblock_error()) {
            return DRAKKNET_ERR_WOULDBLOCK;
        }
        return DRAKKNET_ERR_RECV;
    }
#else
    ssize_t result = recvfrom(sock.handle, buf, len, flags,
                               (struct sockaddr*)&tmp_storage, &tmp_len);
    if (result == -1) {
        if (is_wouldblock_error()) {
            return DRAKKNET_ERR_WOULDBLOCK;
        }
        return DRAKKNET_ERR_RECV;
    }
#endif

    if (out_src != NULL) {
        out_src->storage = tmp_storage;
        out_src->len = tmp_len;
    }

    *out_received = (size_t)result;
    return DRAKKNET_OK;
}

// SOCKOPT
drakknet_error_t drakknet_setsockopt(drakknet_socket_t sock, int level, int optname,
                                      const void* optval, size_t optlen) {
#if defined(_WIN32)
    if (setsockopt(sock.handle, level, optname, (const char*)optval, (int)optlen) != 0) {
        return DRAKKNET_ERR_UNKNOWN;
    }
#else
    if (setsockopt(sock.handle, level, optname, optval, (socklen_t)optlen) != 0) {
        return DRAKKNET_ERR_UNKNOWN;
    }
#endif
    return DRAKKNET_OK;
}

drakknet_error_t drakknet_set_blocking_mode(drakknet_socket_t sock, drakknet_blocking_mode_t mode) {
    #if defined(_WIN32)
        u_long mode_long = 0;
        switch (mode) {
        case DRAKKNET_BLOCKING:
            if (ioctlsocket(sock.handle, FIONBIO, &mode_long) == SOCKET_ERROR) {
                return DRAKKNET_ERR_SET_BLOCKING_MODE;
            }
            break;
        case DRAKKNET_UNBLOCKING:
            mode_long = 1;
            if (ioctlsocket(sock.handle, FIONBIO, &mode_long) == SOCKET_ERROR) {
                return DRAKKNET_ERR_SET_BLOCKING_MODE;
            }
            break;
        } 

    #else
        int flags = fcntl(sock.handle, F_GETFL, 0);
        if (flags == -1) {
            return DRAKKNET_ERR_SET_BLOCKING_MODE;
        }

        switch (mode) {
        case DRAKKNET_BLOCKING:
            if (fcntl(sock.handle, F_SETFL, flags & ~O_NONBLOCK) == -1) {
                return DRAKKNET_ERR_SET_BLOCKING_MODE;
            }
            break;
        case DRAKKNET_UNBLOCKING:
            if (fcntl(sock.handle, F_SETFL, flags | O_NONBLOCK) == - 1) {
                return DRAKKNET_ERR_SET_BLOCKING_MODE;
            }
            break;
        }
    #endif

    return DRAKKNET_OK;
}

