

local multisocket = require("multisocket")

local TIMEOUT = 0
local BUFFER_SIZE = 4096

local http = {}

local CRLF = '\r\n'

local obj = {}
local res = {}
local req = {}

http.codes = {
    -- Informational
    [100] = {type = "Informational", name = "Continue"},
    [101] = {type = "Informational", name = "Switching Protocols"},
    [102] = {type = "Informational", name = "Processing"},

    -- Success
    [200] = {type = "Success",       name = "OK"},
    [201] = {type = "Success",       name = "Created"},
    [202] = {type = "Success",       name = "Accepted"},
    [203] = {type = "Success",       name = "Non-Authoritative Information"},
    [204] = {type = "Success",       name = "No Content"},
    [205] = {type = "Success",       name = "Reset Content"},
    [206] = {type = "Success",       name = "Partial Content"},
    [207] = {type = "Success",       name = "Multi-Status"},
    [208] = {type = "Success",       name = "Already Reported"},

    -- Redirection
    [300] = {type = "Redirection",   name = "Multiple Choices"},
    [301] = {type = "Redirection",   name = "Moved Permanently"},
    [302] = {type = "Redirection",   name = "Found"},
    [303] = {type = "Redirection",   name = "See Other"},
    [304] = {type = "Redirection",   name = "Not Modified"},
    [305] = {type = "Redirection",   name = "Use Proxy"},
    [306] = {type = "Redirection",   name = nil},
    [307] = {type = "Redirection",   name = "Temporary Redirect"},
    [308] = {type = "Redirection",   name = "Permanent Redirect"},

    -- Client Error
    [400] = {type = "Client Error",  name = "Bad Request",                     desc = "The request could not be understood by the server due to malformed syntax.", emoji = {"😒", "😟"}, comment = "Is it so difficult to send a VALID Request?!"},
    [401] = {type = "Client Error",  name = "Unauthorized",                    desc = "The request requires user authentication.", emoji = {"🤐", "🚧", "😉", "🔒", "🕵", "😁", "😠"}, comment = "HAHA!"},
    [402] = {type = "Client Error",  name = "Payment Required",                desc = "This code is reserved for future use.", emoji = {"🤑"}, comment = "I want your Money!"},
    [403] = {type = "Client Error",  name = "Forbidden",                       desc = "The server understood the request, but is refusing to fulfill it.", emoji = {"🤐", "🚧", "😉", "🔒", "🕵", "😁", "😠"}, comment = "HAHA!"},
    [404] = {type = "Client Error",  name = "Not Found",                       desc = "The server has not found anything matching the Request-URI.", emoji = {"😡", "😠", "😒"}, comment = "THIS. FILE. DOES. NOT. EXIST!"},
    [405] = {type = "Client Error",  name = "Method Not Allowed",              desc = "The method specified in the Request-Line is not allowed for the resource identified by the Request-URI."},
    [406] = {type = "Client Error",  name = "Not Acceptable",                  desc = "The resource identified by the request is only capable of generating response entities which have content characteristics not acceptable according to the accept headers sent in the request."},
    [407] = {type = "Client Error",  name = "Proxy Authentication Required",   desc = nil},
    [408] = {type = "Client Error",  name = "Request Time-out",                desc = "The connection timeout has run out."},
    [409] = {type = "Client Error",  name = "Conflict",                        desc = nil},
    [410] = {type = "Client Error",  name = "Gone",                            desc = nil},
    [411] = {type = "Client Error",  name = "Length Required",                 desc = nil},
    [412] = {type = "Client Error",  name = "Precondition Failed",             desc = nil},
    [413] = {type = "Client Error",  name = "Request Entity Too Large",        desc = "The server is refusing to process a request because the request entity is larger than the server is willing or able to process"},
    [414] = {type = "Client Error",  name = "Request-URL Too Long",            desc = nil},
    [415] = {type = "Client Error",  name = "Unsupported Media Type",          desc = nil},
    [416] = {type = "Client Error",  name = "Requested range not satisfiable", desc = nil},
    [417] = {type = "Client Error",  name = "Expectation Failed",              desc = nil},
    [418] = {type = "Client Error",  name = "I'm a teapot",                    desc = nil},
    [420] = {type = "Client Error",  name = "Policy Not Fulfilled",            desc = nil},
    [421] = {type = "Client Error",  name = "Misdirected Request",             desc = nil},
    [422] = {type = "Client Error",  name = "Unprocessable Entity",            desc = nil},
    [423] = {type = "Client Error",  name = "Locked",                          desc = nil},
    [424] = {type = "Client Error",  name = "Failed Dependency",               desc = nil},
    [425] = {type = "Client Error",  name = "Unordered Collection",            desc = nil},
    [426] = {type = "Client Error",  name = "Upgrade Required",                desc = nil},
    [428] = {type = "Client Error",  name = "Precondition Required",           desc = nil},
    [429] = {type = "Client Error",  name = "Too Many Requests",               desc = nil},
    [431] = {type = "Client Error",  name = "Request Header Fields Too Large", desc = nil},
    [451] = {type = "Client Error",  name = "Unavailable For Legal Reasons",   desc = nil},

    -- Server Error
    [500] = {type = "Server Error",  name = "Internal Server Error",           desc = "The server encountered an unexpected condition which prevented it from fulfilling the request.", comment = "Stop crashing this Server!", emoji = {"😐", "😑", "😒", "😓", "😞", "😟", "😠", "😥", "😦", "😧", "😬", "🙄", "🤔", "😴", "🦄"}},
    [501] = {type = "Server Error",  name = "Not Implemented",                 desc = "The server does not support the functionality required to fulfill the request."},
    [502] = {type = "Server Error",  name = "Bad Gateway",                     desc = "The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request."},
    [503] = {type = "Server Error",  name = "Service Unavailable",             desc = "The server is currently unable to handle the request due to a temporary overloading or maintenance of the server."},
    [504] = {type = "Server Error",  name = "Gateway Time-out",                desc = nil},
    [505] = {type = "Server Error",  name = "HTTP Version not supported",      desc = nil},
    [506] = {type = "Server Error",  name = "Variant Also Negotiates",         desc = nil},
    [507] = {type = "Server Error",  name = "Insufficient Storage",            desc = nil},
    [508] = {type = "Server Error",  name = "Loop Detected",                   desc = nil},
    [509] = {type = "Server Error",  name = "Bandwidth Limit Exceeded",        desc = nil},
    [510] = {type = "Server Error",  name = "Not Extended",                    desc = nil},
    [511] = {type = "Server Error",  name = "Network Authentication Required", desc = nil},
}


