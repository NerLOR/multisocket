

local multisocket = require("multisocket")
local base64 = require("multisocket.base64")

local smtp = {}

local function printT(t)
    for a,b in pairs(t) do
        print(a,b)
    end
end

local mtConnection = {
    __index = {
        send = function(self, str)
            return self.socket:send(str)
        end,
        receive = function(self)
            local respond = {}
            while true do
                local line, err = self.socket:receiveLine()
                if not line then
                    return nil, err
                end
                local code,ch,str = line:match("^(%d%d%d)([- ])(.*)$")
                if not code then
                    return nil
                end
                respond.code = respond.code or tonumber(code)
                table.insert(respond, str)
                if ch == " " then
                    break
                end
            end
            return respond
        end,
        command = function(self, str)
            local succ, err = self:send(str.."\r\n")
            if not succ then
                return nil, err
            end
            return self:receive()
        end,
        greet = function(self, str)
            local res, err = self:command("EHLO "..tostring(str))
            if not res then
                return nil, err
            elseif res.code ~= 250 then
                return nil, res.code, res[1]
            end
            return true
        end,
        encrypt = function(self)
            local res, err = self:command("STARTTLS")
            if not res then
                return nil, err
            elseif res.code ~= 220 then
                return nil, res.code, res[1]
            end

            return self.socket:encrypt()
        end,
        auth = function(self, username, password)
            local res, err = self:command("AUTH LOGIN")
            if not res then
                return nil, err
            elseif res.code ~= 334 then
                return nil, res.code, res[1]
            end
            local index = base64.decode(res[1])
            if index ~= "Username:" then
                return nil, "invalid server response"
            end

            local res, err = self:command(base64.encode(tostring(username)))
            if not res then
                return nil, err
            elseif res.code == 535 then
                return nil, "invalid username"
            elseif res.code ~= 334 then
                return nil, res.code, res[1]
            end
            local index = base64.decode(res[1])
            if index ~= "Password:" then
                return nil, "invalid server response"
            end

            local res, err = self:command(base64.encode(tostring(password)))
            if not res then
                return nil, err
            elseif res.code == 535 then
                return nil, "invalid password"
            elseif res.code ~= 235 then
                return nil, res.code, res[1]
            end
            return true
        end,
        quit = function(self)
            local res, err = self:command("QUIT")
            if not res then
                return nil, err
            elseif res.code ~= 221 then
                return nil, res.code, res[1]
            end
            return self.socket:close()
        end,
        mail = function(self)

        end,
    },
}


function smtp.newConnection(server, port)
    local conn, err = multisocket.open(server, port or 25, false)
    if not conn then
        return nil, err
    end
    conn = setmetatable({
        socket = conn,
    }, mtConnection)
    return conn, conn:receive()
end



function smtp.newMail(from, to, cc, bcc, subject)

end

function smtp.send(server, username, password, mail, port)
    local conn, err = smtp.newConnection(server, port)
    if not conn then
        print("Error: "..tostring(err))
        return nil
    end
    local success, err1, err2
    success, err1, err2 = conn:greet()
    if not success then
        return nil, err
    end
    success, err1, err2 = conn:encrypt()
    if not success then
        return nil, err1, err2
    end
    success, err1, err2 = conn:greet()
    if not success then
        return nil, err1, err2
    end
    success, err1, err2 = conn:auth(username, password)
    if not success then
        return nil, err1, err2
    end

    local res, err

    res, err = conn:command("MAIL FROM:"..(mail.from:match("<.->") or mail.from))
    if not res then
        return nil, err
    elseif res.code ~= 250 then
        return nil, res.code, res[1]
    end

    for i,name in ipairs(mail.to or {}) do
        res, err = conn:command("RCPT TO:"..(name:match("<.->") or name))
        if not res then
            return nil, err
        elseif res.code ~= 250 then
            return nil, res.code, res[1]
        end
    end
    for i,name in ipairs(mail.cc or {}) do
        res, err = conn:command("RCPT TO:"..(name:match("<.->") or name))
        if not res then
            return nil, err
        elseif res.code ~= 250 then
            return nil, res.code, res[1]
        end
    end
    for i,name in ipairs(mail.bcc or {}) do
        res, err = conn:command("RCPT TO:"..(name:match("<.->") or name))
        if not res then
            return nil, err
        elseif res.code ~= 250 then
            return nil, res.code, res[1]
        end
    end

    res, err = conn:command("DATA")
    if not res then
        return nil, err
    elseif res.code ~= 354 then
        return nil, res.code, res[1]
    end

    local msg = "From: "..mail.from.."\r\n"
    for i,name in ipairs(mail.to or {}) do
        msg = msg.."To: "..name.."\r\n"
    end
    for i,name in ipairs(mail.cc or {}) do
        msg = msg.."Cc: "..name.."\r\n"
    end
    --for i,name in ipairs(mail.bcc or {}) do
    --    msg = msg.."Bcc: "..name.."\r\n"
    --end
    msg = msg.."Subject: "..(mail.subject or "No Subject" ).."\r\n"
    msg = msg.."Content-Transfer-Encoding: base64\r\n"
    msg = msg.."Content-Type: "..(mail.type or "test/plain").."\r\n"
    msg = msg.."\r\n"..base64.encode(mail.body or "").."\r\n.\r\n"

    res, err = conn:command(msg)
    if not res then
        return nil, err
    elseif res.code ~= 250 then
        return nil, res.code, res[1]
    end

    return conn:quit()
end



return smtp
