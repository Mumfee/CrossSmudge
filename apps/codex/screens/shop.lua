local screen = {}
local idx = 1

local function gen_shop()
    shop_state = { cards = {}, relics = {}, heal = false }
    while #shop_state.cards < 3 do
        shop_state.cards[#shop_state.cards + 1] = math.random(0, #kCards - 1)
    end
    for i = 0, 11 do
        if not has_relic(i) then shop_state.relics[#shop_state.relics + 1] = i end
        if #shop_state.relics >= 2 then break end
    end
    while #shop_state.relics < 2 do shop_state.relics[#shop_state.relics + 1] = 0 end
    if has_relic(11) then
        state.playerHp = math.min(state.playerMaxHp, state.playerHp + 8)
    end
    state.savedScreen = "shop"
    save_game_state()
end

function screen.enter(arg)
    if (arg and arg.newShop) or not shop_state.cards or #shop_state.cards == 0 then
        gen_shop()
    end
    idx = 1
    collectgarbage("collect")
end

function screen.exit()
    collectgarbage("collect")
end

function screen.draw()
    local m = smudge.get_metrics()
    smudge.header("Scriptorium", "Ch.1 Pg " .. state.floor .. "/15")
    local ty = m.top_padding + m.header_height + 2
    draw_page_frame(ty, h - m.button_hints_height - 2)

    ty = draw_codex_stats(ty + 6)
    draw_miniature_frame(330, ty + 10, 136, 136)
    smudge.draw_sprite(334, ty + 14, 128, 128, "sprites/merchant.raw")
    smudge.text(24, ty + 14, "Merchant's Wares:", "ui12", "bold", "left")

    local iy = ty + 44
    for i = 1, 7 do
        local sel = (i == idx)
        local it = ""
        if i <= 3 then
            local cId = shop_state.cards[i]
            local cName = cId < 0 and "SOLD" or kCards[cId + 1].name
            it = i .. ". [Verse] " .. cName .. " (" .. (35 + i * 10) .. " G)"
        elseif i <= 5 then
            if not kRelics then dofile("relics.lua") end
            local rId = shop_state.relics[i - 3]
            local rName = rId < 0 and "SOLD" or kRelics[rId + 1].name
            it = i .. ". [Relic] " .. rName .. " (" .. (95 + (i - 3) * 15) .. " G)"
        elseif i == 6 then
            it = "6. " .. (shop_state.heal and "Scribe's Draught [SOLD]" or "Scribe's Draught (+12 HP) [35 G]")
        else
            it = "7. Turn the Page -> (Depart)"
        end

        if sel then smudge.rect(20, iy - 2, 300, 26, true) end
        smudge.text(26, iy + 2, it, "small", sel and "bold" or "regular", "left", not sel)
        iy = iy + 30
    end
    smudge.button_hints("Leave", "Buy", "Prev", "Next")
end

function screen.on_button(btn, pressed)
    if not pressed then return end
    if btn == "back" then return advance_floor() end
    if btn == "confirm" then
        if idx == 7 then return advance_floor() end
        local p = (idx <= 3) and (35 + idx * 10) or ((idx <= 5) and (95 + (idx - 3) * 15) or 35)
        if state.gold >= p then
            if idx <= 3 then
                local cId = shop_state.cards[idx]
                if cId >= 0 then
                    state.gold = state.gold - p
                    state.deck[#state.deck + 1] = cId
                    shop_state.cards[idx] = -1
                    save_game_state()
                end
            elseif idx <= 5 then
                local rIdx = idx - 3
                local rId = shop_state.relics[rIdx]
                if rId >= 0 then
                    state.gold = state.gold - p
                    if not has_relic(rId) then state.relics[#state.relics + 1] = rId end
                    shop_state.relics[rIdx] = -1
                    save_game_state()
                end
            elseif not shop_state.heal then
                state.gold = state.gold - p
                state.playerHp = math.min(state.playerMaxHp, state.playerHp + 12)
                shop_state.heal = true
                save_game_state()
            end
        end
    elseif btn == "left" or btn == "up" then
        idx = (idx - 2 + 7) % 7 + 1
    elseif btn == "right" or btn == "down" then
        idx = (idx % 7) + 1
    elseif btn == "page_back" then
        set_screen("deck_view", {tab = 1})
    elseif btn == "page_forward" then
        set_screen("deck_view", {tab = 4})
    end
    smudge.request_update()
end

function screen.on_tap(x, y)
    local m = smudge.get_metrics()
    local iy = m.top_padding + m.header_height + 72
    for i = 1, 7 do
        if x >= 20 and x <= 320 and y >= iy - 4 and y <= iy + 26 then
            idx = i
            screen.on_button("confirm", true)
            return
        end
        iy = iy + 30
    end
end

return screen
