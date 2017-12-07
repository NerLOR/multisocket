


#include <sys/socket.h>
#include <memory.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <time.h>
#include <poll.h>
#include <netdb.h>

#include <lua5.3/lua.h>
#include <lua5.3/lualib.h>
#include <lua5.3/lauxlib.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <signal.h>



/**
 * Number of bytes used to create a buffer in receive or send functions
 */
#define MULTISOCKET_BUFFER_SIZE 6000 // Ethernetframe size = 1500, TLS/SSL size = 16384

/**
 * Primary socket identifier: multisocket
 */
typedef struct {
    /**
     * Filedescriptor of the socket
     */
    int socket;

    /**
     * If the connection is encrypted, use the ssl descriptor
     */
    SSL *ssl;

    /**
     * Creation-time of the socket in nanoseconds
     */
    long startT;

    /**
     * Time of the last connection signal in nanoseconds
     */
    long lastT;

    /**
     * Number of bytes received by the socket
     */
    long recB;

    /**
     * Number of bytes sent by the socket
     */
    long sndB;

    /**
     * Is the socket a listener?
     */
    unsigned char listen:1;

    /**
     * Is the socket already connected?
     */
    unsigned char conn:1;

    /**
     * Is the socket on serverside?
     */
    unsigned char servers:1;

    /**
     * Is the socket on clientside?
     */
    unsigned char clients:1;

    /**
     * Is the connection SSL/TLS encrypted?
     */
    unsigned char enc:1;

    /**
     * Is the socket a TCP socket?
     */
    unsigned char tcp:1;

    /**
     * Is the socket a UDP socket?
     */
    unsigned char udp:1;

    /**
     * Is the socket a IPv6 socket?
     */
    unsigned char ipv6:1;

    /**
     * Is the socket a IPv4 socket?
     */
    unsigned char ipv4:1;

} Multisocket;

/**
 * Returns the current UNIX-time in nanoseconds.
 * @return UNIX-time in nanoseconds
 */
static long getcurrenttime() {
    struct timespec tv;
    clockid_t clock = 0;
    clock_gettime(clock, &tv);
    return tv.tv_sec * 1000000000 + tv.tv_nsec;
}


#include "ssl.h"
#include "tcp.h"
#include "support.h"



/**
 * Initializer called by Lua
 * @return1 [Table] Library
 */
int luaopen_multisocket(lua_State *L) {

    multi_ssl_init();
    signal(SIGPIPE, SIG_IGN);

    /**
     * The Metatable for TCP Sockets, IPv6 and IPv4
     */
    static const luaL_Reg mt_tcp[] = {
            {"bind",                multi_tcp_bind},
            {"encrypt",             multi_tcp_encrypt},
            {"listen",              multi_tcp_listen},
            {"accept",              multi_tcp_accept},
            {"connect",             multi_tcp_connect},
            {"receive",             multi_tcp_receive},
            {"receiveLine",         multi_tcp_receive_line},
            {"send",                multi_tcp_send},
            {"close",               multi_tcp_close},
            {"pointer",             multi_getpointer},
            {"getSocketAddress",    multi_tcp_get_sockaddr},
            {"getSocketPort",       multi_tcp_get_sockport},
            {"getSocketName",       multi_tcp_get_sockname},
            {"getPeerAddress",      multi_tcp_get_peeraddr},
            {"getPeerPort",         multi_tcp_get_peerport},
            {"getPeerName",         multi_tcp_get_peername},
            {"setTimeout",          multi_tcp_set_timeout},
            {"getDuration",         multi_get_duration},
            {"getStartTime",        multi_get_starttime},
            {"getLastSignalTime",   multi_get_lasttime},
            {"getSentBytes",        multi_get_sent},
            {"getReceivedBytes",    multi_get_received},
            {"isServerSide",        multi_tcp_is_server_side},
            {"isClientSide",        multi_tcp_is_client_side},
            {"isEncrypted",         multi_tcp_is_encrypted},
            {"isIpv6",              multi_is_ipv6},
            {"isIpv4",              multi_is_ipv4},
            {NULL, NULL}
    };

    luaL_newmetatable(L, "multisocket_tcp");
    lua_pushstring(L, "__metatable");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    lua_pushstring(L, "__index");
    luaL_newlib(L, mt_tcp);
    lua_settable(L, -3);

    //lua_pushstring(L, "__tostring");
    //lua_pushcfunction(L, multi_tostring);
    //lua_settable(L, -3);


    /**
     * The main Library Table
     */
    static const luaL_Reg lib_functions[] = {
            {"tcp6",    multi_tcp6},        // Create new IPv6 Socket
            {"tcp4",    multi_tcp4},        // Create new IPv4 Socket
            {"pointer", multi_pointer},     // Create new Socket from Pointer
            {"select",  multi_select},      // Wait until a Socket State has changed
            {"open",    multi_open},        // Create and connects an IPv6/IPv4 Socket
            {"time",    multi_time},        // Get the current UNIX-Time
            {NULL, NULL}
    };
    luaL_newlib(L, lib_functions);  // Create a new Table and put it on the Stack
    return 1;  // Return the last Item on the Stack
}


