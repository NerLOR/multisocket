

local multisocket = require("multisocket")
local mariadb = require("mariadb")


local conn, err = mariadb.connect("192.168.1.89", 3306, "Necronda", "lorenz", "PferdeFenster86")

if not conn then
    print("Unable to connect to DB: "..tostring(err))
end

--for i = 1,2 do
    local succ, err = conn:execute("INSERT INTO Session (Cookie) VALUES (UUID());")--conn:execute("SELECT SessionID, Cookie, User, loggedInSince FROM Session WHERE Cookie = '';")
    --local succ, err = conn:execute("SELECT SessionID, Cookie, User, loggedInSince FROM Session WHERE Cookie = '';")
    if not succ then
        print("Unable to query DB: "..tostring(err))
    end

    for a,b in ipairs(succ) do
        io.write(string.format("%48s | ", b.columnAlias))
    end
    print()

    for a,b in conn:fetch() do
        for c,d in ipairs(succ) do
            io.write(string.format("%48s | ", b[d.columnAlias]))
        end
        io.write("\n")
    end
--end


