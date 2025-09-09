/*
 * Zephyr HAL for socket/handleset API using BSD sockets
 */

#include "hal_socket.h"
#include "lib_memory.h"
#include <zephyr/net/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/net/net_ip.h>

struct sSocket {
    int fd;
    uint32_t connectTimeout;
};

struct sServerSocket {
    int fd;
    int backLog;
};

struct sUdpSocket {
    int fd;
};

struct sHandleSet {
    struct pollfd* pfds;
    int count;
    int cap;
};

PAL_API HandleSet
Handleset_new(void)
{
    struct sHandleSet* hs = (struct sHandleSet*)GLOBAL_CALLOC(1, sizeof(struct sHandleSet));
    if (hs) {
        hs->cap = 8;
        hs->pfds = (struct pollfd*)GLOBAL_CALLOC(hs->cap, sizeof(struct pollfd));
        if (!hs->pfds) { GLOBAL_FREEMEM(hs); return NULL; }
    }
    return hs;
}

PAL_API void
Handleset_reset(HandleSet self)
{
    if (!self) return;
    self->count = 0;
}

PAL_API void
Handleset_addSocket(HandleSet self, const Socket sock)
{
    if (!self || !sock) return;
    if (self->count >= self->cap) {
        int ncap = (self->cap == 0) ? 4 : (self->cap * 2);
        struct pollfd* np = (struct pollfd*)Memory_realloc(self->pfds, ncap * sizeof(struct pollfd));
        if (!np) return;
        self->pfds = np;
        self->cap = ncap;
    }
    self->pfds[self->count].fd = sock->fd;
    self->pfds[self->count].events = POLLIN;
    self->pfds[self->count].revents = 0;
    self->count++;
}

void
Handleset_removeSocket(HandleSet self, const Socket sock)
{
    if (!self || !sock) return;
    for (int i = 0; i < self->count; ++i) {
        if (self->pfds[i].fd == sock->fd) {
            if (i + 1 < self->count)
                memmove(&self->pfds[i], &self->pfds[i+1], (self->count - i - 1) * sizeof(struct pollfd));
            self->count--;
            break;
        }
    }
}

PAL_API int
Handleset_waitReady(HandleSet self, unsigned int timeoutMs)
{
    if (!self) return -1;
    return poll(self->pfds, (unsigned int)self->count, (int)timeoutMs);
}

PAL_API void
Handleset_destroy(HandleSet self)
{
    if (!self) return;
    if (self->pfds) Memory_free(self->pfds);
    GLOBAL_FREEMEM(self);
}

PAL_API ServerSocket
TcpServerSocket_create(const char* address, int port)
{
    int fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) return NULL;
    int one = 1;
    (void)setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &one, sizeof(one));
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    char portstr[16]; snprintf(portstr, sizeof(portstr), "%d", port);
    int rc = getaddrinfo(address, portstr, &hints, &res);
    if (rc != 0 || !res) {
        struct sockaddr_in6 a6 = {0};
        a6.sin6_family = AF_INET6; a6.sin6_port = htons((uint16_t)port); a6.sin6_addr = in6addr_any;
        if (bind(fd, (struct sockaddr*)&a6, sizeof(a6)) < 0) { close(fd); return NULL; }
    } else {
        if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) { freeaddrinfo(res); close(fd); return NULL; }
        freeaddrinfo(res);
    }
    int fl = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    struct sServerSocket* ss = (struct sServerSocket*)GLOBAL_CALLOC(1, sizeof(struct sServerSocket));
    if (!ss) { close(fd); return NULL; }
    ss->fd = fd; ss->backLog = 10; return ss;
}

PAL_API void
ServerSocket_listen(ServerSocket self)
{
    if (!self) return;
    int backlog = self->backLog > 0 ? self->backLog : 10;
    int r = listen(self->fd, backlog);
    (void)r;
}

