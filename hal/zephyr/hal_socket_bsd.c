/*
 * Zephyr HAL stubs for socket/handleset API
 * Minimal implementations returning failure/neutral values to satisfy linking.
 */

#include "hal_socket.h"
#include "lib_memory.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
LOG_MODULE_REGISTER(libiec_hal_socket, LOG_LEVEL_INF);

struct sSocket {
    int dummy;
    uint32_t connectTimeout;
};

struct sServerSocket {
    int dummy;
    int backLog;
};

struct sUdpSocket {
    int dummy;
};

struct sHandleSet {
    int nfds;
};

PAL_API HandleSet
Handleset_new(void)
{
    LOG_ERR("Handleset_new()");
    return (HandleSet)GLOBAL_CALLOC(1, sizeof(struct sHandleSet));
}

PAL_API void
Handleset_reset(HandleSet self)
{
    LOG_ERR("Handleset_reset(self=%p)", self);
    (void)self;
}

PAL_API void
Handleset_addSocket(HandleSet self, const Socket sock)
{
    LOG_ERR("Handleset_addSocket(self=%p, sock=%p)", self, sock);
    (void)self; (void)sock;
}

void
Handleset_removeSocket(HandleSet self, const Socket sock)
{
    LOG_ERR("Handleset_removeSocket(self=%p, sock=%p)", self, sock);
    (void)self; (void)sock;
}

PAL_API int
Handleset_waitReady(HandleSet self, unsigned int timeoutMs)
{
    LOG_ERR("Handleset_waitReady(self=%p, timeoutMs=%u)", self, timeoutMs);
    (void)self; (void)timeoutMs;
    return 0;
}

PAL_API void
Handleset_destroy(HandleSet self)
{
    LOG_ERR("Handleset_destroy(self=%p)", self);
    if (self) GLOBAL_FREEMEM(self);
}

PAL_API ServerSocket
TcpServerSocket_create(const char* address, int port)
{
    LOG_ERR("TcpServerSocket_create(addr=%s, port=%d)", address ? address : "(null)", port);
    (void)address; (void)port;
    return NULL;
}

PAL_API void
ServerSocket_listen(ServerSocket self)
{
    LOG_ERR("ServerSocket_listen(self=%p)", self);
    (void)self;
}

PAL_API Socket
ServerSocket_accept(ServerSocket self)
{
    LOG_ERR("ServerSocket_accept(self=%p)", self);
    (void)self;
    return NULL;
}

PAL_API void
Socket_activateTcpKeepAlive(Socket self, int idleTime, int interval, int count)
{
    LOG_ERR("Socket_activateTcpKeepAlive(self=%p, idle=%d, intvl=%d, cnt=%d)", self, idleTime, interval, count);
    (void)self; (void)idleTime; (void)interval; (void)count;
}

PAL_API void
ServerSocket_setBacklog(ServerSocket self, int backlog)
{
    LOG_ERR("ServerSocket_setBacklog(self=%p, backlog=%d)", self, backlog);
    if (self) {
        self->backLog = backlog;
    }
    (void)backlog;
}

PAL_API void
ServerSocket_destroy(ServerSocket self)
{
    LOG_ERR("ServerSocket_destroy(self=%p)", self);
    if (self) GLOBAL_FREEMEM(self);
}

PAL_API Socket
TcpSocket_create(void)
{
    LOG_ERR("TcpSocket_create()");
    Socket s = (Socket)GLOBAL_CALLOC(1, sizeof(struct sSocket));
    if (s) {
        s->connectTimeout = 5000;
    }
    return s;
}

PAL_API void
Socket_setConnectTimeout(Socket self, uint32_t timeoutInMs)
{
    LOG_ERR("Socket_setConnectTimeout(self=%p, timeoutMs=%u)", self, (unsigned)timeoutInMs);
    if (self) self->connectTimeout = timeoutInMs;
}

PAL_API bool
Socket_bind(Socket self, const char* srcAddress, int srcPort)
{
    LOG_ERR("Socket_bind(self=%p, addr=%s, port=%d)", self, srcAddress ? srcAddress : "(null)", srcPort);
    (void)self; (void)srcAddress; (void)srcPort;
    return false;
}

