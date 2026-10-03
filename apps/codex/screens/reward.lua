local screen = {}

function screen.enter()
    state.rewardIdx = 1
end

function screen.draw()
    local m = smudge.get_metrics()
    local ty = m.top_padding + m.header_height + 2
    local by = h - m.button_hints_height - 2
    draw_page_frame(ty, by)

    smudge.header("Codex Delve", "Victory")
    local y = ty + 8
    local idx = 18 + (has_relic(6) and 15 or 0)
    smudge.centered_text(y, "Victory +" .. idx .. " Gold", "ui12", "bold", true)
    y = y + smudge.line_height("ui12") + 6
    draw_codex_divider(y, w, 24)
    y = y + 10

    local n = #(state.cardRewards or {})
    local sel = state.rewardIdx or 1
    if sel > n and n > 0 then sel = 1 state.rewardIdx = 1 end
    if sel <= n and n > 0 then
        render_card_inspector(16, y, w - 32, 118, kCards[state.cardRewards[sel] + 1], false)
    end
    y = y + 132

    local cw = math.floor((w - 48) / 3)
    local ch = 220
    local sx = math.floor((w - (cw * n)) / 2)
    for i = 1, n do
        draw_card_item(sx + (i - 1) * cw, (sel == i) and (y - 6) or y, cw - 4, ch, kCards[state.cardRewards[i] + 1], (sel == i))
    end

    smudge.button_hints("Skip", "Take", "<", ">")
end

function screen.on_button(btn, pressed)
    if not pressed then return end
    if btn == "up" or btn == "page_back" then
        set_screen("deck_view", {tab = 1, page = 0, return_screen = "reward"})
        return
    elseif btn == "down" or btn == "page_forward" then
        set_screen("deck_view", {tab = 4, page = 0, return_screen = "reward"})
        return
    end

    local n = #(state.cardRewards or {})
    if btn == "back" then
        advance_floor()
    elseif btn == "confirm" then
        local sel = state.rewardIdx or 1
        if sel <= n and n > 0 then
            table.insert(state.deck, state.cardRewards[sel])
        end
        advance_floor()
    elseif btn == "left" and n > 0 then
        state.rewardIdx = ((state.rewardIdx or 1) - 2 + n) % n + 1
        smudge.request_update()
    elseif btn == "right" and n > 0 then
        state.rewardIdx = ((state.rewardIdx or 1) % n) + 1
        smudge.request_update()
    end
end

function screen.on_tap(x, y)
    local m = smudge.get_metrics()
    local n = #(state.cardRewards or {})
    local cw = math.floor((w - 48) / 3)
    local sx = math.floor((w - (cw * n)) / 2)
    local hy = m.top_padding + m.header_height + 176
    for i = 1, n do
        if x >= sx + (i - 1) * cw and x <= sx + i * cw and y >= hy and y <= hy + 220 then
            if (state.rewardIdx or 1) == i then
                table.insert(state.deck, state.cardRewards[i])
                advance_floor()
            else
                state.rewardIdx = i
                smudge.request_update()
            end
            return
        end
    end
end

return screen
