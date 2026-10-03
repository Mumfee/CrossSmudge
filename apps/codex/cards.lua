-- Codex: Ink & Iron — Cards Streaming Catalog
local card_cache = setmetatable({}, { __mode = "v" })
local card_mt = {
    __index = function(t, k)
        if k == "id" then return t[1]
        elseif k == "name" then return t[2]
        elseif k == "cost" then return t[3]
        elseif k == "meter" then return t[4]
        elseif k == "type" then return t[5]
        elseif k == "baseVal" then return t[6]
        elseif k == "couplet" then return t[7]
        elseif k == "cBonus" then return t[8]
        elseif k == "desc" then return t[9]
        elseif k == "cDesc" then return t[10] end
    end
}

function get_card(id)
    if id == nil then return nil end
    local numId = tonumber(id)
    if not numId or numId < 0 or numId >= 30 then return nil end
    if card_cache[numId] then return card_cache[numId] end

    local card = nil
    if smudge and smudge.find_section then
        smudge.find_section("cards.txt", "[" .. tostring(numId) .. "]", "[", function(line)
            local name, cost, meter, typ, baseVal, couplet, cBonus, desc, cDesc =
                line:match("^([^|]+)|(%d+)|([^|]+)|([^|]+)|(%d+)|([^|]+)|(%d+)|([^|]*)|?(.*)$")
            if name then
                card = setmetatable({
                    numId, name, tonumber(cost) or 1,
                    meter, typ, tonumber(baseVal) or 0,
                    couplet, tonumber(cBonus) or 0,
                    desc, (cDesc and #cDesc > 0) and cDesc or nil
                }, card_mt)
            end
            return false
        end)
    end
    if card then card_cache[numId] = card end
    return card
end

kCards = setmetatable({}, {
    __index = function(_, k)
        local n = tonumber(k)
        if n then return get_card(n - 1) end
        return nil
    end,
    __len = function() return 30 end
})

return kCards
