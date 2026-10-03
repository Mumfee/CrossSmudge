local screen = {}

function screen.enter(arg)
    state.phase = "battle"
    if not state.enemy or (arg and arg.newCombat) then
        init_combat()
    end
end

function screen.draw()
    local m = smudge.get_metrics()
    local ty = m.top_padding + m.header_height + 2
    local by = h - m.button_hints_height - 2
    draw_page_frame(ty, by)

    local y = draw_combat_header(ty)

    local n = #state.hand
    if n > 0 and state.selectedHandIdx >= 1 and state.selectedHandIdx <= n then
        render_card_inspector(16, y, w - 32, 118, kCards[state.hand[state.selectedHandIdx] + 1], true)
        y = y + 124
    else
        y = y + 10
    end

    local hy = y + 2
    local ch = math.min(by - hy - 4, 220)
    if n > 0 then
        local cw = math.min(110, math.floor((w - 36) / n))
        local sx = math.floor((w - (cw * n)) / 2)
        for i = 1, n do
            local sel = (i == state.selectedHandIdx)
            draw_card_item(sx + (i - 1) * cw, sel and (hy - 8) or hy, cw - 4, ch, kCards[state.hand[i] + 1], sel)
        end
    else
        smudge.centered_text(hy + 60, "(Hand empty - press End Turn)", "ui12", "regular", true)
    end

    smudge.button_hints("End Turn", "Play", "<", ">")
end

function screen.on_button(btn, pressed)
    if not pressed then return end
    if btn == "up" or btn == "page_back" then
        set_screen("deck_view", {tab = 1})
        return
    elseif btn == "down" or btn == "page_forward" then
        set_screen("deck_view", {tab = 2})
        return
    end

    local n = #state.hand
    if btn == "back" then
        end_player_turn()
        smudge.request_update()
    elseif btn == "confirm" then
        play_card(state.selectedHandIdx)
        smudge.request_update()
    elseif btn == "left" and n > 0 then
        state.selectedHandIdx = (state.selectedHandIdx - 2 + n) % n + 1
        smudge.request_update()
    elseif btn == "right" and n > 0 then
        state.selectedHandIdx = (state.selectedHandIdx % n) + 1
        smudge.request_update()
    end
end

function screen.on_tap(x, y)
    local n = #state.hand
    if n == 0 then return end
    local m = smudge.get_metrics()
    local by = h - m.button_hints_height - 2
    local cw = math.min(110, math.floor((w - 36) / n))
    local sx = math.floor((w - (cw * n)) / 2)
    local hy = by - 224
    if y >= hy and y <= by then
        local idx = math.floor((x - sx) / cw) + 1
        if idx >= 1 and idx <= n then
            if state.selectedHandIdx == idx then
                play_card(idx)
            else
                state.selectedHandIdx = idx
            end
            smudge.request_update()
        end
    end
end

return screen
