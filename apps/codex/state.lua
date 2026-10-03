-- Codex: Ink & Iron — State & Progression
state = {
    inRun = false,
    playerHp = 50,
    playerMaxHp = 50,
    gold = 50,
    floor = 1,
    maxFloor = 15,
    deck = {},
    relics = {},
    baseMaxInk = 3,
    permStrength = 0,
    bonusWard = 0,
    ink = 3,
    maxInk = 3,
    playerWard = 0,
    lastPlayedMeter = "None",
    playerStatus = {strength=0, weak=0, vuln=0, poison=0, retainWard=false, extraDraw=0, transcendence=false, scriptPlayed=0},
    enemy = nil,
    drawPile = {},
    hand = {},
    discardPile = {},
    cardRewards = {},
    selectedHandIdx = 1,
    phase = "battle",
    rewardIdx = 1,
    savedScreen = "combat",
    eventId = 0
}

shop_state = {cards={}, cardPrices={}, cardSold={}, relics={}, relicPrices={}, relicSold={}, healSold=false}

function has_relic(id)
    for _, r in ipairs(state.relics) do
        if r == id then return true end
    end
    return false
end
function save_game_state()
    if not state.inRun then
        smudge.save("codex_save", "")
        return
    end
    local s = "floor=" .. state.floor ..
        "\ngold=" .. state.gold ..
        "\nhp=" .. state.playerHp ..
        "\nmaxHp=" .. state.playerMaxHp ..
        "\nward=" .. state.playerWard ..
        "\nink=" .. state.ink ..
        "\ndeck=" .. table.concat(state.deck, ",") ..
        "\nrelics=" .. table.concat(state.relics, ",") ..
        "\nhand=" .. table.concat(state.hand, ",") ..
        "\ndraw=" .. table.concat(state.drawPile, ",") ..
        "\ndisc=" .. table.concat(state.discardPile, ",") ..
        "\nsavedScreen=" .. (state.savedScreen or active_screen_name or "combat") ..
        "\neventId=" .. (state.eventId or 0)
    if state.enemy then
        local e = state.enemy
        s = s .. "\nenemyName=" .. e.name ..
            "\nenemyHp=" .. e.hp ..
            "\nenemyMaxHp=" .. e.maxHp ..
            "\nenemyWard=" .. e.ward ..
            "\nenemySpriteId=" .. e.spriteId ..
            "\nenemyIsElite=" .. (e.isElite and "1" or "0") ..
            "\nenemyIsBoss=" .. (e.isBoss and "1" or "0") ..
            "\nenemyIntent=" .. e.intent ..
            "\nenemyIntentVal=" .. e.intentVal
    end
    smudge.save("codex_save", s)
end

function delete_save_state()
    smudge.save("codex_save", "")
end

function start_new_run()
    state.playerHp = 50
    state.playerMaxHp = 50
    state.gold = 50
    state.floor = 1
    state.maxFloor = 15
    state.inRun = true
    state.bonusWard = 0
    state.permStrength = 0
    state.baseMaxInk = 3
    state.relics = {0} -- Silver Quill
    state.deck = {0, 0, 0, 0, 1, 1, 1, 1, 2, 3}
    state.enemy = nil
    state.phase = "battle"
    state.rewardIdx = 1
    state.savedScreen = "combat"
    set_screen("combat", {newCombat = true})
end

function advance_floor()
    state.cardRewards = {}
    state.hand = {}
    state.drawPile = {}
    state.discardPile = {}
    state.enemy = nil
    state.phase = "battle"
    state.rewardIdx = 1
    collectgarbage("collect")

    state.floor = state.floor + 1
    if state.floor > state.maxFloor then
        state.inRun = false
        delete_save_state()
        set_screen("victory")
        return
    end

    if state.floor == 5 or state.floor == 10 then
        state.savedScreen = "shop"
        set_screen("shop", {newShop = true})
    elseif state.floor == 3 or state.floor == 7 or state.floor == 9 or state.floor == 13 then
        state.savedScreen = "scriptorium"
        set_screen("scriptorium", {newEvent = true})
    else
        state.savedScreen = "combat"
        set_screen("combat", {newCombat = true})
    end
end
