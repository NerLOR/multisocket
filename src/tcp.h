
#include <sys/socket.h>
#include <memory.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <time.h>
#include <poll.h>
#include <netdb.h>


#include "encrypt.h"



/**
 * Lua Function
 * Create a new TCP/IPv6 socket
 * @return1 [Multisocket] socket / nil
 * @return2 nil / [String] error
 */
static int multi_tcp6(lua_State *L) {
    // Check if there are any parameters
    if (lua_gettop(L) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    }

    // Init socket filedescriptor
    int desc = 0;
    if ((desc = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    // Allocate memory for multisocket
    multisocket *sock = (multisocket *) lua_newuserdata(L, sizeof(multisocket));
    sock->socket = desc; // Set the socket filedescriptor
    sock->startT = getcurrenttime(); // Set connection start time in nanoseconds
    sock->lastT = getcurrenttime();  // Set last signal time in nanoseconds
    sock->recB = 0;  // Init received bytes
    sock->sndB = 0;  // Init sent bytes
    sock->isSrv = 0; // Socket is clientside
    sock->type = 0;  // Type = TCP/IPv6

    luaL_getmetatable(L, "multisocket_tcp"); // Get multisocket_tcp metatable
    lua_setmetatable(L, -2); // Set the metatable to the multisocket

    return 1; // Return [Multisocket] socket
}

/**
 * Lua Function
 * Create a new TCP/IPv4 socket
 * @return1 [Multisocket] socket / nil
 * @return2 nil / [String] error
 */
static int multi_tcp4(lua_State *L) {
    // Check if there are any parameters
    if (lua_gettop(L) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    }

    // Init socket filedescriptor
    int desc = 0;
    if ((desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    // Allocate memory for multisocket
    multisocket *sock = (multisocket *) lua_newuserdata(L, sizeof(multisocket));
    sock->socket = desc; // Set the socket filedescriptor
    sock->startT = getcurrenttime(); // Set connection start time in nanoseconds
    sock->lastT = getcurrenttime();  // Set last signal time in nanoseconds
    sock->recB = 0;  // Init received bytes
    sock->sndB = 0;  // Init sent bytes
    sock->isSrv = 0; // Socket is clientside
    sock->type = 4;  // Type = TCP/IPv4

    luaL_getmetatable(L, "multisocket_tcp"); // Get multisocket_tcp metatable
    lua_setmetatable(L, -2); // Set the metatable to the multisocket

    return 1; // Return [Multisocket] socket
}

/**
 * Lua Method
 * Bind the socket to an address an a port
 * @param0 [Multisocket] socket (TCP)
 * @param1 [String] address
 * @param2 [Integer] port (0-65535)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_bind(lua_State *L) {
    // Check if there are three parameters and if they have valid values
    if (lua_gettop(L) != 3) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    } else if (!lua_isstring(L, 2)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [String] address");
        return 2; // Return nil, [String] error
    } else if (!lua_isinteger(L, 3) || lua_tointeger(L, 3) > 0xFFFF || lua_tointeger(L, 3) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #2 has to be [Integer] port (0-65535)");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to multisocket
    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    // Load parameters into variables
    size_t addressLength = 0;
    const char *address = lua_tolstring(L, 2, &addressLength);
    unsigned short port = (unsigned short) lua_tointeger(L, 3);

    // Init address structs for IPv6 and IPv4
    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));

    if (!sock->type & 4) {
        // TCP/IPv6
        address6.sin6_family = AF_INET6;
        if (addressLength == 1 && address[0] == '*') {
            address6.sin6_addr = in6addr_any; // The OS decides which address it uses
        } else if (inet_pton(AF_INET6, address, &address6.sin6_addr) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
        // The address is set in inet_pton()
    } else if (sock->type & 4) {
        // TCP/IPv4
        address4.sin_family = AF_INET;
        if (addressLength == 1 && address[0] == '*') {
            address4.sin_addr.s_addr = INADDR_ANY; // The OS decides which address it uses
        } else if (inet_pton(AF_INET, address, &address4.sin_addr) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    }

    // Copy the port to sin_port in network byte order
    address6.sin6_port = htons(port);
    address4.sin_port = htons(port);

    // Get the right address type and address size
    struct sockaddr *socketAddress;
    socklen_t addressSize;
    if (!sock->type & 4) {
        socketAddress = (struct sockaddr *)&address6;
        addressSize = sizeof(address6);
    } else if (sock->type & 4) {
        socketAddress = (struct sockaddr *)&address4;
        addressSize = sizeof(address4);
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "Something went wrong, invalid socket type");
        return 2; // Return nil, [String] error
    }

    // Bind the socket to address and port
    if (bind(sock->socket, socketAddress, addressSize) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

/**
 * Lua Method
 * Mark the socket as passive socket
 * The socket will be used to accept incoming connection requests
 * @param0 [Multisocket] socket (TCP)
 * @param1 [Integer] backlog (0-2147483647)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_listen(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 2) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    } else if (!lua_isinteger(L, 2) || lua_tointeger(L, 2) > 0x7FFFFFFF || lua_tointeger(L, 2) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [Integer] backlog (0-2147483647)");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to multisocket
    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    // Load parameters into variables
    int backlog = (int) lua_tointeger(L, 2);

    // Use the socket as listener
    if (listen(sock->socket, backlog) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

/**
 * Lua Method
 * Wait until a connection is ready to be accepted
 * @param0 [Multisocket] socket (TCP)
 * @return1 [Multisocket] client / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_accept(lua_State *L) {
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

    // Init address variables
    struct sockaddr *address;
    socklen_t len;

    if (!sock->type & 6) {
        // TCP/IPv6
        struct sockaddr_in6 address6;
        bzero(&address6, sizeof(address6));
        len = sizeof(address6);
        address = (struct sockaddr *) &address6;
    } else if (!sock->type & 4) {
        // TCP/IPv4
        struct sockaddr_in address4;
        bzero(&address4, sizeof(address4));
        len = sizeof(address4);
        address = (struct sockaddr *) &address4;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "Something went wrong, invalid socket type");
        return 2; // Return nil, [String] error
    }

    // Accept a new incoming connection
    int desc = accept(sock->socket, address, &len);
    if (desc == -1) {
        lua_pushnil(L);
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            lua_pushstring(L, "timeout");
        } else if (errno == ECONNRESET) {
            lua_pushstring(L, "closed");
        } else {
            lua_pushstring(L, strerror(errno));
        }
        return 2; // Return nil, [String] error
    }

    // Allocate memory for multisocket
    multisocket *client = (multisocket *) lua_newuserdata(L, sizeof(multisocket));
    client->socket = desc; // Set the socket filedescriptor
    client->startT = getcurrenttime(); // Set connection start time in nanoseconds
    client->lastT = getcurrenttime();  // Set last signal time in nanoseconds
    client->recB = 0;  // Init received bytes
    client->sndB = 0;  // Init sent bytes
    client->isSrv = 1; // Socket is serverside
    client->type = sock->type & (char) 6; // Set same type as parent

    luaL_getmetatable(L, "multisocket_tcp");
    lua_setmetatable(L, -2);

    return 1; // Return [Multisocket] client
}

/**
 * Lua Method
 * @param0 [Multisocket] socket (TCP)
 * @param1 [String] address (address or domain)
 * @param2 [Integer] port (0-65535)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_connect(lua_State *L) {
    // Check if there are three parameters and if they have valid values
    if (lua_gettop(L) != 3) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    } else if (!lua_isstring(L, 2)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [String] address (address or domain)");
        return 2; // Return nil, [String] error
    } else if (!lua_isinteger(L, 3) || lua_tointeger(L, 3) > 0xFFFF || lua_tointeger(L, 3) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #2 has to be [Integer] port (0-65535)");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to multisocket
    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    // Load parameters into variables
    size_t addressLength = 0;
    const char *address = lua_tolstring(L, 2, &addressLength);
    unsigned short port = (unsigned short) lua_tointeger(L, 3);

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
    hint.ai_family = (!sock->type & 4) ? AF_INET6 : AF_INET;

    int ret = getaddrinfo(address, NULL, &hint, &result);
    if (ret != 0) {
        lua_pushnil(L);
        lua_pushstring(L, gai_strerror(ret));
        return 2; // Return nil, [String] error
    }

    struct sockaddr *addr;
    socklen_t addrLen = 0;

    for (next = result; next != NULL; next = next->ai_next) {
        addrLen = next->ai_addrlen;
        if (!sock->type & 4 && next->ai_family == AF_INET6) {
            memcpy(&address6, next->ai_addr, next->ai_addrlen);
            break;
        } else if (sock->type & 4 && next->ai_family == AF_INET) {
            memcpy(&address4, next->ai_addr, next->ai_addrlen);
            break;
        }
    }

    address6.sin6_port = htons(port);
    address4.sin_port = htons(port);

    if (!sock->type & 4 ) {
        addr = (struct sockaddr *) &address6;
    } else if (sock->type & 4) {
        addr = (struct sockaddr *) &address4;
    }

    if (connect(sock->socket, addr, addrLen) == -1) {
        lua_pushnil(L);
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            lua_pushstring(L, "timeout");
        } else {
            lua_pushstring(L, strerror(errno));
        }
        return 2; // Return nil, [String] error
    }

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

/**
 * Lua Method
 * @param0 [Multisocket] socket (TCP)
 * @param1 nil / [String] until (Length: 1-MULTISOCKET_BUFFER_SIZE) / [Integer] numBytes (64 Bit)
 * @return1 [String] data / nil
 * @return2 nil / [String] error
 * @return3 nil / [String] partData
 */
static int multi_tcp_receive(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 2 && lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        lua_pushstring(L, "");
        return 3; // Return nil, [String] error, [String] partData
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        lua_pushstring(L, "");
        return 3; // Return nil, [String] error, [String] partData
    } else if (lua_gettop(L) == 2 && ((!lua_isinteger(L, 2) || lua_tointeger(L, 2) < 0) && !lua_isstring(L, 2) && !lua_isnil(L, 2))) {
        lua_pushnil(L);
        lua_pushfstring(L, "Argument #1 has to be nil / [String] until (Length: 1-%i) / [Integer] numBytes (64 Bit)", MULTISOCKET_BUFFER_SIZE);
        lua_pushstring(L, "");
        return 3; // Return nil, [String] error, [String] partData
    }

    // Cast userdata to multisocket
    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    // Init variables
    long wantedBytes = 0;
    long wantedStringLen = 0;
    const char *wantedString = "";

    // Check which argument is passed
    char mode;
    if (lua_gettop(L) == 1 || lua_isnil(L, 2)) {
        mode = 0;
    } else if (lua_gettop(L) == 2 && lua_isinteger(L, 2)) {
        mode = 1;
        wantedBytes = lua_tointeger(L, 2);
    } else if (lua_gettop(L) == 2 && lua_isstring(L, 2)) {
        mode = 2;
        wantedString = lua_tolstring(L, 2, &wantedStringLen);
        if (wantedStringLen < 1 || wantedStringLen > MULTISOCKET_BUFFER_SIZE) {
            lua_pushnil(L);
            lua_pushfstring(L, "Argument #1 has to be nil / [String] until (Length: 1-%i) / [Integer] numBytes (64 Bit)", MULTISOCKET_BUFFER_SIZE);
            lua_pushstring(L, "");
            return 3; // Return nil, [String] error, [String] partData
        }
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "Something went wrong");
        lua_pushstring(L, "");
        return 3; // Return nil, [String] error, [String] partData
    }

    // Init lua string
    luaL_Buffer str;
    luaL_buffinit(L, &str);
    long strLen = 0;

    // Init buffer and pre-buffer
    char pre[MULTISOCKET_BUFFER_SIZE];
    char buffer[MULTISOCKET_BUFFER_SIZE];

    // Init poll filedescriptor(s) to
    struct pollfd ufds[1];
    ufds[0].fd = sock->socket;
    ufds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP;

    // Receive until the condition applies
    while (1) {
        if (mode == 0) {

            long size;
            if (sock->type & 1) {
                size = SSL_read(sock->ssl, buffer, sizeof(buffer));
            } else {
                size = recv(sock->socket, buffer, sizeof(buffer), 0);
            }
            if (size < 0) {
                lua_pushnil(L);
                if (sock->type & 1) {
                    errno = SSL_get_error(sock->ssl, size);
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    lua_pushstring(L, "timeout");
                } else if (errno == ECONNRESET) {
                    lua_pushstring(L, "closed");
                } else {
                    lua_pushstring(L, strerror(errno));
                }
                luaL_pushresultsize(&str, strLen);
                return 3; // Return nil, [String] error, [String] partData
            }
            sock->recB += size;
            sock->lastT = getcurrenttime();
            strLen += size;
            luaL_addlstring(&str, buffer, size);


        } else if (mode == 1) {

            long len = sizeof(buffer);
            if (len > wantedBytes-strLen) {
                len = wantedBytes-strLen;
            }
            long size;
            if (sock->type & 1) {
                size = SSL_read(sock->ssl, buffer, len);
            } else {
                size = recv(sock->socket, buffer, len, 0);
            }

            if (size < 0) {
                lua_pushnil(L);
                if (sock->type & 1) {
                    errno = SSL_get_error(sock->ssl, (int) size);
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    lua_pushstring(L, "timeout");
                } else if (errno == ECONNRESET) {
                    lua_pushstring(L, "closed");
                } else {
                    lua_pushstring(L, strerror(errno));
                }
                luaL_pushresultsize(&str, strLen);
                return 3; // Return nil, [String] error, [String] partData
            }
            sock->recB += size;
            sock->lastT = getcurrenttime();
            strLen += size;
            luaL_addlstring(&str, buffer, size);
            if (strLen == wantedBytes) {
                luaL_pushresult(&str);
                lua_pushnil(L);
                lua_pushnil(L);
                return 3; // Return [String] data, nil, nil
            }


        } else if (mode == 2) {

            memcpy(pre, buffer, sizeof(buffer)); // Copy buffer to pre

            long size;
            if (sock->type & 1) {
                size = SSL_peek(sock->ssl, buffer, sizeof(buffer));
            } else {
                size = recv(sock->socket, buffer, sizeof(buffer), MSG_PEEK);
            }
            if (size < 0) {
                lua_pushnil(L);
                if (sock->type & 1) {
                    errno = SSL_get_error(sock->ssl, (int) size);
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    lua_pushstring(L, "timeout");
                } else if (errno == ECONNRESET) {
                    lua_pushstring(L, "closed");
                } else {
                    lua_pushstring(L, strerror(errno));
                }
                luaL_pushresultsize(&str, strLen);
                return 3; // Return nil, [String] error, [String] partData
            }

            char *ptr = buffer - wantedStringLen + 1;
            while (1) {
                ptr = memchr(ptr, wantedString[0], size);
                if (ptr == NULL || memcmp(ptr, wantedString, wantedStringLen) == 0) {
                    break;
                }
                ptr++;
            }
            if (ptr != NULL) {
                strLen += ptr - buffer + wantedStringLen;
                luaL_addlstring(&str, buffer, ptr - buffer);
                int ret;
                if (sock->type & 1) {
                    ret = SSL_read(sock->ssl, buffer, ptr - buffer + wantedStringLen);
                } else {
                    ret = recv(sock->socket, buffer, ptr - buffer + wantedStringLen, 0);
                }
                if (ret < 0) {
                    lua_pushnil(L);
                    if (sock->type & 1) {
                        errno = SSL_get_error(sock->ssl, (int) size);
                    }
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        lua_pushstring(L, "timeout");
                    } else if (errno == ECONNRESET) {
                        lua_pushstring(L, "closed");
                    } else {
                        lua_pushstring(L, strerror(errno));
                    }
                    luaL_pushresult(&str);
                    return 3; // Return nil, [String] error, [String] partData
                }
                sock->recB += ptr - buffer + wantedStringLen;
                sock->lastT = getcurrenttime();
                luaL_pushresult(&str);
                lua_pushnil(L);
                lua_pushnil(L);
                return 3; // Return [String] data, nil, nil
            } else {
                strLen += size;
                luaL_addlstring(&str, buffer, size);
                int ret;
                if (sock->type & 1) {
                    ret = SSL_read(sock->ssl, buffer, size);
                } else {
                    ret = recv(sock->socket, buffer, size, 0);
                }
                if (ret < 0) {
                    lua_pushnil(L);
                    if (sock->type & 1) {
                        errno = SSL_get_error(sock->ssl, ret);
                    }
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        lua_pushstring(L, "timeout");
                    } else if (errno == ECONNRESET) {
                        lua_pushstring(L, "closed");
                    } else {
                        lua_pushstring(L, strerror(errno));
                    }
                    luaL_pushresult(&str);
                    return 3; // Return nil, [String] error, [String] partData
                }
                sock->recB += size;
                sock->lastT = getcurrenttime();
            }
        }

        if (poll(ufds, 1, 0) == -1) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            lua_pushstring(L, "");
            return 3; // Return nil, [String] error, [String] partData
        }

        /*if ((ufds[0].revents & POLLIN) == 0) {
            if (mode == 0) {
                luaL_pushresult(&str);
                if ((ufds[0].revents & POLLHUP) || (ufds[0].revents & POLLERR)) {
                    lua_pushstring(L, "closed");
                } else {
                    lua_pushnil(L);
                }
                lua_pushnil(L);
                return 3; // Return [String] data, [String] error, nil
            } else {
                lua_pushnil(L);
                if ((ufds[0].revents & POLLHUP) || (ufds[0].revents & POLLERR)) {
                    lua_pushstring(L, "closed");
                } else if (ufds[0].revents & POLLOUT) {
                    lua_pushstring(L, "wantwrite");
                } else {
                    lua_pushstring(L, "error");
                }
                luaL_pushresult(&str);
                return 3; // Return nil, [String] error, [String] partData
            }
        }*/
    }
}