local function export(fields)
    local str = ""
    for index,data in pairs(fields) do
        str = str..tostring(index)..": "..tostring(data)..CRLF
    end
    return str
end

local function urlEncode(str)
    return str:gsub("[^%w]", function(chr)
        return string.format("%%%02X", string.byte(chr))
    end)
end

local function urlDecode(str)
    return str:gsub("%%(%x%x)", function(hex)
        return string.char(tonumber(hex, 16))
    end)
end


function obj:send(...)
    return self.socket:send(...)
end

function obj:receive(...)
    return self.socket:receive(...)
end

function obj:receiveLine(...)
    return self.socket:receiveLine(...)
end

function obj:close()
    return self.socket:close()
end


function obj:getStatusText()
    return self.res.statustext
end

function obj:getStatusCode()
    return self.res.statuscode
end

function obj:getStatus()
    return self.res.statuscode, self.res.statustext
end


function obj:getMethod()
    return self.req.method
end

function obj:getPath()
    return self.req.path
end

function obj:getRequest()
    return self.req.method, self.req.path
end


function req:request(method, path, body)

--- Send Request ---

    self.req.method = tostring(method)
    self.req.path = tostring(path)
    self.req.version = "1.1"

    local len = 0
    if type(body) == "number" then
        len = body
    elseif type(body) == "string" then
        len = #body
    elseif type(body) == "userdata" then
        len = body:seek("end",0)
        body:seek("set",0)
    elseif body ~= nil then
        body = tostring(body)
        len = #body
    end
    self:setField("Content-Length", len)
    local cookie = ""
    for ind,d in pairs(self.req.cookies) do
        cookie = cookie..tostring(ind).."="..tostring(urlEncode(tostring(d))).."; "
    end
    cookie = cookie:sub(1,-3)
    self:setField("Cookie", cookie)

    local str = self.req.method.." "..self.req.path.." HTTP/"..self.req.version..CRLF..export(self.req.fields)..CRLF
    local sent, err = self:send(str)
    if sent ~= #str then
        return nil, err
    end

    if type(body) == "string" then
        local sent, err = self:send(body)
        if not sent then
             return nil, err
        end
    elseif type(body) == "userdata" then
        while true do
            local sent, err = self:send(body:read(4096))
            if not sent then
                return nil, err
            elseif sent == 0 then
                break
            end
        end
    end


