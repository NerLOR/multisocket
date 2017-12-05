


// Import Lua Libraries
#include <lua5.3/lua.h>
#include <lua5.3/lualib.h>
#include <lua5.3/lauxlib.h>

// Import C Libraries
#include <sys/socket.h>
#include <memory.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <time.h>
#include <poll.h>
#include <netdb.h>



/**
 * Number of bytes used to create a buffer in receive or send functions
 */
#define MULTISOCKET_BUFFER_SIZE 4500 // Ethernetframe size = 1500

/**
 * Primary socket identifier: multisocket
 */
typedef struct {
    /**
     * Filedescriptor of the socket
     */
    int socket;

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
     * Is this socket on serverside?
     */
    unsigned char isSrv:1;

    /**
     * Bit 2: 0 = IPv6, 1 = IPv4
     * Bit 1: 0 = TCP, 1 = UDP
     * Bit 0: 0 = not encrypted, 1 = encrypted
     */
    unsigned char type:3;
} multisocket;

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



// Import Multisocket Libraries
#include "tcp.h"
#include "encrypt.h"



/**
 * Lua Method
 * Get the pointer to the socket
 * USE THIS FUNCTION ONLY IF YOU KNOW WHAT YOU ARE DOING!
 * @param0 [Multisocket] socket
 * @return1 [Integer] pointer / nil
 * @return2 nil / [String] error
 */
static int multi_getpointer(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to multisocket
    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    lua_pushinteger(L, (long) sock); // WARNING: USING POINTER!
    return 1;
}

/**
 * Lua Function
 * Get a Multisocket object from a pointer
 * USE THIS FUNCTION ONLY IF YOU KNOW WHAT YOU ARE DOING!
 * @param1 [Integer] pointer
 * @return1 [Multisocket] socket / nil
 * @return2 nil / [String] error
 */
static int multi_pointer(lua_State *L) {
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isinteger(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong argument types");
        return 2; // Return nil, [String] error
    }

    long pointer = (long) lua_tointeger(L, 1);
    multisocket *sock = (multisocket *) pointer;

    lua_pushlightuserdata(L, (void *) pointer); // WARNING: USING POINTER!

    if (!sock->type & 2) {
        luaL_getmetatable(L, "multisocket_tcp");
    } else {
        luaL_getmetatable(L, "multisocket_udp");
    }

    lua_setmetatable(L, -2);

    return 1; // Return [Multisocket] socket
}

/**
 * Lua Function
 * Wait until timeout or a socket status changed
 * @param1 [Table<Integer, Multisocket>] waitRead
 * @param2 [Table<Integer, Multisocket>] waitWrite
 * @param3 [Number] timeout (seconds) / nil
 * @return1 [Table<Integer, Multisocket>] readable (NOT YET IMPLEMENTED)
 * @return2 [Table<Integer, Multisocket>] writable (NOT YET IMPLEMENTED)
 */
static int multi_select(lua_State *L) {
    if (lua_gettop(L) != 2 && lua_gettop(L) != 3) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_istable(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [Table<Integer, Multisocket>] waitRead");
        return 2; // Return nil, [String] error
    } else if (!lua_istable(L, 2)){
        lua_pushnil(L);
        lua_pushstring(L, "Argument #2 has to be [Table<Integer, Multisocket>] waitWrite");
        return 2; // Return nil, [String] error
    } else if (lua_gettop(L) == 3 && !lua_isnumber(L, 3)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #3 has to be [Number] timeout (seconds)");
        return 2; // Return nil, [String] error
    }

    fd_set read, write;
    int maxfd = 0;
    FD_ZERO(&read);
    FD_ZERO(&write);

    lua_pushvalue(L, 1);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pushvalue(L, -2);
        //const char *key = lua_tostring(L, -1);
        multisocket *sock = (multisocket *) lua_touserdata(L, -2);
        if ( sock->socket > maxfd ) {
            maxfd = sock->socket;
        }
        FD_SET( sock->socket, &read );
        lua_pop(L, 2);
    }
    lua_pop(L, 1);

    lua_pushvalue(L, 2);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pushvalue(L, -2);
        //const char *key = lua_tostring(L, -1);
        multisocket *sock = (multisocket *) lua_touserdata(L, -2);
        if ( sock->socket > maxfd ) {
            maxfd = sock->socket;
        }
        FD_SET( sock->socket, &write );
        lua_pop(L, 2);
    }
    lua_pop(L, 1);

    struct timeval tv;
    if ( lua_gettop(L) == 3 ) {
        double n = lua_tonumber(L,3);
        tv.tv_sec = (long) n;
        tv.tv_usec = (long) (n - tv.tv_sec) * 1000000;
    }

    int ret = select( maxfd+1, &read, &write, NULL, &tv );
    if ( ret < 0 ) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    // TODO Implement readable and writable return values
    lua_newtable(L);
    lua_newtable(L);
    return 2; // Return [Table<Integer, Multisocket>] readable, [Table<Integer, Multisocket] writable
}

/**
 * Lua Function
 * Create a socket, bind it and connect it
 * @param1 [String] address (address or domain)
 * @param2  [Integer] port (0-65535)
 * @return1 [Multisocket] client / nil
 * @return2 nil / [String] error
 */
