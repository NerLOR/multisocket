#!/usr/bin/lua5.3

package.cpath = package.cpath..";/home/lorenz/Documents/Projects/lua/?/?.so"
package.path = package.path..";/home/lorenz/Documents/Projects/lua/?.lua"

local http = require("multisocket.http")

local smtp = require("multisocket.smtp")


smtp.send("smtp.gmail.com", "lorenz.stechauner@gmail.com", "melvsyopkfpinmkc", {
    from = "Necronda Server <lorenz.stechauner@gmail.com>",
    to = {
        "hermann".." <".."lorenz.stechauner@htl.rennweg.at"..">",
    },
    subject = "Password Reset",
    type = "text/html",
    body = [[<a href="https://www.necronda.net/account/reset-password?token=]].."asdf"..[[">Token</a>]]
})

--[[


local req, err = http.request("GET", "https://www.htl.rennweg.at")
if not req then
    print("Error: "..tostring(err))
else
    for a,b in pairs(req) do
        print(a,b)
    end
    print(req.res.body)
end


local req, err = http.request("GET", "https://ipv6.google.com")
if not req then
    print("Error: "..tostring(err))
else
    for a,b in pairs(req) do
        print(a,b)
    end
    print(req.res.body)
end

]]