/**
 * Lua Method
 * Receive until CRLF and return the line without CRLF
 * @param0 [Multisocket] socket (TCP)
 * @return1 [String] data / nil
 * @return2 nil / [String] error
 * @return3 nil / [String] partData
 */
static int multi_tcp_receive_line(lua_State *L) {
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

    lua_pushcfunction(L, multi_tcp_receive);
    lua_pushvalue(L, 1);
    lua_pushstring(L, "\r\n");
    lua_call(L, 2, 3);
    return 3;
}

/**
 * Lua Method
 * Send data to the peer
 * @param0 [Multisocket] socket (TCP)
 * @param1 [String] data
 * @return1 [Integer] byteNum / nil
 * @return2 nil / [String] error
 * @return3 nil / [Integer] partByteNum
 */
static int multi_tcp_send(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 2) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        lua_pushinteger(L, 0);
        return 3; // Return nil, [String] error, [Integer] partByteNum
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        lua_pushinteger(L, 0);
        return 3; // Return nil, [String] error, [Integer] partByteNum
    } else if (!lua_tostring(L, 2)) {
        lua_pushnil(L);
        lua_pushfstring(L, "Argument #1 has to be [String] data");
        lua_pushinteger(L, 0);
        return 3; // Return nil, [String] error, [Integer] partByteNum
    }

    // Cast userdata to multisocket
    multisocket *sock = (multisocket *) lua_touserdata(L, 1);
    long dataSize = 0;
    long pos = 0;
    const char *data = lua_tolstring(L, 2, &dataSize);

    // Init poll filedescriptor(s) to
    struct pollfd ufds[1];
    ufds[0].fd = sock->socket;
    ufds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP;

    while (pos < dataSize) {

        long s = MULTISOCKET_BUFFER_SIZE;
        if (s > dataSize - pos) {
            s = dataSize - pos;
        }
        long trans;
        if ( sock->type & 1 ) {
            trans = SSL_write(sock->ssl, data+pos, s);
        } else {
            trans = send(sock->socket, data+pos, s, 0);
        }
        if (trans < 0) {
            lua_pushnil(L);
            if (poll(ufds, 1, 0) == -1) {
                lua_pushnil(L);
                lua_pushstring(L, strerror(errno));
                lua_pushinteger(L, pos);
                return 3; // Return nil, [String] error, [Integer] partByteNum
            }
            if (sock->type & 1) {
                errno = SSL_get_error(sock->ssl, (int) trans);
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                lua_pushstring(L, "timeout");
            } else if (errno == ECONNRESET) {
                lua_pushstring(L, "closed");
            } else if (!sock->type & 1 && ufds[0].revents & POLLIN || sock->type & 1 && SSL_want_read(sock->ssl)) {
                lua_pushstring(L, "wantread");
            } else {
                lua_pushstring(L, strerror(errno));
            }
            lua_pushinteger(L, pos);
            return 3; // Return nil, [String] error, [Integer] partByteNum
        }
        pos += trans;
        sock->sndB += trans;
        sock->lastT = getcurrenttime();
    }

    lua_pushinteger(L, pos);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3; // Return [Integer] byteNum, nil, nil
}

