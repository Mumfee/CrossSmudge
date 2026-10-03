local screen = {}
local text = ""

function screen.enter(arg)
    if arg and arg.choice then
        local c = arg.choice
        if c[3] ~= 0 then state.playerHp = math.max(0, math.min(state.playerMaxHp, state.playerHp + c[3])) end
        if c[4] ~= 0 then
            state.playerMaxHp = math.max(10, state.playerMaxHp + c[4])
            if state.playerHp > state.playerMaxHp then state.playerHp = state.playerMaxHp end
        end
        if c[5] ~= 0 then state.gold = math.max(0, state.gold + c[5]) end
        if c[6] ~= 0 then state.permStrength = state.permStrength + c[6] end
        if c[7] ~= 0 then state.baseMaxInk = math.min(6, state.baseMaxInk + c[7]) end
        if c[8] ~= 0 then state.bonusWard = state.bonusWard + c[8] end
        if c[9] ~= 0 then table.insert(state.deck, c[9]) end
        if c[10] ~= 0 then
            for r = 0, 11 do
                if not has_relic(r) then table.insert(state.relics, r) break end
            end
        end
        if c[11] ~= 0 then state.playerHp = state.playerMaxHp end
        text = c[12] or ""
    end
    state.savedScreen = "outcome"
    save_game_state()
end

function screen.draw()
    local m = smudge.get_metrics()
    smudge.header("Scriptorium", "Consequence")
    local ty = m.top_padding + m.header_height + 2
    draw_page_frame(ty, h - m.button_hints_height - 2)

    ty = draw_codex_stats(ty + 6) + 30
    draw_miniature_frame(22, ty, w - 44, 160)
    smudge.centered_text(ty + 16, "Consequence", "ui12", "bold", true)

    local ol = smudge.wrapped_text(text, w - 76, "small", "regular", 4)
    local oy = ty + 48
    for i = 1, #(ol and ol.lines or {}) do
        smudge.centered_text(oy, ol.lines[i], "small", "regular", true)
        oy = oy + smudge.line_height("small") + 4
    end

    local bx, by = math.floor((w - 200) / 2), ty + 190
    smudge.rect(bx, by, 200, 42, true)
    smudge.text(bx + 100, by + 13, "Continue", "small", "bold", "center", false)
    smudge.button_hints("Continue", "Continue", "", "")
end

function screen.on_button(btn, pressed)
    if not pressed then return end
    if state.playerHp <= 0 then
        state.inRun = false
        delete_save_state()
        set_screen("game_over")
    else
        advance_floor()
    end
end

function screen.on_tap(x, y)
    screen.on_button("confirm", true)
end

return screen