--- Receive Response ---

    local linenum = 0
    while true do
        linenum = linenum + 1
        local line, err = self:receiveLine()
        if not line then
            return nil, err
        elseif line == "" then
            break
        elseif linenum == 1 then
            local version, statuscode, statustext = line:match("^HTTP/(%d%.%d) (%d%d%d) (.+)$")
            if not statuscode or not statustext or not version then
                return nil, "invalid response"
            end
            self.res.statuscode = tonumber(statuscode)
            self.res.statustext = statustext
            self.res.version = version
        else
            local index,data = line:match("^%s*([^:]+)%s*:%s*(.-)%s*$")
            if not index or not data then
                return nil, "invalid response"
            end
            if index:lower() == "set-cookie" then
                local name, value, meta = data:match("^([^=]+)=([^;]*)(.*)$")
                self.res.cookies[name] = {value = urlDecode(value)}
                while true do
                    local ind,d
                    ind,d,meta = meta:match("^;%s*([^=]*)=([^;*])(.*)")
                    if not ind then
                        break
                    end
                    self.res.cookies[ind] = urlDecode(d)
                    if #meta == 0 then
                        break
                    end
                end
            else
                self.res.fields[index] = tonumber(data) or data
            end
        end
    end

    if self.res.fields.transferencoding == "chunked" then
        --self.res.body = io.tmpfile()
        self.res.body = ""
        while true do
            local len, err = self:receiveLine()
            if not len then
                return nil, err
            end
            len = tonumber(len,16)
            if not len or len == 0 then
                break
            end
            local buffer, err = self:receive(len)
            if not buffer then
                return nil, err
            end
            --self.res.body:write(buffer)
            self.res.body = self.res.body..buffer
            local succ, err = self:receive(2)
            if not succ then
                return nil, err
            elseif succ ~= CRLF then
                return nil, "invalid resposne"
            end
        end
    else
        local body, err = self.res.fields.contentlength ~= 0 and self:receive(self.res.fields.contentlength) or "", nil
        if not body then
            return nil, err
        end
        self.res.body = body
    end

    return self
end

function req:setField(index, data)
    self.req.fields[index] = data
end

function req:getField(index)
    return self.res.fields[index]
end

function req:setCookie(index, data)
    self.req.fields["Cookie"] = (self.req.fields["Cookie"] and self.req.fields["Cookie"].."; " or "")..index.."="..data
end

function req:getCookie(index)
    error("Not implemented")
end

function req:getCookies()
    error("Not implemented")
end


