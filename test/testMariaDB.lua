

local multisocket = require("multisocket")
local mariadb = require("mariadb")


local conn, err = mariadb.connect("stechauner.noip.me", 3306, "Test", "lorenz", "<PASSWORD>")

if not conn then
    print("Unable to connect to DB: "..tostring(err))
end

local succ, err = conn:query([==[SELECT CONCAT(Vorname, ' ', Nachname) AS Name, Filiale.Name AS Filiale FROM Mitarbeiter JOIN Filiale ON PK_filId = arbeitet;]==])
if not succ then
    print("Unable to query DB: "..tostring(err))
end

for a,b in ipairs(succ.cols) do
    io.write(string.format("%16s | ", b.columnAlias))
end
print()

for a,b in ipairs(succ) do
    for c,d in ipairs(succ.cols) do
        io.write(string.format("%16s | ", b[d.columnAlias]))
    end
    io.write("\n")
end