/**
 * Lua Method
 * Close the socket connection
 * @param0 [Multisocket] socket (TCP)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_close(lua_State *L) {
    // Check if there are any parameters
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

    // Close the socket connection
    if (close(sock->socket) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}


/**
 * Lua Method
 * @param0 [Multisocket] socket (TCP)
 * @param1 nil / [Number] timeout (seconds)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_settimeout(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 2 && lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    } else if (lua_gettop(L) == 2 && (!lua_isnumber(L, 2) ||  lua_tonumber(L, 2) < 0)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [Number] timeout");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to multisocket
    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    // Load parameters into variables
    // If timeval time is 0 then the timeout is unlimited
    // Therefore 1 will be added to tv_usec
    struct timeval time;
    bzero(&time, sizeof(time));
    if (lua_gettop(L) == 2) {
        double sec = lua_tonumber(L, 2);
        time.tv_sec = (long) sec;
        time.tv_usec = (long) ((sec - time.tv_sec) * 1000000) + 1;
    }

    if (setsockopt(sock->socket, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time)) < 0 ||
        setsockopt(sock->socket, SOL_SOCKET, SO_SNDTIMEO, &time, sizeof(time)) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

/**
 * Lua Method
 * Get the duration the socket is created
 * @param0 [Multisocket] socket
 * @return1 [Number] duration
 */
