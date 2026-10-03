
local screen = {}
local tab = 1
local page = 0
local menuIdx = 1
local retScreen = 'title'

local TABS = {'Deck', 'Disc', 'Relics', 'Menu'}
local OPTS = {'Resume', 'Save & Title', 'Exit', 'Abandon'}

function screen.enter(arg)
    if arg then
        tab = arg.tab or tab
        page = arg.page or 0
        retScreen = arg.return_screen or state.savedScreen or retScreen
    else
        retScreen = state.savedScreen or retScreen
    end
    menuIdx = 1
end

local function draw_menu(y, by)
    for i = 1, 4 do
        local iy = y + 10 + (i - 1) * 54
        local sel = (i == menuIdx)
        smudge.rect(30, iy, w - 60, 42, sel)
        smudge.text(240, iy + 12, OPTS[i], 'ui12', sel and 'bold' or 'regular', 'center', not sel)
    end
    smudge.centered_text(by - 26, 'Auto-saved', 'small', 'regular', true)
    smudge.button_hints('Back', 'Confirm', 'Up', 'Down')
end

local function draw_list(y, by)
    local list = (tab == 1) and state.deck or ((tab == 2) and state.discardPile or state.relics)
    local total = #list
    local maxP = math.max(0, math.floor((total - 1) / 8))
    if page > maxP then page = maxP end

    if total == 0 then
        smudge.centered_text(y + 40, '(Empty)', 'ui12', 'regular', true)
    else
        for i = page * 8 + 1, math.min(total, page * 8 + 8) do
            local id = list[i]
            if tab <= 2 then
                local c = kCards[id + 1]
                smudge.text(20, y, c.cost .. ' Ink ' .. c.name, 'small', 'bold', 'left', true)
                smudge.text(w - 20, y, c.meter, 'small', 'bold', 'right', true)
                smudge.text(20, y + 16, c.desc, 'small', 'regular', 'left', true)
                smudge.line(20, y + 32, w - 20, y + 32)
                y = y + 36
            else
                if not kRelics then dofile('relics.lua') end
                local r = kRelics[id + 1]
                smudge.rect(20, y + 4, 56, 56, false)
                if id >= 0 and id < 12 then
                    smudge.draw_sprite(24, y + 8, 48, 48, 'sprites/relic_' .. id .. '.raw')
                end
                smudge.text(84, y + 10, r.name, 'small', 'bold', 'left', true)
                smudge.text(84, y + 32, r.desc, 'small', 'regular', 'left', true)
                y = y + 70
                smudge.line(20, y - 2, w - 20, y - 2)
            end
        end
        if maxP > 0 then
            smudge.centered_text(by - 22, (page + 1) .. '/' .. (maxP + 1), 'small', 'bold', true)
        end
    end
    smudge.button_hints('Back', 'Tab', '<', '>')
end

function screen.draw()
    local m = smudge.get_metrics()
    local ty = m.top_padding + m.header_height + 2
    local by = h - m.button_hints_height - 2
    draw_page_frame(ty, by)
    smudge.header(TABS[tab])

    for i = 1, 4 do
        local tx = 24 + (i - 1) * 108
        local sel = (i == tab)
        smudge.rect(tx, ty + 8, 104, 28, sel)
        smudge.text(tx + 54, ty + 14, TABS[i], 'small', sel and 'bold' or 'regular', 'center', not sel)
    end

    draw_codex_divider(ty + 44, w, 24)
    local y = ty + 54

    if tab == 4 then
        draw_menu(y, by)
    else
        draw_list(y, by)
    end
end

function screen.on_button(btn, pressed)
    if not pressed then return end
    if btn == 'back' then set_screen(retScreen) return end

    if btn == 'up' or btn == 'page_back' then
        tab = (tab - 2 + 4) % 4 + 1
        page, menuIdx = 0, 1
    elseif btn == 'down' or btn == 'page_forward' or (tab <= 3 and btn == 'confirm') then
        tab = (tab % 4) + 1
        page, menuIdx = 0, 1
    elseif tab == 4 then
        if btn == 'confirm' then
            if menuIdx == 1 then return set_screen(retScreen) end
            if menuIdx < 4 then save_game_state() else delete_save_state() state.inRun = false end
            if menuIdx == 3 then smudge.exit() else set_screen('title') end
            return
        end
        menuIdx = (btn == 'left') and ((menuIdx - 2 + 4) % 4 + 1) or ((menuIdx % 4) + 1)
    else
        local list = (tab == 1) and state.deck or ((tab == 2) and state.discardPile or state.relics)
        local maxP = math.floor((#list - 1) / 8)
        if btn == 'left' and page > 0 then page = page - 1
        elseif btn == 'right' and page < maxP then page = page + 1 end
    end
    smudge.request_update()
end

function screen.on_tap(x, y)
    if y < 100 then
        screen.on_button('down', true)
    elseif tab == 4 and y > 120 and y < 350 then
        menuIdx = math.floor((y - 120) / 54) + 1
        screen.on_button('confirm', true)
    end
end

return screen