static int multi_open(lua_State *L) {
    // Check if there are three parameters and if they have valid values
    if (lua_gettop(L) != 2) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isstring(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [String] address (address or domain)");
        return 2; // Return nil, [String] error
    } else if (!lua_isinteger(L, 2) || lua_tointeger(L, 2) > 0xFFFF || lua_tointeger(L, 2) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #2 has to be [Integer] port (0-65535)");
        return 2; // Return nil, [String] error
    }

    // Load parameters into variables
    size_t addressLength = 0;
    const char *address = lua_tolstring(L, 1, &addressLength);
    unsigned short port = (unsigned short) lua_tointeger(L, 2);

    // Init address structs for IPv6 and IPv4
    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));

    address6.sin6_family = AF_INET6;
    address4.sin_family = AF_INET;

    struct addrinfo *result, *next;
    struct addrinfo hint;
    bzero(&hint, sizeof(hint));
    hint.ai_family = AF_INET6;

    struct sockaddr *addr;
    socklen_t addrLen6 = 0;
    socklen_t addrLen4 = 0;
    socklen_t addrLen = 0;

    while (1) {
        int ret = getaddrinfo(address, NULL, &hint, &result);
        printf("asfd\n");
        if (ret != 0 && ret != -2) {
            lua_pushnil(L);
            lua_pushstring(L, gai_strerror(ret));
            return 3; // Return nil, [String] error
        } else if (ret == 0) {
            for (next = result; next != NULL; next = next->ai_next) {
                if (addrLen6 == 0 && next->ai_family == AF_INET6) {
                    memcpy(&address6, next->ai_addr, next->ai_addrlen);
                    addrLen6 = next->ai_addrlen;
                } else if (addrLen4 == 0 && next->ai_family == AF_INET) {
                    memcpy(&address4, next->ai_addr, next->ai_addrlen);
                    addrLen4 = next->ai_addrlen;
                }
            }
        }
        if (hint.ai_family == AF_INET6) {
            hint.ai_family = AF_INET;
        } else {
            break;
        }
    }

    address6.sin6_port = htons(port);
    address4.sin_port = htons(port);

    multisocket *sock;

    while (1) {
        if (addrLen6 != 0) {
            lua_pushcfunction(L, multi_tcp6);
            lua_call(L, 0, 2);
            if (lua_isnil(L, 3)) {
                lua_pushnil(L);
                lua_pushvalue(L, 2);
                return 2; // Return nil, [String] error
            }
            sock = (multisocket *) lua_touserdata(L, 3);
        } else if (addrLen4 != 0) {
            lua_pushcfunction(L, multi_tcp4);
            lua_call(L, 0, 2);
            if (lua_isnil(L, 3)) {
                lua_pushnil(L);
                lua_pushvalue(L, 2);
                return 2; // Return nil, [String] error
            }
            sock = (multisocket *) lua_touserdata(L, 3);
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "Unable to resolve address");
            return 2; // Return nil, [String] error
        }

        if (!sock->type & 4) {
            addr = (struct sockaddr *) &address6;
            addrLen = addrLen6;
        } else if (sock->type & 4) {
            addr = (struct sockaddr *) &address4;
            addrLen = addrLen4;
        }

        lua_pushcfunction(L, multi_tcp_bind);
        lua_pushvalue(L, 1);
        lua_pushstring(L, "*");
        lua_pushinteger(L, 0);
        lua_call(L, 3, 2);
        if (lua_isnil(L, 3)) {
            lua_pushnil(L);
            lua_pushvalue(L, 2);
            return 2; // Return nil, [String] error
        }

        if (connect(sock->socket, addr, addrLen) == -1) {
            if (!sock->type & 4 && addrLen4 != 0 && addrLen6 != 0) {
                addrLen6 = 0;
                lua_pop(L,4);
                continue;
            }
            lua_pushnil(L);
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                lua_pushstring(L, "timeout");
            } else {
                lua_pushstring(L, strerror(errno));
            }
            return 2; // Return nil, [String] error
        } else {
            break;
        }
    }

    lua_pushvalue(L, 3);
    return 1; // Return [Multisocket] client

}

/**
 * Lua Function
 * Get the current systemtime in UNIX
 * @return1 [Number] time (UNIX time)
 */
static int multi_time(lua_State *L) {
    if (lua_gettop(L) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    }

    lua_pushnumber(L, getcurrenttime() / 1000000000.0);
    return 1; // Return [Number] time
}



/**
 * Initializer called by Lua
 * @return1 [Table] Library
 */
int luaopen_multisocket(lua_State *L) {

    multi_init_ssl();

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
            {"getSocketAddress",    multi_tcp_getsockaddr},
            {"getSocketPort",       multi_tcp_getsockport},
            {"getSocketName",       multi_tcp_getsockname},
            {"getPeerAddress",      multi_tcp_getpeeraddr},
            {"getPeerPort",         multi_tcp_getpeerport},
            {"getPeerName",         multi_tcp_getpeername},
            {"setTimeout",          multi_tcp_settimeout},
            {"getDuration",         multi_getduration},
            {"getStartTime",        multi_getstarttime},
            {"getLastSignalTime",   multi_getlasttime},
            {"getSentBytes",        multi_getsnd},
            {"getReceivedBytes",    multi_getrec},
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


