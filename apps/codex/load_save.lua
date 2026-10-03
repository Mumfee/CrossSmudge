-- Codex: Ink & Iron — One-Shot Save Loader (Executed once at startup, freed immediately)
return function()
    local data = smudge.load("codex_save", "")
    if not data or #data < 10 then return false end

    local vals = {}
    for line in (data .. "\n"):gmatch("(.-)\r?\n") do
        local k, v = line:match("^(.-)=(.*)$")
        if k and v then vals[k] = v end
    end

    if not vals["floor"] then return false end

    state.inRun = true
    state.floor = tonumber(vals["floor"]) or 1
    state.maxFloor = tonumber(vals["maxFloor"]) or 15
    state.gold = tonumber(vals["gold"]) or 50
    state.playerHp = tonumber(vals["playerHp"]) or 50
    state.playerMaxHp = tonumber(vals["playerMaxHp"]) or 50
    state.playerWard = tonumber(vals["playerWard"]) or 0
    state.bonusWard = tonumber(vals["bonusWard"]) or 0
    state.permStrength = tonumber(vals["permStrength"]) or 0
    state.baseMaxInk = tonumber(vals["baseMaxInk"]) or 3
    state.ink = tonumber(vals["ink"]) or 3
    state.maxInk = tonumber(vals["maxInk"]) or 3
    state.lastPlayedMeter = vals["lastPlayedMeter"] or "None"
    state.eventId = tonumber(vals["eventId"]) or 0

    local function parseList(str)
        local t = {}
        if not str or str == "" then return t end
        for num in str:gmatch("%d+") do
            table.insert(t, tonumber(num))
        end
        return t
    end

    state.deck = parseList(vals["deck"])
    state.relics = parseList(vals["relics"])
    state.drawPile = parseList(vals["drawPile"])
    state.hand = parseList(vals["hand"])
    state.discardPile = parseList(vals["discardPile"])
    state.cardRewards = parseList(vals["cardRewards"])

    if vals["enemyName"] then
        state.enemy = {
            name = vals["enemyName"],
            hp = tonumber(vals["enemyHp"]) or 20,
            maxHp = tonumber(vals["enemyMaxHp"]) or 20,
            ward = tonumber(vals["enemyWard"]) or 0,
            spriteId = tonumber(vals["enemySpriteId"]) or 0,
            isElite = (vals["enemyIsElite"] == "1"),
            isBoss = (vals["enemyIsBoss"] == "1"),
            intent = vals["enemyIntent"] or "Attack",
            intentVal = tonumber(vals["enemyIntentVal"]) or 6,
            status = {strength=0, weak=0, vuln=0, poison=0}
        }
    end

    local ss = vals["savedScreen"] or "combat"
    if ss == "title" or ss == "deck_view" then ss = "combat" end
    state.savedScreen = ss
    return true
end
