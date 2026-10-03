-- Codex: Ink & Iron — Relics Streaming Catalog
local relic_cache = setmetatable({}, { __mode = "v" })

function get_relic(id)
    if id == nil then return nil end
    local numId = tonumber(id)
    if not numId or numId < 0 or numId >= 12 then return nil end
    if relic_cache[numId] then return relic_cache[numId] end

    local relic = nil
    if smudge and smudge.find_section then
        smudge.find_section("relics.txt", "[" .. tostring(numId) .. "]", "[", function(line)
            local name, desc = line:match("^([^|]+)|(.*)$")
            if name then
                relic = { id = numId, name = name, desc = desc }
            end
            return false
        end)
    end
    if relic then relic_cache[numId] = relic end
    return relic
end

kRelics = setmetatable({}, {
    __index = function(_, k)
        local n = tonumber(k)
        if n then return get_relic(n - 1) end
        return nil
    end,
    __len = function() return 12 end
})

return kRelics
