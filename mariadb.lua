local multisocket = require("multisocket")

local mariadb = {}

local MAX_PACKET_SIZE = 0xFFFF

-- LITTLE ENDIAN! (0002 = 0200)--


local function SHA1(data)
    local d = data:gsub(".", function(ch)
        return string.format("\\x%02X", ch:byte())
    end)
    local p = io.popen('/bin/echo -ne "'..d..'" | openssl sha1')
    local hash = p:read("*all")
    hash = hash:sub(10,-2)
    hash = hash:gsub("%x%x", function(ch)
        return string.char(tonumber(ch, 16))
    end)
    p:close()
    return hash
end

local function XOR(s1, s2)
    local s = ""
    for i = 1,#s1 do
        s = s..string.char(s1:sub(i,i):byte() ~ s2:sub(i,i):byte())
    end
    return s
end

local function str2num(str, len)
    local num = 0
    for i = 1, len or #str do
        num = num | (str:sub(i, i):byte() << ((i - 1) * 8))
        local a = 0
    end
    return num
end

local function num2str(num, len)
    local str = ""
    for i = 1, len do
        str = str .. string.char(num & 0xFF)
        num = num >> 8
    end
    if num ~= 0 then
        return nil, str
    end
    return str
end

local function printX(str)
    print(str:gsub(".", function(ch)
        if ch == "\\" then
            return "\\\\"
        elseif ch == "\0" then
            return "\\0"
        elseif ch:match("[%w%p_ ]") then
            return ch
        else
            return string.format("\\x%02X", ch:byte())
        end
    end ))
end


local function _r_int(stream, len)
    if len == nil then
        local ch = stream:sub(1, 1):byte()
        stream = stream:sub(2, -1)
        if ch < 0xFB then
            return stream, ch
        end
        if ch == 0xFB then
            -- null
            return stream, nil
        elseif ch == 0xFC then
            -- 2 Bytes
            len = 2
        elseif ch == 0xFD then
            -- 3 Bytes
            len = 3
        elseif ch == 0xFE then
            -- 8 Bytes
            len = 8
        else
            return stream, nil
        end
    end
    if type(len) == "number" then
        local s = str2num(stream:sub(1, len), len)
        stream = stream:sub(len + 1, -1)
        return stream, s
    else
        return stream, nil
    end
end

local function _r_string(stream, len)
    if len == "EOF" then
        return "", stream
    elseif len == "NUL" then
        local s
        s, stream = stream:match("^(.-)\0(.*)$")
        return stream, s
    elseif len == "LENENC" then
        stream, len = _r_int(stream)
    end
    if type(len) == "number" then
        return stream:sub(len + 1, -1), stream:sub(1, len)
    else
        return stream, nil
    end
end

local function _s_int(stream, int, len)
    if len == nil then
        if int == nil then
            return stream .. "\xFB"
        elseif int < 0xFB then
            return stream .. num2str(int, 1)
        elseif int < 0xFFFF then
            return stream .. "\xFC" .. num2str(int, 2)
        elseif int < 0xFFFFFF then
            return stream .. "\xFD" .. num2str(int, 3)
        else
            return stream .. "\xFE" .. num2str(int, 8)
        end
    else
        return stream .. num2str(int, len)
    end
end