static int multi_getduration(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    lua_pushnumber(L, ((double) getcurrenttime() - sock->startT) / 1000000000);
    return 1; // Return [Number] duration
}

/**
 * Lua Method
 * Get the time the socket was created
 * @param0 [Multisocket] socket
 * @return1 [Number] startTime (UNIX)
 */
static int multi_getstarttime(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    lua_pushnumber(L, ((double) sock->startT) / 1000000000);
    return 1; // Return [Number] startTime (UNIX)
}

/**
 * Lua Method
 * Get the time the socket got the last signal from the peer
 * @param0 [Multisocket] socket
 * @return1 [Number] lastTime (UNIX)
 */
static int multi_getlasttime(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    lua_pushnumber(L, ((double) sock->lastT) / 1000000000);
    return 1; // Return [Number] lastSignal (UNIX)
}

/**
 * Lua Method
 * Get the number of bytes sent by the socket
 * @param0 [Multisocket] socket
 * @return1 [Integer] numBytesSent
 */
static int multi_getsnd(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    lua_pushinteger(L, sock->sndB);
    return 1; // Return [Integer] numBytesSent
}

/**
 * Lua Method
 * Get the number of bytes received by the socket
 * @param0 [Multisocket] socket
 * @return1 [Integer] numBytesReceived
 */
