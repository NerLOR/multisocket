
# Lua Multisocket
A library written in C to provide simple IPv4/TCP and IPv6/TCP sockets in Lua.
UDP is currently not supported.

## Compatibility
The library is designed to be used with **Lua 5.3**, 
but it could also work with older and newer versions.

#### Features:
* IPv4 TCP
* IPv6 TCP

#### Planned:
* SSL/TLS Encryption on TCP connections
* IPv4/IPv6 UDP support

## Documentation
The full documentation and reference can be found on the Wiki pate

## Examples
#### HTTP Client
Send a HTTP Request
```
local multisocket = require("multisocket")


local socket, err = multisocket.open("www.google.com", 80)
if not socket then
    print("Error: "..tostring(err))
    os.exit(1)
end

local success, err = socket:send("GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n")
if not success then
    print("Error: "..tostring(err))
    os.exit(2)
end

local fields = {}
local version, code, status
local linenum = 0
while true do
    linenum = linenum + 1
    local line, err, part = sock:receiveLine()
    if not line then
        print("Error: "..tostring(err))
        os.exit(3)
    elseif linenum == 1 then
        version, code, status = line:match("^HTTP/(%d%.%d) (%d+) (.+)$")
        print(code.." "..status)
    elseif line == "" then
        break
    else
        local field, value = line:match("^([^:]+): (.+)$")
        fields[field] = value
    end
end

local data = ""
local size = tonumber(fields["Content-Length"])
local chunked = fields["Transfer-Encoding"] == "chunked"
if chunked then
    while true do
        local hex = socket:receiveLine()
        local num = tonumber(hex, 16)
        if num == 0 and size and #data == size then
            break
        end
        local part, err = socket:receive(num)
        if not part then
            print("Error: "..tostring(err))
            os.exit(4)
        end
        socket:receiveLine()
        data = data..part
    end
else
    data = socket:receive(size)
end

print(data)


```

