

static const unsigned char BASE64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\0";

/**
 * Lua Function
 * Encode a string into base64
 * @param1 [String] plain
 * @return1 [String] encoded / nil
 * @return2 nil / [String] error
 */
int base64_encode(lua_State *L) {
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isstring(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [String] plaintext");
        return 2; // Return nil, [String] error
    }

    size_t len = 0;
    const char *in = lua_tolstring(L, 1, &len);

    luaL_Buffer out;
    luaL_buffinit(L, &out);

    for (size_t i = 0; i < len; i+=3) {
        int stream = 0;
        for (char j = 0; j < 3; j++) {
            stream <<= 8;
            if (i + j < len) {
                stream |= in[i+j];
            }
        }
        for (char j = 0; j < 4; j++) {
            int pos = (stream >> ((3 - j) * 6)) & 0x3F;
            if (pos == 0 && i + j - 2 >= len) {
                luaL_addchar(&out, '=');
            } else {
                luaL_addchar(&out, BASE64[pos]);
            }
        }
    }

    luaL_pushresult(&out);
    return 1; // Return [String] encoded
}

/**
 * Lua Function
 * Decode a base64 string back to plain
 * @param1 [String] encoded
 * @return1 [String] plaintext / nil
 * @return2 nil / [String] error
 */
int base64_decode(lua_State *L) {
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isstring(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [String] plaintext");
        return 2; // Return nil, [String] error
    }

    size_t len = 0;
    const char *in = lua_tolstring(L, 1, &len);

    luaL_Buffer out;
    luaL_buffinit(L, &out);

    for (size_t i = 0; i < len; i+=4) {
        int stream = 0;
        for (char j = 0; j < 4; j++) {
            stream <<= 6;
            if (i + j < len) {
                char ch = in[i+j];
                unsigned int pos = (unsigned int) ((strchr(BASE64, ch)) - (char*) BASE64);
                if (pos > 64 && ch != '=' && ch != '\r' && ch != '\n' && ch != ' ') {
                    lua_pushnil(L);
                    lua_pushstring(L, "invalid base64 string");
                    return 2; // Return nil, [String] error
                } else if (pos < 64) {
                    stream |= pos;
                }
            }
        }
        luaL_addchar(&out, (stream>>16) & 0xFF);
        luaL_addchar(&out, (stream>>8) & 0xFF);
        luaL_addchar(&out, stream & 0xFF);
    }

    luaL_pushresult(&out);
    return 1; // Return [String] encoded
}



int luaopen_multisocket_base64(lua_State *L) {
    static const luaL_Reg lib_functions[] = {
            {"encode", base64_encode},
            {"decode", base64_decode},
            {NULL, NULL}
    };
    luaL_newlib(L, lib_functions);  // Create a new Table and put it on the Stack
    return 1;  // Return the last Item on the Stack
}