function res:accept()

    local linenum = 0
    while true do
        linenum = linenum + 1
        local line, err = self:receiveLine()
        if not line then
            return nil, err
        elseif line == "" then
            break
        elseif linenum == 1 then
            local method, path, version = line:match("^(%S+) (%S+) HTTP/(%d%.%d)$")
            if not method or not path or not version then
                return nil, "invalid response"
            end
            self.req.method = method
            self.req.path = path
            self.req.version = version

        else
            local index, data = line:match("^%s*([^:]+)%s*:%s*(.-)%s*$")
            if not index or not data then
                break
            end
            self.req.fields[index] = tonumber(data) or data
        end
    end

    local data, err = self.req.fields.contentlength and self.req.fields.contentlength ~= 0 and self:receive(self.req.fields.contentlength) or "", nil
    if not data then
        return nil, err
    end
    self.req.body = data

    return self
end

function res:respond(statuscode, body, statustext, length)
    local len
    local isFile = false
    local sent = 0
    local isPart = false
    local range = self.req.fields["Range"] or "" -- TODO FIX
    local startb, endb
    --print(range,range:match("^bytes="))
    if range:match("^bytes=") then
        startb, endb = range:match("^bytes=(%d+)-(%d+)?")
        startb = tonumber(startb)
        endb = tonumber(endb)
        --print(startb,endb)
        if startb then
            isPart = true
        end
    end
    if type(body) == "userdata" then
        self.res.fields["Transfer-Encoding"] = "chunked"
        isFile = true
        len = body:seek("end",0)
        self.res.fields["Content-Range"] = "bytes "..(startb or 0).."-"..(endb or len - 1).."/"..len
        body:seek("set",startb or 0)
        len = endb and (endb - startb + 1) or len
    elseif body then
        body = tostring(body)
        len = #body
    else
        body = ""
        len = 0
    end

    if type(length) == "number" then
        len = length
    elseif length ~= nil then
        len = nil
    end

    self.res.statuscode = statuscode == 200 and isPart and 206 or statuscode
    self.res.statustext = statustext or http.codes[statuscode].name
    self.res.version = "1.1"
    self.res.fields["Content-Length"] = len
    self.res.fields["Accept-Ranges"] = "bytes"

    local str = "HTTP/"..self.res.version.." "..self.res.statuscode.." "..self.res.statustext..CRLF..export(self.res.fields)..CRLF
    local s, err = self:send(str)
    if s ~= #str then
        return nil, err
    end

    if not isFile then
        local s, err = self:send(body)
        if s ~= #body then
            return nil, err
        end
        sent = #body
    else
        --self.socket:setTimeout(0)
        while true do
            local buffer = body:read(BUFFER_SIZE) -- body:read(sent+BUFFER_SIZE < len and BUFFER_SIZE or len-sent)
            if not buffer or #buffer == 0 then
                self:send("0\r\n\r\n")
                break
            end
            sent = sent + #buffer
            local success, err = self:send(string.format("%X\r\n%s\r\n", #buffer, buffer))
            if not success then
                return nil, err
            end
        end
    end

    return self
end

function res:error(statuscode, message, statustext, comment, emoji)
    self:setField("Content-Type", "text/html")
    local code = http.codes[statuscode]
    local title = statustext or statuscode.." "..code.name
    local emoji = emoji or code.emoji and code.emoji[ math.random(1, #code.emoji) ]
    local desc = "HTTP/1.1 "..code.type.." "..statuscode..": "..code.name.."<br>"..code.desc..(message and "<br><br>"..message or "")
    local comment = comment or code.comment
    local content = ""
    content = content..'<!DOCTYPE html><html>'
    content = content..'<head><meta charset="UTF-8"><title>'..title..'</title><style>'
    content = content..'html{font-family:"Times New Roman";}*{text-align:center;}h1{font-size:2em;}h2{font-size:1.5em}h3{font-size:8em;margin:0;}'
    content = content..'</style></head><body>'
    content = content..'<h1>'..title..'</h1>'
    content = content..(emoji and '<h3>'..emoji..'</h3>' or '')
    content = content..'<p>'..desc..'</p>'
    content = content..(comment and '<h2>'..comment..'</h2>' or '')
    content = content..'</body></html>'
    return self:respond(statuscode, content, statustext)
end

function res:redirect(path)
    self:setField("Location", tostring(path or "/"))
    self:respond(303)
end

function res:setField(index, data)
    self.res.fields[index] = data
end

function res:getField(index)
    return self.req.fields[index]
end

function res:setCookie(index, data)
    self:setField("Set-Cookie", index.."="..data)
end

function res:getCookie(index)
    return self:getCookies()[index]
end

function res:getCookies()
    local tbl = {}
    local cookies = self:getField("Cookie") or ""
    for index,data in cookies:gmatch("%s*([^=]+)%s*=%s*([^;]*)%s*;?") do
        tbl[index] = data
    end
    return tbl
end



local mtFields = {
    __index = function(self, key)
        for ikey,data in pairs(self) do
            if ikey == key or ikey:gsub("%W",""):lower() == key:lower() then
                return data
            end
        end
    end,
    __newindex = function(self, key, value)
        return rawset(self, key, value)
    end,
}

local mtReq = {
    __index = (function()
        local index = {}
        for name,func in pairs(obj) do
            index[name] = func
        end
        for name,func in pairs(req) do
            index[name] = func
        end
        return index
    end)(),
    __tostring = function(self)
        return self.req.method.." "..self.req.path
    end,
}

local mtRes = {
    __index = (function()
        local index = {}
        for name,func in pairs(obj) do
            index[name] = func
        end
        for name,func in pairs(res) do
            index[name] = func
        end
        return index
    end)(),
    __tostring = function(self)
        return self.res.statuscode.." "..self.res.statustext
    end,
}


function http.wrap(conn, params)
    local sock = {
        req = {
            method = nil,
            path = nil,
            version = nil,
            fields = setmetatable({}, mtFields),
            body = nil,
            cookies = {},
        },
        res = {
            statuscode = nil,
            statustext = nil,
            version = nil,
            fields = setmetatable({}, mtFields),
            body = nil,
            cookies = {},
        },
        socket = conn,
    }
    params = params or {}

    if conn:isServerSide() then
        sock = setmetatable(sock, mtRes)
        sock:setField("Server","Necronda/2.0.1")
        if params.fields then
            for index,data in pairs(params.fields) do
                sock:setField(index, data)
            end
        end
    elseif conn:isClientSide() then
        sock = setmetatable(sock, mtReq)
        sock:setField("Host", params.host or conn:getPeerAddress())
        sock:setField("User-Agent","Mozilla/5.0 Necronda/2.0.1")
        if params.fields then
            for index,data in pairs(params.fields) do
                sock:setField(index, data)
            end
        end
    else
        return nil, "unable to determine mode for connection"
    end

    return sock
end

function http.request(method, url, fields, body)
    local port
    local scheme, host, sport, path = url:match("^([^:]+)://([^:/]+):?(%d*)(.-)$")
    path = path == "" and "/" or path

    if scheme == "http" then
        port = sport ~= "" and tostring(sport) or 80
    elseif scheme == "https" then
        port = sport ~= "" and tostring(sport) or 443
    else
        return nil, "scheme not supported"
    end

    local sock, err = multisocket.open(host, (scheme == "https" and 443 or 80), (scheme == "https"))
    if not sock then
        return nil, err
    end
    sock:setTimeout(4)
    local sock, err = http.wrap(sock, {fields = fields, host = host})
    if not sock then
        return nil, err
    end

    local succ, err = sock:request(method, path, body)
    local peerport = sock.socket:getPeerPort()
    local sockport = sock.socket:getSocketPort()
    local peeraddr = sock.socket:getPeerAddress()
    local sockaddr = sock.socket:getSocketAddress()
    sock:close()
    sock.socket = nil
    sock.peerPort = peerport
    sock.socketPort = sockport
    sock.peerAddress = peeraddr
    sock.socketAddress = sockaddr

    if not succ then
        return nil, err, sock
    else
        return sock
    end
end


return http