PAL_API Socket
ServerSocket_accept(ServerSocket self)
{
    if (!self) return NULL;
    struct sockaddr_storage ss; socklen_t sl = sizeof(ss);
    int cfd = accept(self->fd, (struct sockaddr*)&ss, &sl);
    if (cfd < 0) return NULL;
    int fl = fcntl(cfd, F_GETFL, 0);
    fcntl(cfd, F_SETFL, fl | O_NONBLOCK);
    struct sSocket* s = (struct sSocket*)GLOBAL_CALLOC(1, sizeof(struct sSocket));
    if (!s) { close(cfd); return NULL; }
    s->fd = cfd; s->connectTimeout = 5000; return s;
}

PAL_API void
Socket_activateTcpKeepAlive(Socket self, int idleTime, int interval, int count)
{
    if (!self) return;
    int one = 1;
    (void)setsockopt(self->fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
#ifdef TCP_KEEPIDLE
    (void)setsockopt(self->fd, IPPROTO_TCP, TCP_KEEPIDLE, &idleTime, sizeof(idleTime));
#endif
#ifdef TCP_KEEPINTVL
    (void)setsockopt(self->fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
#endif
#ifdef TCP_KEEPCNT
    (void)setsockopt(self->fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
#endif
}

PAL_API void
ServerSocket_setBacklog(ServerSocket self, int backlog)
{
    if (self) self->backLog = backlog;
}

PAL_API void
ServerSocket_destroy(ServerSocket self)
{
    if (!self) return;
    if (self->fd >= 0) close(self->fd);
    GLOBAL_FREEMEM(self);
}

PAL_API Socket
TcpSocket_create(void)
{
    int fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) return NULL;
    int fl = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    struct sSocket* s = (struct sSocket*)GLOBAL_CALLOC(1, sizeof(struct sSocket));
    if (!s) { close(fd); return NULL; }
    s->fd = fd; s->connectTimeout = 5000; return s;
}

PAL_API void
Socket_setConnectTimeout(Socket self, uint32_t timeoutInMs)
{
    if (self) self->connectTimeout = timeoutInMs;
}

PAL_API bool
Socket_bind(Socket self, const char* srcAddress, int srcPort)
{
    if (!self) return false;
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET6; hints.ai_socktype = SOCK_STREAM; hints.ai_flags = AI_PASSIVE;
    char portstr[16]; snprintf(portstr, sizeof(portstr), "%d", srcPort);
    int rc = getaddrinfo(srcAddress, (srcPort > 0) ? portstr : NULL, &hints, &res);
    if (rc != 0 || !res) return false;
    bool ok = (bind(self->fd, res->ai_addr, res->ai_addrlen) == 0);
    freeaddrinfo(res);
    return ok;
}

PAL_API bool
Socket_connect(Socket self, const char* address, int port)
{
    if (!self) return false;
    struct addrinfo hints = {0}, *res = NULL; char portstr[16];
    hints.ai_family = AF_INET6; hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    int rc = getaddrinfo(address, portstr, &hints, &res);
    if (rc != 0 || !res) return false;
    int c = connect(self->fd, res->ai_addr, res->ai_addrlen);
    if (c < 0 && errno == EINPROGRESS) {
        struct pollfd p = { .fd = self->fd, .events = POLLOUT };
        int pr = poll(&p, 1, (int)self->connectTimeout);
        if (pr <= 0) { freeaddrinfo(res); return false; }
        int err = 0; socklen_t elen = sizeof(err);
        getsockopt(self->fd, SOL_SOCKET, SO_ERROR, &err, &elen);
        freeaddrinfo(res);
        return (err == 0);
    }
    freeaddrinfo(res);
    return (c == 0);
}

PAL_API bool
Socket_connectAsync(Socket self, const char* address, int port)
{
    if (!self) return false;
    struct addrinfo hints = {0}, *res = NULL; char portstr[16];
    hints.ai_family = AF_INET6; hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    int rc = getaddrinfo(address, portstr, &hints, &res);
    if (rc != 0 || !res) return false;
    int c = connect(self->fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (c == 0) return true;
    return (errno == EINPROGRESS);
}

PAL_API SocketState
Socket_checkAsyncConnectState(Socket self)
{
    if (!self) return SOCKET_STATE_FAILED;
    int err = 0; socklen_t elen = sizeof(err);
    if (getsockopt(self->fd, SOL_SOCKET, SO_ERROR, &err, &elen) < 0) return SOCKET_STATE_FAILED;
    return (err == 0) ? SOCKET_STATE_CONNECTED : SOCKET_STATE_FAILED;
}

PAL_API int
Socket_read(Socket self, uint8_t* buf, int size)
{
    if (!self) return -1;
    int r = recv(self->fd, buf, (size_t)size, MSG_DONTWAIT);
    if (r == 0) return -1;
    if (r < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return -1;
    }
    return r;
}

PAL_API int
Socket_write(Socket self, uint8_t* buf, int size)
{
    if (!self) return -1;
    int r = send(self->fd, buf, (size_t)size, 0);
    return (r < 0) ? -1 : r;
}

PAL_API char*
Socket_getLocalAddress(Socket self)
{
    if (!self) return NULL;
    struct sockaddr_storage ss; socklen_t sl = sizeof(ss);
    if (getsockname(self->fd, (struct sockaddr*)&ss, &sl) < 0) return NULL;
    char ip[48];
    int port = 0; bool v6 = false;
    if (ss.ss_family == AF_INET6) {
        struct sockaddr_in6* a6 = (struct sockaddr_in6*)&ss;
        net_addr_ntop(AF_INET6, &a6->sin6_addr, ip, sizeof(ip));
        port = ntohs(a6->sin6_port); v6 = true;
    } else if (ss.ss_family == AF_INET) {
        struct sockaddr_in* a4 = (struct sockaddr_in*)&ss;
        net_addr_ntop(AF_INET, &a4->sin_addr, ip, sizeof(ip));
        port = ntohs(a4->sin_port);
    } else {
        return NULL;
    }
    size_t iplen = strlen(ip);
    size_t need = iplen + (v6 ? 3 : 1) + 5 /* port digits */ + 1 /* NUL */;
    char* out = (char*)GLOBAL_MALLOC(need);
    if (!out) return NULL;
    if (v6)
        snprintf(out, need, "[%s]:%d", ip, port);
    else
        snprintf(out, need, "%s:%d", ip, port);
    LZDBG("tcp:getLocal fd=%d %s", self->fd, out);
    return out;
}

PAL_API char*
Socket_getPeerAddress(Socket self)
{
    if (!self) return NULL;
    struct sockaddr_storage ss; socklen_t sl = sizeof(ss);
    if (getpeername(self->fd, (struct sockaddr*)&ss, &sl) < 0) return NULL;
    char ip[48];
    int port = 0; bool v6 = false;
    if (ss.ss_family == AF_INET6) {
        struct sockaddr_in6* a6 = (struct sockaddr_in6*)&ss;
        net_addr_ntop(AF_INET6, &a6->sin6_addr, ip, sizeof(ip));
        port = ntohs(a6->sin6_port); v6 = true;
    } else if (ss.ss_family == AF_INET) {
        struct sockaddr_in* a4 = (struct sockaddr_in*)&ss;
        net_addr_ntop(AF_INET, &a4->sin_addr, ip, sizeof(ip));
        port = ntohs(a4->sin_port);
    } else {
        return NULL;
    }
    size_t iplen = strlen(ip);
    size_t need = iplen + (v6 ? 3 : 1) + 5 /* port digits */ + 1 /* NUL */;
    char* out = (char*)GLOBAL_MALLOC(need);
    if (!out) return NULL;
    if (v6)
        snprintf(out, need, "[%s]:%d", ip, port);
    else
        snprintf(out, need, "%s:%d", ip, port);
    LZDBG("tcp:getPeer fd=%d %s", self->fd, out);
    return out;
}

PAL_API char*
Socket_getPeerAddressStatic(Socket self, char* peerAddressString)
{
    if (!self || !peerAddressString) return NULL;
    struct sockaddr_storage ss; socklen_t sl = sizeof(ss);
    if (getpeername(self->fd, (struct sockaddr*)&ss, &sl) < 0) return NULL;
    if (ss.ss_family == AF_INET6) {
        char ip[48];
        struct sockaddr_in6* a6 = (struct sockaddr_in6*)&ss;
        net_addr_ntop(AF_INET6, &a6->sin6_addr, ip, sizeof(ip));
        snprintf(peerAddressString, 60, "[%s]:%u", ip, ntohs(a6->sin6_port));
    } else {
        char ip[24];
        struct sockaddr_in* a4 = (struct sockaddr_in*)&ss;
        net_addr_ntop(AF_INET, &a4->sin_addr, ip, sizeof(ip));
        snprintf(peerAddressString, 60, "%s:%u", ip, ntohs(a4->sin_port));
    }
    return peerAddressString;
}

PAL_API void
Socket_destroy(Socket self)
{
    if (!self) return;
    if (self->fd >= 0) close(self->fd);
    GLOBAL_FREEMEM(self);
}

PAL_API UdpSocket
UdpSocket_create(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) return NULL;
    struct sUdpSocket* us = (struct sUdpSocket*)GLOBAL_CALLOC(1, sizeof(struct sUdpSocket));
    if (!us) { close(fd); return NULL; }
    us->fd = fd; return us;
}

PAL_API UdpSocket
UdpSocket_createIpV6(void)
{
    int fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) return NULL;
    struct sUdpSocket* us = (struct sUdpSocket*)GLOBAL_CALLOC(1, sizeof(struct sUdpSocket));
    if (!us) { close(fd); return NULL; }
    us->fd = fd; return us;
}

PAL_API bool
UdpSocket_addGroupMembership(UdpSocket self, const char* multicastAddress)
{
    (void)self; (void)multicastAddress; return false;
}

PAL_API bool
UdpSocket_setMulticastTtl(UdpSocket self, int ttl)
{
    (void)self; (void)ttl; return false;
}

PAL_API bool
UdpSocket_bind(UdpSocket self, const char* address, int port)
{
    if (!self) return false;
    struct sockaddr_in6 a6 = {0};
    a6.sin6_family = AF_INET6; a6.sin6_port = htons((uint16_t)port); a6.sin6_addr = in6addr_any;
    return (bind(self->fd, (struct sockaddr*)&a6, sizeof(a6)) == 0);
}

PAL_API bool
UdpSocket_sendTo(UdpSocket self, const char* address, int port, uint8_t* msg, int msgSize)
{
    if (!self || !address) return false;
    struct addrinfo hints = {0}, *res = NULL; char portstr[16];
    hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_DGRAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    if (getaddrinfo(address, portstr, &hints, &res) != 0 || !res) return false;
    int rc = sendto(self->fd, msg, (size_t)msgSize, 0, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    return (rc == msgSize);
}

PAL_API int
UdpSocket_receiveFrom(UdpSocket self, char* address, int maxAddrSize, uint8_t* msg, int msgSize)
{
    if (!self) return -1;
    struct sockaddr_storage ss; socklen_t sl = sizeof(ss);
    int r = recvfrom(self->fd, msg, (size_t)msgSize, MSG_DONTWAIT, (struct sockaddr*)&ss, &sl);
    if (r < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return -1;
    }
    if (address && maxAddrSize > 0) {
        if (ss.ss_family == AF_INET6) {
            char ip[48]; struct sockaddr_in6* a6 = (struct sockaddr_in6*)&ss;
            net_addr_ntop(AF_INET6, &a6->sin6_addr, ip, sizeof(ip));
            snprintf(address, (size_t)maxAddrSize, "[%s]:%u", ip, ntohs(a6->sin6_port));
        } else {
            char ip[24]; struct sockaddr_in* a4 = (struct sockaddr_in*)&ss;
            net_addr_ntop(AF_INET, &a4->sin_addr, ip, sizeof(ip));
            snprintf(address, (size_t)maxAddrSize, "%s:%u", ip, ntohs(a4->sin_port));
        }
    }
    return r;
}
