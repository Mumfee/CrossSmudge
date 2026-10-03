local screen = {}
local evId = 0
local choiceIdx = 1
local ev = nil

local function get_event(id)
    id = id or 0
    local res = { id = id, narrative = {}, choices = {} }
    if smudge and smudge.find_section then
        local n = 0
        smudge.find_section("events.txt", "[" .. tostring(id) .. "]", "[", function(line)
            n = n + 1
            if n == 1 then
                local t, s = line:match("^([^|]+)|(.*)$")
                res.title, res.sub = t or "Scriptorium", s or "Sanctuary"
            elseif n == 2 then
                for p in line:gmatch("[^|]+") do res.narrative[#res.narrative + 1] = p end
            else
                local c = {}
                for p in line:gmatch("[^|]+") do c[#c + 1] = tonumber(p) or p end
                res.choices[#res.choices + 1] = c
            end
            return true
        end)
    end
    return res
end

function screen.enter(arg)
    if arg and arg.newEvent then state.eventId = math.random(0, 8) end
    evId = state.eventId or 0
    ev = get_event(evId)
    choiceIdx = 1
    state.savedScreen = "scriptorium"
    save_game_state()
    collectgarbage("collect")
end

function screen.exit()
    ev = nil
    collectgarbage("collect")
end

function screen.draw()
    local m = smudge.get_metrics()
    local cur = ev or get_event(evId)
    smudge.header(cur.title, cur.sub)

    local ty = m.top_padding + m.header_height + 2
    draw_page_frame(ty, h - m.button_hints_height - 2)

    ty = draw_codex_stats(ty + 6)
    draw_miniature_frame(172, ty, 136, 136)
    smudge.draw_sprite(176, ty + 4, 128, 128, "sprites/event_" .. cur.id .. ".raw")
    ty = ty + 144

    for i = 1, #cur.narrative do
        smudge.centered_text(ty, cur.narrative[i], "small", "regular", true)
        ty = ty + smudge.line_height("small") + 2
    end
    ty = ty + 4
    draw_codex_divider(ty, w, 28)
    ty = ty + 10

    screen.optY = ty
    for i = 1, #cur.choices do
        local c = cur.choices[i]
        local sel = (i == choiceIdx)
        local canAfford = not (c[5] < 0 and state.gold < -c[5])
        local iy = ty + ((i - 1) * 82)
        smudge.rect(22, iy, w - 44, 74, sel)
        smudge.text(36, iy + 8, c[1], "ui12", "bold", "left", not sel)
        smudge.text(36, iy + 36, canAfford and c[2] or ("[Requires " .. (-c[5]) .. " Gold]"), "small", "regular", "left", not sel)
    end
    smudge.button_hints("", "Select", "<", ">")
end

function screen.on_button(btn, pressed)
    if not pressed then return end
    if btn == "up" or btn == "page_back" then
        set_screen("deck_view", {tab = 1, return_screen = "scriptorium"})
        return
    elseif btn == "down" or btn == "page_forward" then
        set_screen("deck_view", {tab = 4, return_screen = "scriptorium"})
        return
    end

    local cur = ev or get_event(evId)
    if btn == "confirm" then
        local c = cur.choices[choiceIdx]
        if c[5] < 0 and state.gold < -c[5] then return end
        set_screen("outcome", { choice = c })
    elseif btn == "left" then
        choiceIdx = choiceIdx > 1 and (choiceIdx - 1) or #cur.choices
        smudge.request_update()
    elseif btn == "right" then
        choiceIdx = choiceIdx < #cur.choices and (choiceIdx + 1) or 1
        smudge.request_update()
    end
end

function screen.on_tap(x, y)
    if not screen.optY then return end
    for i = 1, #ev.choices do
        local iy = screen.optY + (i - 1) * 82
        if x >= 22 and x <= w - 22 and y >= iy and y <= iy + 74 then
            choiceIdx = i
            screen.on_button("confirm", true)
            return
        end
    end
end

return screen