local function _s_string(stream, str, t)
    if type(str) == "number" then
        str = string.rep("\0", str)
    end
    if t == nil then
        return stream .. str
    elseif t == "NUL" then
        return stream .. str .. "\0"
    elseif t == "LENENC" then
        return _s_int(stream, #str) .. str
    end
end


local flags = {
    CLIENT_MYSQL = 0x00000001,
    CLIENT_FOUND_ROWS = 0x00000002,
    CLIENT_LONG_FLAG = 0x00000004,
    CLIENT_CONNECT_WITH_DB = 0x00000008,
    CLIENT_NO_SCHEMA = 0x00000010,
    CLIENT_COMPRESS = 0x00000020,
    CLIENT_ODBC = 0x00000040,
    CLIENT_LOCAL_FILES = 0x00000080,
    CLIENT_IGNORE_SPACE = 0x00000100,
    CLIENT_PROTOCOL_41 = 0x00000200,
    CLIENT_INTERACTIVE = 0x00000400,
    CLIENT_SSL = 0x00000800,
    CLIENT_TRANSACTIONS = 0x00001000,
    CLIENT_SECURE_CONNECTION = 0x00002000,
    CLIENT_MULTI_STATEMENTS = 0x00010000,
    CLIENT_MULTI_RESULTS = 0x00020000,
    CLIENT_PS_MULTI_RESULTS = 0x00040000,
    CLIENT_PLUGIN_AUTH = 0x00080000,
    CLIENT_CONNECT_ARTTRS = 0x00100000,
    CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA = 0x00200000,
    CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS = 0x00400000,
    CLIENT_SESSION_TRACK = 0x00800000,
    CLIENT_DEPRECATE_EOF = 0x01000000,
    CLIENT_SSL_VERIFY_SERVER_CERT = 0x40000000,
    MARIADB_CLIENT_PROGRESS = 0x100000000,
    MARIADB_CLIENT_COM_MULTI = 0x200000000,
    MARIADB_CLIENT_STMT_BLUK_OPERATIONS = 0x400000000,
}

local fieldTypes = {
    [000] = "decimal",
    [001] = "tiny",
    [002] = "short",
    [003] = "long",
    [004] = "float",
    [005] = "double",
    [006] = "null",
    [007] = "timestamp",
    [008] = "longlong",
    [009] = "int24",
    [010] = "date",
    [011] = "time",
    [012] = "datetime",
    [013] = "year",
    [014] = "newdate",
    [015] = "varchar",
    [016] = "bit",
    [017] = "timestamp2",
    [018] = "datetime2",
    [019] = "time2",
    [246] = "newdecimal",
    [247] = "enum",
    [248] = "set",
    [249] = "tiny_blob",
    [250] = "medium_blob",
    [251] = "long_blob",
    [252] = "blob",
    [253] = "var_string",
    [254] = "string",
    [255] = "geometry"
}

local fieldDetailFlags = {
    NOT_NULL = 1,
    PRIMARY_KEY = 2,
    UNIQUE_KEY = 4,
    MULTIPLE_KEY = 8,
    BLOB = 16,
    UNSIGNED = 32,
    DECIMAL = 64,
    BINARY_COLLATION = 128,
    ENUM = 256,
    AUTO_INCREMENT = 512,
    TIMESTAMP = 1024,
    SET = 2048
}

local connection = {}


function connection:receivePacket()
    local data = ""
    repeat
        local d, err = self.socket:receive(4)
        if not d then
            return nil, err
        end
        local len = str2num(d:sub(1, 3))
        local seq = str2num(d:sub(4, 4))
        local d, err = self.socket:receive(len)
        if not d then
            return nil, err
        end
        data = data .. d
        if seq ~= self.seq then
            return nil, data
        end
        self.seq = self.seq + 1
    until len ~= 0xFFFFFF
    return data
end

function connection:sendPacket(data)
    while true do
        local len = math.min(#data, 0xFFFFF)
        local succ, err = self.socket:send(num2str(len, 3) .. num2str(self.seq, 1) .. data:sub(1, len))
        if not succ then
            return nil, err
        end
        data = data:sub(len + 1, -1)
        self.seq = self.seq + 1
        if data == "" then
            break
        end
    end
    return true
end

function connection:parsePacket(str)
    local packet = {}
    local stream, err
    if str then
        stream = str
    else
        stream, err = self:receivePacket()
    end
    local len = #stream
    if not stream then
        return nil, err
    end
    stream, packet.header = _r_int(stream, 1)
    if packet.header == 0x00 or (packet.header == 0xFE and self.flags.CLIENT_DEPRECATE_EOF) then
        packet.type = "OK"
        stream, packet.affectedRows = _r_int(stream, "LENENC")
        stream, packet.lastInsertId = _r_int(stream, "LENENC")
        stream, packet.status = _r_int(stream, 2)
        stream, packet.warnings = _r_int(stream, 2)
        stream, packet.body = _r_string(stream, "EOF")
    elseif packet.header == 0xFE and len == 5 then
        packet.type = "EOF"
        stream, packet.warnings = _r_int(stream, 2)
        stream, packet.body = _r_int(stream, 2)
    elseif packet.header == 0xFE then
        packet.type = "OK"
        stream, packet.body = _r_string(stream, "EOF")
    elseif packet.header == 0xFF then
        packet.type = "ERR"
        stream, packet.code = _r_int(stream, 2)
        local message
        stream, message = _r_string(stream, "EOF")
        if message:sub(1,1) == "#" then
            packet.sqlState = message:sub(2,6)
            message = message:sub(7,-1)
        end
        packet.body = message
        packet.message = (packet.sqlState and "[#"..packet.sqlState.."] " or "")..packet.body
    else
        packet.body = stream
    end
    return packet
end

function connection:handshake(capabilities, collation, username, password, dbname)
    local stream, err = self:receivePacket()
    if not stream then
        return nil, err
    end

    local capa1, capa2, capa3, capa
    local scrambleLen, pluginDataLen

    stream, self.protocolVersion = _r_int(stream, 1)
    stream, self.serverVersion = _r_string(stream, "NUL")
    stream, self.connectionId = _r_int(stream, 4)
    stream, self.authSeed = _r_string(stream, 8)
    stream = _r_string(stream, 1) -- 1 Reserved Byte
    stream, capa1 = _r_int(stream, 2)
    stream, self.collation = _r_int(stream, 1)
    stream, self.statusFlags = _r_int(stream, 2)
    stream, capa2 = _r_int(stream, 2)
    capa = capa1 | (capa2 << 16)
    stream, scrambleLen = _r_int(stream, 1)
    stream, pluginDataLen = _r_int(stream, 1)
    stream = _r_string(stream, 6) -- filler



    if ((capa & flags.CLIENT_MYSQL) ~= 0) then
        stream = _r_string(stream, 3) -- filler
    else
        stream, capa3 = _r_int(stream, 3)
        capa = capa | (capa3 << 32)
    end

    for name, value in pairs(flags) do
        self.flags[name] = ((capa & value) ~= 0)
    end

    if self.flags.CLIENT_SECURE_CONNECTION then
        local len = math.max(12, pluginDataLen - 9)
        local scramble
        stream, scramble = _r_string(stream, len)
        self.authSeed = self.authSeed..scramble
        stream = _r_string(stream, 1) -- 1 Reserved Byte
    end

    if self.flags.CLIENT_PLUGIN_AUTH then
        stream, self.authPlugin = _r_string(stream, "NUL")
    end

    local auth = XOR(SHA1(password), SHA1(self.authSeed..SHA1(SHA1(password))))
    local authPlugin = "mysql_native_password"
    local attr = {}
    local clientCapabilities = 0

    for index, active in pairs(capabilities) do
        if active then
            clientCapabilities = clientCapabilities | flags[index]
        end
    end

    local stream = ""
    stream = _s_int(stream, clientCapabilities & 0xFFFFFFFF, 4)
    stream = _s_int(stream, MAX_PACKET_SIZE, 4)
    stream = _s_int(stream, collation, 1)
    stream = _s_string(stream, 19)
    if not self.flags.CLIENT_MYSQL then
        stream = _s_int(stream, clientCapabilities >> 32, 4)
    else
        stream = _s_string(stream, 4)
    end
    stream = _s_string(stream, username, "NUL")
    if false and self.flags.CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA then
        stream = _s_string(stream, auth, "LENENC")
    elseif false and self.flags.CLIENT_SECURE_CONNECTION then
        stream = _s_int(stream, #auth, 1)
        stream = _s_string(stream, auth)
    else
        stream = _s_int(stream, 0, 1)
    end
    if self.flags.CLIENT_CONNECT_WITH_DB then
        stream = _s_string(stream, dbname, "NUL")
    end
    if self.flags.CLIENT_PLUGIN_AUTH then
        stream = _s_string(stream, authPlugin, "NUL")
    end
    if self.flags.CLIENT_CONNECT_ATTRS then
        local attrLen = 0
        for a, b in pairs(attr) do
            attrLen = attrLen + 1
        end
        stream = _s_int(stream, attrLen)
        for index, value in pairs(attr) do
            stream = _s_string(stream, tostring(index), "LENENC")
            stream = _s_string(stream, tostring(value), "LENENC")
        end
    end

    local succ, err = self:sendPacket(stream)
    if not succ then
        return nil, err
    end


    local response, err = self:parsePacket()
    if not response then
        return nil, err
    end

    if response.header == 0xFE then
        local authPlugin2, authSeed2
        response.body, authPlugin2 = _r_string(response.body, "NUL")
        response.body, authSeed2 = _r_string(response.body, "EOF")
        authSeed2 = authSeed2:sub(1,20)
        if authPlugin2 == "mysql_native_password" then
            local succ, err = self:sendPacket(auth)
            if not succ then
                return nil, err
            end
        end
    end


    local response, err = self:parsePacket()
    if not response then
        return nil, err
    end

    self.seq = 0

    if response.type == "ERR" then
        return nil, response.message
    elseif response.type == "OK" then
        return true
    end

    return nil, "Unexpected server response"
end

function connection:execute(sql)
    local stream = ""
    stream = _s_int(stream, 0x03, 1)
    stream = _s_string(stream, sql)
    local succ, err = self:sendPacket(stream)
    if not succ then
        return nil, err
    end

    local stream, err = self:receivePacket()
    if not stream then
        return nil, err
    elseif stream:sub(1,1) == "\xFF" then
        local packet = self:parsePacket(stream)
        return nil, packet.message
    end

    local data = {}
    local columns = {}

    local columnCount
    stream, columnCount = _r_int(stream)
    for i = 1,columnCount do
        local col = {}
        local stream, err = self:receivePacket()
        if not stream then
            return nil, err
        end
        local catalog, schema, tableAlias, table, columnAlias, column, fixedFields, charset, maxColSize, fieldType, detail, decimals
        stream, catalog = _r_string(stream, "LENENC")
        stream, schema = _r_string(stream, "LENENC")
        stream, tableAlias = _r_string(stream, "LENENC")
        stream, table = _r_string(stream, "LENENC")
        stream, columnAlias = _r_string(stream, "LENENC")
        stream, column = _r_string(stream, "LENENC")
        stream, fixedFields = _r_int(stream)
        stream, charset = _r_int(stream, 2)
        stream, maxColSize = _r_int(stream, 4)
        stream, fieldType = _r_int(stream, 1)
        stream, detail = _r_int(stream, 1)
        stream, decimals = _r_int(stream, 1)

        if catalog ~= "def" then
            return nil, "Unexpected server response"
        end

        col.schema = schema
        col.tableAlias = tableAlias
        col.table = table
        col.columnAlias = columnAlias
        col.column = column
        col.type = fieldTypes[fieldType]
        col.charset = charset
        col.flags = {}
        for name,value in pairs(fieldDetailFlags) do
            col.flags[name] = (detail & value) ~= 0
        end
        col.decimals = decimals

        columns[i] = col
    end

    local response, err = self:parsePacket()
    if not response then
        return nil, err
    elseif response.type == "ERR" then
        return nil, (response.sqlState and "[#"..response.sqlState.."] " or "")..response.body
    end

    data.cols = columns

    while true do
        local stream, err = self:receivePacket()
        if stream:sub(1,1) == "\xFE" then
            break
        end
        local row = {}
        for i = 1,columnCount do
            local col = columns[i]
            local d
            stream, d = _r_string(stream, "LENENC")
            row[col.columnAlias] = d
        end
        table.insert(data, row)
    end

    return data
end


local mtConn = {
    __index = connection
}



function mariadb.connect(address, port, schema, username, password)
    local socket, err = multisocket.open(address, port, false)
    if not socket then
        return nil, err
    end

    socket:setTimeout(10)
    local conn = setmetatable({
        socket = socket,
        flags = {},
        seq = 0,
    }, mtConn)

    local params = {
        CLIENT_SECURE_CONNECTION = true,
        CLIENT_PLUGIN_AUTH = true,
        CLIENT_PROTOCOL_41 = true,
        CLIENT_DEPRECATE_EOF = true,
        CLIENT_CONNECT_WITH_DB = true
    }

    local succ, err = conn:handshake(params, 33, username, password, schema)
    if not succ then
        return nil, err
    end

    return conn
end



return mariadb