PAL_API bool
Socket_connect(Socket self, const char* address, int port)
{
    LOG_ERR("Socket_connect(self=%p, addr=%s, port=%d)", self, address ? address : "(null)", port);
    (void)self; (void)address; (void)port;
    return false;
}

PAL_API bool
Socket_connectAsync(Socket self, const char* address, int port)
{
    LOG_ERR("Socket_connectAsync(self=%p, addr=%s, port=%d)", self, address ? address : "(null)", port);
    (void)self; (void)address; (void)port;
    return false;
}

PAL_API SocketState
Socket_checkAsyncConnectState(Socket self)
{
    LOG_ERR("Socket_checkAsyncConnectState(self=%p)", self);
    (void)self;
    return SOCKET_STATE_FAILED;
}

PAL_API int
Socket_read(Socket self, uint8_t* buf, int size)
{
    LOG_ERR("Socket_read(self=%p, size=%d)", self, size);
    (void)self; (void)buf; (void)size;
    return -1;
}

PAL_API int
Socket_write(Socket self, uint8_t* buf, int size)
{
    LOG_ERR("Socket_write(self=%p, size=%d)", self, size);
    (void)self; (void)buf; (void)size;
    return -1;
}

PAL_API char*
Socket_getLocalAddress(Socket self)
{
    LOG_ERR("Socket_getLocalAddress(self=%p)", self);
    (void)self;
    return NULL;
}

PAL_API char*
Socket_getPeerAddress(Socket self)
{
    LOG_ERR("Socket_getPeerAddress(self=%p)", self);
    (void)self;
    return NULL;
}

PAL_API char*
Socket_getPeerAddressStatic(Socket self, char* peerAddressString)
{
    LOG_ERR("Socket_getPeerAddressStatic(self=%p)", self);
    (void)self; (void)peerAddressString;
    return NULL;
}

PAL_API void
Socket_destroy(Socket self)
{
    LOG_ERR("Socket_destroy(self=%p)", self);
    if (self) GLOBAL_FREEMEM(self);
}

PAL_API UdpSocket
UdpSocket_create(void)
{
    LOG_ERR("UdpSocket_create()");
    return NULL;
}

PAL_API UdpSocket
UdpSocket_createIpV6(void)
{
    LOG_ERR("UdpSocket_createIpV6()");
    return NULL;
}

PAL_API bool
UdpSocket_addGroupMembership(UdpSocket self, const char* multicastAddress)
{
    LOG_ERR("UdpSocket_addGroupMembership(self=%p, addr=%s)", self, multicastAddress ? multicastAddress : "(null)");
    (void)self; (void)multicastAddress;
    return false;
}

PAL_API bool
UdpSocket_setMulticastTtl(UdpSocket self, int ttl)
{
    LOG_ERR("UdpSocket_setMulticastTtl(self=%p, ttl=%d)", self, ttl);
    (void)self; (void)ttl;
    return false;
}

PAL_API bool
UdpSocket_bind(UdpSocket self, const char* address, int port)
{
    LOG_ERR("UdpSocket_bind(self=%p, addr=%s, port=%d)", self, address ? address : "(null)", port);
    (void)self; (void)address; (void)port;
    return false;
}

PAL_API bool
UdpSocket_sendTo(UdpSocket self, const char* address, int port, uint8_t* msg, int msgSize)
{
    LOG_ERR("UdpSocket_sendTo(self=%p, addr=%s, port=%d, size=%d)", self, address ? address : "(null)", port, msgSize);
    (void)self; (void)address; (void)port; (void)msg; (void)msgSize;
    return false;
}

PAL_API int
UdpSocket_receiveFrom(UdpSocket self, char* address, int maxAddrSize, uint8_t* msg, int msgSize)
{
    LOG_ERR("UdpSocket_receiveFrom(self=%p, max=%d)", self, msgSize);
    (void)self; (void)address; (void)maxAddrSize; (void)msg; (void)msgSize;
    return -1;
}
