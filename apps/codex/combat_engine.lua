function draw_cards(n)
    for _ = 1, n do
        if #state.drawPile == 0 then
            if #state.discardPile == 0 then break end
            for _, c in ipairs(state.discardPile) do table.insert(state.drawPile, c) end
            state.discardPile = {}
            for j = #state.drawPile, 2, -1 do
                local r = math.random(j)
                state.drawPile[j], state.drawPile[r] = state.drawPile[r], state.drawPile[j]
            end
        end
        if #state.drawPile > 0 then
            table.insert(state.hand, table.remove(state.drawPile))
        end
    end
end

function spawn_enemy(n)
    local b = (n == 15)
    local u = (n == 8 or n == 11 or n == 14)
    local r = b and ((n % 2 == 0) and 11 or 6) or
                (u and ({4, 5, 9, 10})[(n % 4) + 1] or
                ({0, 1, 2, 3, 7, 8})[((n - 1) % 6) + 1])
    local e = nil
    if smudge and smudge.find_section then
        smudge.find_section("monsters.txt", "[" .. r .. "]", "[", function(line)
            local name, hp, s, intent, iv = line:match("^([^|]+)|(%d+)|(%d+)|([^|]+)|(%d+)")
            if name then
                local bVal = (not b and not u) and math.floor((n - 1) * 2.5) or 0
                hp = tonumber(hp) + bVal
                e = {
                    name = name, hp = hp, maxHp = hp, ward = 0,
                    spriteId = tonumber(s), intent = intent,
                    intentVal = tonumber(iv) + ((not b and not u) and math.floor(n / 3) or 0),
                    isElite = u, isBoss = b,
                    status = {strength = 0, weak = 0, vuln = 0}
                }
            end
            return false
        end)
    end
    return e
end

function init_combat()
    state.enemy = spawn_enemy(state.floor)

    state.maxInk = state.baseMaxInk + (has_relic(0) and 1 or 0)
    state.ink = state.maxInk
    state.playerWard = state.bonusWard + (has_relic(1) and 6 or 0)
    state.bonusWard = 0
    state.lastPlayedMeter = "None"
    state.playerStatus = {strength = state.permStrength or 0, weak = 0, vuln = 0, retainWard = false, extraDraw = 0, transcendence = false}

    state.drawPile = {}
    for _, c in ipairs(state.deck) do table.insert(state.drawPile, c) end
    for j = #state.drawPile, 2, -1 do
        local r = math.random(j)
        state.drawPile[j], state.drawPile[r] = state.drawPile[r], state.drawPile[j]
    end

    state.hand = {}
    state.discardPile = {}
    draw_cards(has_relic(3) and 5 or 4)

    state.selectedHandIdx = 1
    state.savedScreen = "combat"
    save_game_state()
end

function play_card(hIdx)
    local cid = state.hand[hIdx]
    if not cid then return end
    local card = kCards[cid + 1]
    if not card or state.ink < card.cost then return end

    state.ink = state.ink - card.cost
    local b = (coup_on and coup_on(card)) and card.cBonus or 0
    local ps = state.playerStatus

    if card.type == "Attack" then
        local d = card.baseVal + b + ps.strength + (has_relic(4) and (card.meter == "Blade" and 1 or 0) or 0)
        if ps.weak > 0 then d = math.max(1, math.floor(d * 0.75)) end
        if state.enemy.status and state.enemy.status.vuln > 0 then d = math.floor(d * 1.5) end

        if state.enemy.ward >= d then
            state.enemy.ward = state.enemy.ward - d
        else
            local u = d - state.enemy.ward
            state.enemy.ward = 0
            state.enemy.hp = math.max(0, state.enemy.hp - u)
        end
    elseif card.type == "Skill" then
        if card.baseVal > 0 then
            state.playerWard = state.playerWard + card.baseVal + b
        end
        if card.id == 2 then draw_cards(2)
        elseif card.id == 14 then draw_cards(3) end
    elseif card.type == "Power" then
        if card.id == 20 then ps.strength = ps.strength + 2
        elseif card.id == 21 then ps.retainWard = true
        elseif card.id == 22 then ps.extraDraw = ps.extraDraw + 1
        elseif card.id == 28 then ps.transcendence = true end
    end

    state.lastPlayedMeter = card.meter
    if card.type ~= "Power" then table.insert(state.discardPile, cid) end
    table.remove(state.hand, hIdx)
    if state.selectedHandIdx > #state.hand then state.selectedHandIdx = math.max(1, #state.hand) end

    if state.enemy.hp <= 0 then
        state.gold = state.gold + 18 + (has_relic(6) and 15 or 0)
        if has_relic(2) then state.playerHp = math.min(state.playerMaxHp, state.playerHp + 2) end
        if state.floor == 15 then
            state.inRun = false
            delete_save_state()
            set_screen("victory")
            return
        end
        state.cardRewards = {}
        while #state.cardRewards < 3 do table.insert(state.cardRewards, math.random(0, #kCards - 1)) end
        state.rewardIdx = 1
        state.phase = "reward"
        state.savedScreen = "combat"
        save_game_state()
        set_screen("reward")
        return
    end
end

function end_player_turn()
    for _, c in ipairs(state.hand) do table.insert(state.discardPile, c) end
    state.hand = {}

    local d = state.enemy.intentVal + ((state.enemy.status and state.enemy.status.strength) or 0)
    if state.enemy.status and state.enemy.status.weak > 0 then d = math.max(1, math.floor(d * 0.75)) end
    if state.playerStatus.vuln > 0 then d = math.floor(d * 1.5) end

    if state.enemy.intent == "Attack" or state.enemy.intent == "AttackAndDefend" then
        if state.playerWard >= d then
            state.playerWard = state.playerWard - d
            if has_relic(8) then state.enemy.hp = math.max(0, state.enemy.hp - 3) end
        else
            local u = d - state.playerWard
            state.playerWard = 0
            state.playerHp = math.max(0, state.playerHp - u)
        end
    end

    if state.enemy.intent == "Defend" or state.enemy.intent == "AttackAndDefend" then
        state.enemy.ward = state.enemy.ward + 8
    end

    if state.playerHp <= 0 then
        state.inRun = false
        delete_save_state()
        set_screen("game_over")
        return
    end

    state.ink = state.maxInk
    state.lastPlayedMeter = "None"
    if not state.playerStatus.retainWard and not has_relic(5) then
        state.playerWard = math.floor(state.playerWard / 2)
    end
    draw_cards(4 + state.playerStatus.extraDraw)
    state.selectedHandIdx = 1
    save_game_state()
end
