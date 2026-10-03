-- Codex: Ink & Iron — Title Screen

local screen = {}
local title_idx = 1

function screen.enter()
    title_idx = 1
end

function screen.draw()
    smudge.header("Codex", "Ink & Iron")
    local m = smudge.get_metrics()
    local topY = m.top_padding + m.header_height + 2
    local bottomY = h - m.button_hints_height - 2
    draw_page_frame(topY, bottomY)

    local y = topY + 12
    smudge.centered_text(y, "CODEX: INK & IRON", "ui12", "bold", true)
    y = y + smudge.line_height("ui12") + 4
    smudge.centered_text(y, "A Roguelike Deckbuilder for E-Paper", "small", "regular", true)
    y = y + smudge.line_height("small") + 12

    local plateSize = 136
    local plateX = math.floor((w - plateSize) / 2)
    draw_monster(plateX, y, 6)
    y = y + plateSize + 16

    draw_codex_divider(y, w, 28)
    y = y + 16

    local btnW = 340
    local btnH = 38
    local btnX = math.floor((w - btnW) / 2)

    local options = {}
    if state.inRun then
        options = {
            string.format("Continue Delve (Page %d/15)", state.floor),
            "New Delve",
            "Inspect Grimoire",
            "Abandon Delve"
        }
    else
        options = {
            "New Delve",
            "Inspect Grimoire",
            "Exit to Applications"
        }
    end

    for i, opt in ipairs(options) do
        local optY = y + (i - 1) * (btnH + 10)
        local isSel = (i == title_idx)
        if isSel then
            smudge.rect(btnX, optY, btnW, btnH, true)
            smudge.text(btnX + math.floor(btnW / 2), optY + math.floor((btnH - smudge.line_height("ui12")) / 2), opt, "ui12", "bold", "center", false)
        else
            smudge.rect(btnX, optY, btnW, btnH, false)
            smudge.text(btnX + math.floor(btnW / 2), optY + math.floor((btnH - smudge.line_height("ui12")) / 2), opt, "ui12", "regular", "center", true)
        end
    end

    smudge.button_hints("Back", "Select", "Up", "Down")
end

function screen.on_button(btn, pressed)
    if not pressed then return end
    local totalOptions = state.inRun and 4 or 3
    if btn == "back" then
        smudge.exit()
    elseif btn == "up" or btn == "left" or btn == "page_back" then
        title_idx = (title_idx - 2 + totalOptions) % totalOptions + 1
        smudge.request_update()
    elseif btn == "down" or btn == "right" or btn == "page_forward" then
        title_idx = (title_idx % totalOptions) + 1
        smudge.request_update()
    elseif btn == "confirm" then
        if state.inRun then
            if title_idx == 1 then
                -- Continue Delve
                set_screen(state.savedScreen or "combat")
            elseif title_idx == 2 then
                -- New Delve
                start_new_run()
            elseif title_idx == 3 then
                -- Inspect Grimoire
                set_screen("deck_view", {tab = 1, page = 0, return_screen = "title"})
            elseif title_idx == 4 then
                -- Abandon Delve
                delete_save_state()
                state.inRun = false
                title_idx = 1
                smudge.request_update()
            end
        else
            if title_idx == 1 then
                start_new_run()
            elseif title_idx == 2 then
                set_screen("deck_view", {tab = 1, page = 0, return_screen = "title"})
            elseif title_idx == 3 then
                smudge.exit()
            end
        end
    end
end

function screen.on_tap(x, y)
    local m = smudge.get_metrics()
    local topY = m.top_padding + m.header_height + 2
    local menuStartY = topY + 12 + smudge.line_height("ui12") + 4 + smudge.line_height("small") + 12 + 136 + 16 + 16
    local btnW = 340
    local btnH = 38
    local btnX = math.floor((w - btnW) / 2)
    local totalOptions = state.inRun and 4 or 3

    for i = 1, totalOptions do
        local optY = menuStartY + (i - 1) * (btnH + 10)
        if x >= btnX and x <= btnX + btnW and y >= optY and y <= optY + btnH then
            title_idx = i
            screen.on_button("confirm", true)
            return
        end
    end
end

return screen