static int multi_getrec(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    lua_pushinteger(L, sock->recB);
    return 1; // Return [Integer] numBytesReceived
}


/**
 * Lua Method
 * Get the address of the socket
 * @param0 [Multisocket] socket
 * @return1 [String] address / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_getsockaddr(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    char string[INET6_ADDRSTRLEN+INET_ADDRSTRLEN];
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));
    bzero(string, sizeof(string));
    socklen_t addrLen;

    struct sockaddr *address;
    if (!sock->type & 4) {
        address = (struct sockaddr *) &address6;
        addrLen = sizeof(address6);
    } else if (sock->type & 4) {
        address = (struct sockaddr *) &address4;
        addrLen = sizeof(address4);
    }

    if (getsockname(sock->socket, address, &addrLen) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    if (!sock->type & 4) {
        if (inet_ntop(AF_INET6, &address6.sin6_addr, string, sizeof(string)) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    } else if (sock->type & 4) {
        if (inet_ntop(AF_INET, &address4.sin_addr, string, sizeof(string)) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    }

    lua_pushstring(L, string);
    return 1; // Return [String] address
}

/**
 * Lua Method
 * Get the port of the socket
 * @param0 [Multisocket] socket
 * @return1 [Integer] port / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_getsockport(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));
    socklen_t addrLen;

    struct sockaddr *address;
    if (!sock->type & 4) {
        address = (struct sockaddr *) &address6;
        addrLen = sizeof(address6);
    } else if (sock->type & 4) {
        address = (struct sockaddr *) &address4;
        addrLen = sizeof(address4);
    }

    if (getsockname(sock->socket, address, &addrLen) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    if (!sock->type & 4) {
        lua_pushinteger(L, ntohs(((struct sockaddr_in6 *) address)->sin6_port));
    } else if (sock->type & 4) {
        lua_pushinteger(L, ntohs(((struct sockaddr_in *) address)->sin_port));
    }
    return 1; // Return [Integer] port
}

/**
 * Lua Method
 * Get the address and port of the socket
 * @param0 [Multisocket] socket (TCP)
 * @return1 [String] name / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_getsockname(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    luaL_Buffer str;
    luaL_buffinit(L, &str);

    lua_pushcfunction(L, multi_tcp_getsockaddr);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 2);
    if (lua_isnil(L, 2)) {
        lua_pushnil(L);
        lua_pushvalue(L, 3);
        return 2;
    }
    if (!sock->type & 4) {
        luaL_addstring(&str, "[");
        luaL_addstring(&str, lua_tostring(L, 2));
        luaL_addstring(&str, "]:");
    } else if (sock-> type & 4) {
        luaL_addstring(&str, lua_tostring(L, 2));
        luaL_addstring(&str, ":");
    }

    lua_pushcfunction(L, multi_tcp_getsockport);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 2);
    if (lua_isnil(L, 2)) {
        lua_pushnil(L);
        lua_pushvalue(L, 3);
        return 2;
    }
    luaL_addstring(&str, lua_tostring(L, 0));

    luaL_pushresult(&str);
    return 1; // Return [String] name
}

/**
 * Lua Method
 * Get the address of the peer
 * @param0 [Multisocket] socket
 * @return1 [String] address / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_getpeeraddr(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    char string[INET6_ADDRSTRLEN+INET_ADDRSTRLEN];
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));
    bzero(string, sizeof(string));
    socklen_t addrLen;

    struct sockaddr *address;
    if (!sock->type & 4) {
        address = (struct sockaddr *) &address6;
        addrLen = sizeof(address6);
    } else if (sock->type & 4) {
        address = (struct sockaddr *) &address4;
        addrLen = sizeof(address4);
    }

    if (getpeername(sock->socket, address, &addrLen) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    if (!sock->type & 4) {
        if (inet_ntop(AF_INET6, &address6.sin6_addr, string, sizeof(string)) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    } else if (sock->type & 4) {
        if (inet_ntop(AF_INET, &address4.sin_addr, string, sizeof(string)) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    }

    lua_pushstring(L, string);
    return 1; // Return [String] address
}

/**
 * Lua Method
 * Get the port of the peer
 * @param0 [Multisocket] socket
 * @return1 [Integer] port / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_getpeerport(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));
    socklen_t addrLen;

    struct sockaddr *address;
    if (!sock->type & 4) {
        address = (struct sockaddr *) &address6;
        addrLen = sizeof(address6);
    } else if (sock->type & 4) {
        address = (struct sockaddr *) &address4;
        addrLen = sizeof(address4);
    }

    if (getpeername(sock->socket, address, &addrLen) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    if (!sock->type & 4) {
        lua_pushinteger(L, ntohs(((struct sockaddr_in6 *) address)->sin6_port));
    } else if (sock->type & 4) {
        lua_pushinteger(L, ntohs(((struct sockaddr_in *) address)->sin_port));
    }
    return 1; // Return [Integer] port
}

/**
 * Lua Method
 * Get the address and port of the peer
 * @param0 [Multisocket] socket (TCP)
 * @return1 [String] name / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_getpeername(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    multisocket *sock = (multisocket *) lua_touserdata(L, 1);

    luaL_Buffer str;
    luaL_buffinit(L, &str);

    lua_pushcfunction(L, multi_tcp_getpeeraddr);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 2);
    if (lua_isnil(L, 2)) {
        lua_pushnil(L);
        lua_pushvalue(L, 3);
        return 2;
    }
    if (!sock->type & 4) {
        luaL_addstring(&str, "[");
        luaL_addstring(&str, lua_tostring(L, 2));
        luaL_addstring(&str, "]:");
    } else if (sock->type & 4) {
        luaL_addstring(&str, lua_tostring(L, 2));
        luaL_addstring(&str, ":");
    }

    lua_pushcfunction(L, multi_tcp_getpeerport);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 2);
    if (lua_isnil(L, 2)) {
        lua_pushnil(L);
        lua_pushvalue(L, 3);
        return 2;
    }
    luaL_addstring(&str, lua_tostring(L, 0));

    luaL_pushresult(&str);
    return 1; // Return [String] name
}


