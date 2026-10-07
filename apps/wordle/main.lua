-- Wordle game for CrossSmudge (Lua)
-- Faithful reproduction of WordleActivity.h with on-screen keyboard, reset modal, auto-advance & dithering

local TOTAL_WORDS = 3103
local can_seek = nil
local cached_words_bin = nil

local function pick_secret_word()
    if can_seek == nil then
        if smudge and smudge.read_file then
            local test_0 = smudge.read_file("words.bin", 5, 0)
            local test_5 = smudge.read_file("words.bin", 5, 5)
            if test_0 and test_5 and #test_0 == 5 and #test_5 == 5 and test_0 ~= test_5 then
                can_seek = true
            else
                can_seek = false
            end
        else
            can_seek = false
        end
    end

    local idx = math.random(0, TOTAL_WORDS - 1)

    if can_seek then
        local w = smudge.read_file("words.bin", 5, idx * 5)
        if w and #w == 5 then
            return w:upper()
        end
    end

    -- Fallback: load full words.bin if seeking is unsupported on older firmware
    if not cached_words_bin and smudge and smudge.read_file then
        cached_words_bin = smudge.read_file("words.bin")
    end

    if cached_words_bin and #cached_words_bin >= 5 then
        local total = math.floor(#cached_words_bin / 5)
        if total > 0 then
            local fallback_idx = math.random(0, total - 1)
            local pos = fallback_idx * 5 + 1
            local w = cached_words_bin:sub(pos, pos + 4)
            if #w == 5 then
                return w:upper()
            end
        end
    end

    return "CRANE"
end

local secret = "CRANE"
local guesses = {} -- array of {guess = "WORDS", check = {2, 0, 1, 0, 2}}
local current_guess = ""
local in_game = true
local game_won = false
local kb_index = 0 -- 0..29 (29 = reset)
local show_reset_confirm = false
local absent_keys = {}
local w, h = 480, 800

local k_keys = {
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "<-",
    "Z", "X", "C", "V", "B", "N", "M", "ok", "clear"
}

local function evaluate_guess(guess_str)
    local check = {0, 0, 0, 0, 0}
    local secret_counts = {}
    for i = 1, 5 do
        local c = secret:sub(i, i)
        secret_counts[c] = (secret_counts[c] or 0) + 1
    end

    -- First pass: exact matches (2)
    for i = 1, 5 do
        local g = guess_str:sub(i, i)
        if g == secret:sub(i, i) then
            check[i] = 2
            secret_counts[g] = secret_counts[g] - 1
        end
    end

    -- Second pass: present matches (1) and absent (0)
    for i = 1, 5 do
        if check[i] == 0 then
            local g = guess_str:sub(i, i)
            if secret_counts[g] and secret_counts[g] > 0 then
                check[i] = 1
                secret_counts[g] = secret_counts[g] - 1
            else
                check[i] = 0
                if not secret:find(g, 1, true) then
                    absent_keys[g] = true
                end
            end
        end
    end

    return check
end

local function save_game_state()
    if not smudge or not smudge.save then return end
    if not in_game then
        smudge.save("wordle_state", "")
        return
    end

    local guess_list = {}
    for _, g in ipairs(guesses) do
        table.insert(guess_list, g.guess)
    end
    local guesses_str = table.concat(guess_list, ",")
    local data = string.format("%s;%s;%s;%d", secret, guesses_str, current_guess, kb_index)
    smudge.save("wordle_state", data)
end

local function load_game_state()
    if not smudge or not smudge.load then return false end
    local data = smudge.load("wordle_state")
    if not data or data == "" then return false end

    local parts = {}
    for part in string.gmatch(data .. ";", "([^;]*);") do
        table.insert(parts, part)
    end

    if #parts < 4 then return false end

    local s_secret = parts[1]
    local s_guesses = parts[2]
    local s_cur = parts[3]
    local s_kb = tonumber(parts[4]) or 0

    if #s_secret ~= 5 or not s_secret:match("^[A-Z]+$") then
        return false
    end

    secret = s_secret
    guesses = {}
    absent_keys = {}
    in_game = true
    game_won = false

    if #s_guesses > 0 then
        for g in string.gmatch(s_guesses .. ",", "([^,]+),") do
            if #g == 5 and g:match("^[A-Z]+$") then
                local check = evaluate_guess(g)
                table.insert(guesses, {guess = g, check = check})
                if g == secret then
                    game_won = true
                    in_game = false
                elseif #guesses >= 6 then
                    in_game = false
                end
            end
        end
    end

    if not in_game then
        if smudge and smudge.save then
            smudge.save("wordle_state", "")
        end
        return false
    end

    if #s_cur <= 5 and (s_cur == "" or s_cur:match("^[A-Z]+$")) then
        current_guess = s_cur
    else
        current_guess = ""
    end

    if s_kb >= 0 and s_kb <= 29 then
        kb_index = s_kb
    else
        kb_index = 0
    end

    show_reset_confirm = false
    if smudge and smudge.log then
        smudge.log(string.format("Wordle: restored game with %d guesses, secret %s", #guesses, secret))
    end
    return true
end

local function new_game()
    secret = pick_secret_word()
    if #secret ~= 5 then secret = "CRANE" end
    if smudge and smudge.log then
        smudge.log("Wordle secret: " .. secret)
    end
    guesses = {}
    current_guess = ""
    in_game = true
    game_won = false
    kb_index = 0
    show_reset_confirm = false
    absent_keys = {}
    save_game_state()
end

function on_init()
    w, h = smudge.get_bounds()
    math.randomseed(os.time())
    if not load_game_state() then
        new_game()
    end
    if smudge and smudge.get_memory and smudge.log then
        local mem = smudge.get_memory()
        smudge.log(string.format("Wordle Lua memory: %d KB / %d KB", mem.lua_kb or 0, mem.lua_max_kb or 0))
    end
end

function on_exit()
    save_game_state()
end

local function submit_guess()
    if #current_guess ~= 5 then return end
    local check = evaluate_guess(current_guess)
    table.insert(guesses, {guess = current_guess, check = check})

    if current_guess == secret then
        game_won = true
        in_game = false
    elseif #guesses >= 6 then
        in_game = false
    end
    current_guess = ""
    save_game_state()
end

local function handle_key_action(idx)
    if not in_game then
        new_game()
        return
    end

    if idx == 29 then
        show_reset_confirm = true
        return
    end

    if idx < 10 then -- Row 0
        if #current_guess < 5 then
            current_guess = current_guess .. k_keys[idx + 1]
            if #current_guess == 5 then
                kb_index = 27 -- snap to 'ok'
            end
        end
    elseif idx < 19 then -- Row 1 letters
        if #current_guess < 5 then
            current_guess = current_guess .. k_keys[idx + 1]
            if #current_guess == 5 then
                kb_index = 27 -- snap to 'ok'
            end
        end
    elseif idx == 19 then -- Backspace (<-)
        if #current_guess > 0 then
            current_guess = current_guess:sub(1, #current_guess - 1)
        end
    elseif idx < 27 then -- Row 2 letters
        if #current_guess < 5 then
            current_guess = current_guess .. k_keys[idx + 1]
            if #current_guess == 5 then
                kb_index = 27 -- snap to 'ok'
            end
        end
    elseif idx == 27 then -- ok / submit
        if #current_guess == 5 then
            submit_guess()
        end
    elseif idx == 28 then -- clear
        current_guess = ""
    end
end

function on_draw()
    smudge.clear()

    local m = smudge.get_metrics()
    local subtitle = game_won and "WINNER!" or (not in_game and "GAME OVER" or "")
    smudge.header("Wordle", subtitle)

    local availableWidth = w - 24
    local tilePadding = 6
    local tileSize = math.floor((availableWidth - (tilePadding * 4)) / 5)
    if tileSize > 54 then tileSize = 54 end

    local gridWidth = (tileSize * 5) + (tilePadding * 4)
    local startX = math.floor((w - gridWidth) / 2)
    local gridTopY = m.top_padding + m.header_height + 8
    local fontYOffset = 18

    -- 1. Render Grid
    for row = 0, 5 do
        local rowY = gridTopY + row * (tileSize + tilePadding)

        for col = 0, 4 do
            local tileX = startX + col * (tileSize + tilePadding)

            if row < #guesses then
                local g = guesses[row + 1]
                local charStr = g.guess:sub(col + 1, col + 1)
                local checkResult = g.check[col + 1]
                local textX = tileX + math.floor(tileSize / 2)
                local textY = rowY + math.floor(tileSize / 2) - fontYOffset

                if checkResult == 2 then -- Correct: Solid Black
                    smudge.rect(tileX, rowY, tileSize, tileSize, true)
                    smudge.text(textX, textY, charStr, 12, true, "center", false)
                elseif checkResult == 1 then -- Present: Dithered
                    smudge.rect_dither(tileX, rowY, tileSize, tileSize, false)
                    smudge.rect(tileX, rowY, tileSize, tileSize, false)
                    smudge.text(textX, textY, charStr, 12, true, "center", true)
                else -- Absent: Outline only
                    smudge.rect(tileX, rowY, tileSize, tileSize, false)
                    smudge.text(textX, textY, charStr, 12, true, "center", true)
                end
            elseif row == #guesses and in_game then
                smudge.rect(tileX, rowY, tileSize, tileSize, false)
                if col == #current_guess and #current_guess < 5 then
                    smudge.rect(tileX + 20, rowY + 20, tileSize - 40, tileSize - 40, true)
                end
                if col < #current_guess then
                    local charStr = current_guess:sub(col + 1, col + 1)
                    local textX = tileX + math.floor(tileSize / 2)
                    local textY = rowY + math.floor(tileSize / 2) - fontYOffset
                    smudge.text(textX, textY, charStr, 12, true, "center", true)
                end
            else
                smudge.rect(tileX, rowY, tileSize, tileSize, false)
            end
        end
    end

    -- 2. Render On-Screen Keyboard
    local kbTopY = gridTopY + (6 * (tileSize + tilePadding)) + 12
    local keyPad = 3
    local keyW = math.floor((availableWidth - (9 * keyPad)) / 10)
    local keyH = keyW
    local kbRowWidth = (10 * keyW) + (9 * keyPad)
    local kbStartX = math.floor((w - kbRowWidth) / 2)
    local okClearTotalW = 3 * keyW + 2 * keyPad
    local okW = math.floor((okClearTotalW - keyPad) / 2)
    local clearW = (okClearTotalW - keyPad) - okW
    local okX = kbStartX + 7 * (keyW + keyPad)
    local clearX = okX + okW + keyPad

    for i = 0, 28 do
        local r = 0
        local c = 0
        local kX = 0
        local keyWD = keyW

        if i < 10 then
            r = 0
            c = i
            kX = kbStartX + (c * (keyW + keyPad))
        elseif i < 20 then
            r = 1
            c = i - 10
            kX = kbStartX + (c * (keyW + keyPad))
        else
            r = 2
            local colIdx = i - 20
            if colIdx < 7 then
                c = colIdx
                kX = kbStartX + (c * (keyW + keyPad))
            elseif colIdx == 7 then -- ok key
                kX = okX
                keyWD = okW
            elseif colIdx == 8 then -- clear key
                kX = clearX
                keyWD = clearW
            end
        end

        local kY = kbTopY + (r * (keyH + keyPad))
        local kLabel = k_keys[i + 1]

        local isSelected = (i == kb_index and in_game and not show_reset_confirm)
        local isAbsent = absent_keys[kLabel]

        if isSelected then
            smudge.rect(kX, kY, keyWD, keyH, true)
            smudge.text(kX + math.floor(keyWD / 2), kY + math.floor(keyH / 2) - 12, kLabel, 10, true, "center", false)
        elseif isAbsent then
            smudge.rect_dither(kX, kY, keyWD, keyH, false)
            smudge.rect(kX, kY, keyWD, keyH, false)
            smudge.text(kX + math.floor(keyWD / 2), kY + math.floor(keyH / 2) - 12, kLabel, 10, false, "center", true)
        else
            smudge.rect(kX, kY, keyWD, keyH, false)
            smudge.text(kX + math.floor(keyWD / 2), kY + math.floor(keyH / 2) - 12, kLabel, 10, true, "center", true)
        end
    end

    -- Render Row 3: reset Button (matching width of clear button)
    local resetW = keyW * 2 + keyPad
    local resetX = math.floor((w - resetW) / 2)
    local resetY = kbTopY + 3 * (keyH + keyPad)
    local isResetSelected = (kb_index == 29 and in_game and not show_reset_confirm)

    if isResetSelected then
        smudge.rect(resetX, resetY, resetW, keyH, true)
        smudge.text(resetX + math.floor(resetW / 2), resetY + math.floor(keyH / 2) - 12, "reset", 10, true, "center", false)
    else
        smudge.rect(resetX, resetY, resetW, keyH, false)
        smudge.text(resetX + math.floor(resetW / 2), resetY + math.floor(keyH / 2) - 12, "reset", 10, true, "center", true)
    end

    -- Status message when game over
    if not in_game then
        local statusY = resetY + keyH + 6
        if game_won then
            smudge.centered_text(statusY, "YOU WON!", 12, true, true)
        else
            smudge.centered_text(statusY, "WORD WAS: " .. secret, 12, true, true)
        end
    end

    -- Modal Confirmation Dialog
    if show_reset_confirm then
        local modalW = w - 60
        local modalH = 60
        local modalX = math.floor((w - modalW) / 2)
        local modalY = math.floor((h - modalH) / 2) - 20

        smudge.rect(modalX, modalY, modalW, modalH, true)
        smudge.rect(modalX + 2, modalY + 2, modalW - 4, modalH - 4, false)
        smudge.centered_text(modalY + math.floor(modalH / 2) - fontYOffset + 4, "RESET GAME?", 12, true, false)
    end

    -- 3. Footer Hints
    if show_reset_confirm then
        smudge.button_hints("Cancel", "Confirm", "", "")
    elseif in_game then
        smudge.button_hints("Back", "Select", "Up", "Down")
    else
        smudge.button_hints("Back", "Play", "", "")
    end
end

function on_button(btn, pressed)
    if not pressed then return end

    if show_reset_confirm then
        if btn == "back" then
            show_reset_confirm = false
        elseif btn == "confirm" then
            show_reset_confirm = false
            new_game()
        end
        return
    end

    if btn == "back" then
        save_game_state()
        smudge.exit()
        return
    end

    if not in_game then
        if btn == "confirm" then
            new_game()
        end
        return
    end

    -- In-game navigation matching WordleActivity.h
    if btn == "up" then
        if kb_index == 29 then
            -- Stay on reset button
        elseif kb_index == 0 then
            kb_index = 9
        elseif kb_index == 10 then
            kb_index = 19
        elseif kb_index == 20 then
            kb_index = 28
        elseif kb_index == 28 then
            kb_index = 27
        else
            kb_index = kb_index - 1
        end
    elseif btn == "down" then
        if kb_index == 29 then
            -- Stay on reset button
        elseif kb_index == 9 then
            kb_index = 0
        elseif kb_index == 19 then
            kb_index = 10
        elseif kb_index == 28 then
            kb_index = 20
        elseif kb_index == 27 then
            kb_index = 28
        else
            kb_index = kb_index + 1
        end
    elseif btn == "left" or btn == "page_back" then
        if kb_index == 29 then
            kb_index = 24 -- reset -> Y
        elseif kb_index == 4 or kb_index == 5 then -- E or F -> reset
            kb_index = 29
        elseif kb_index < 10 then -- Row 0 -> Row 2
            if kb_index >= 8 then
                kb_index = 28 -- I or J -> clear
            else
                kb_index = 20 + kb_index
            end
        elseif kb_index < 20 then -- Row 1 -> Row 0
            kb_index = kb_index - 10
        else -- Row 2 -> Row 1
            if kb_index == 28 then
                kb_index = 19
            else
                kb_index = kb_index - 10
            end
        end
    elseif btn == "right" or btn == "page_forward" then
        if kb_index == 29 then
            kb_index = 4 -- reset -> E
        elseif kb_index == 24 or kb_index == 25 then -- Y or Z -> reset
            kb_index = 29
        elseif kb_index < 10 then -- Row 0 -> Row 1
            kb_index = kb_index + 10
        elseif kb_index < 20 then -- Row 1 -> Row 2
            if kb_index >= 18 then
                kb_index = 28 -- S or T -> clear
            else
                kb_index = kb_index + 10
            end
        else -- Row 2 -> Row 0
            local col = kb_index - 20
            if kb_index == 28 then
                kb_index = 9
            else
                kb_index = col
            end
        end
    elseif btn == "confirm" then
        handle_key_action(kb_index)
    end
end

function on_tap(x, y)
    if show_reset_confirm then
        -- Tap anywhere to cancel, or top/bottom
        show_reset_confirm = false
        return
    end

    local m = smudge.get_metrics()
    if y > h - m.button_hints_height then
        if x < w / 4 then
            save_game_state()
            smudge.exit()
        elseif x < w / 2 then
            handle_key_action(kb_index)
        elseif x < 3 * w / 4 then
            on_button("up", true)
        else
            on_button("down", true)
        end
        return
    end

    local availableWidth = w - 24
    local tilePadding = 6
    local tileSize = math.floor((availableWidth - (tilePadding * 4)) / 5)
    if tileSize > 54 then tileSize = 54 end
    local gridTopY = m.top_padding + m.header_height + 8
    local kbTopY = gridTopY + (6 * (tileSize + tilePadding)) + 12
    local keyPad = 3
    local keyW = math.floor((availableWidth - (9 * keyPad)) / 10)
    local keyH = keyW
    local kbRowWidth = (10 * keyW) + (9 * keyPad)
    local kbStartX = math.floor((w - kbRowWidth) / 2)

    -- Check reset button tap
    local resetW = keyW * 2 + keyPad
    local resetX = math.floor((w - resetW) / 2)
    local resetY = kbTopY + 3 * (keyH + keyPad)
    if x >= resetX and x <= resetX + resetW and y >= resetY and y <= resetY + keyH then
        kb_index = 29
        handle_key_action(29)
        return
    end

    -- Check keyboard keys tap
    if y >= kbTopY and y < resetY then
        local row = math.floor((y - kbTopY) / (keyH + keyPad))
        if row == 0 or row == 1 then
            local col = math.floor((x - kbStartX) / (keyW + keyPad))
            if col >= 0 and col < 10 then
                local idx = row * 10 + col
                kb_index = idx
                handle_key_action(idx)
            end
        elseif row == 2 then
            local okClearTotalW = 3 * keyW + 2 * keyPad
            local okW = math.floor((okClearTotalW - keyPad) / 2)
            local clearW = (okClearTotalW - keyPad) - okW
            local okX = kbStartX + 7 * (keyW + keyPad)
            local clearX = okX + okW + keyPad

            if x >= kbStartX and x < kbStartX + 7 * (keyW + keyPad) then
                local col = math.floor((x - kbStartX) / (keyW + keyPad))
                local idx = 20 + col
                kb_index = idx
                handle_key_action(idx)
            elseif x >= okX and x < okX + okW then
                kb_index = 27
                handle_key_action(27)
            elseif x >= clearX and x <= clearX + clearW then
                kb_index = 28
                handle_key_action(28)
            end
        end
    end
end
