-- 2048 game for CrossSmudge (Lua)
-- Faithful reproduction of TwoZeroFourEightActivity.h

local board = {}
local score = 0
local gameOver = false
local showResetConfirm = false
local w, h = 480, 800

local function init_board()
    board = {}
    for r = 1, 4 do
        board[r] = {}
        for c = 1, 4 do
            board[r][c] = 0
        end
    end
    score = 0
    gameOver = false
    showResetConfirm = false
end

local empty_r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
local empty_c = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
local non_zero = {0, 0, 0, 0}
local res = {0, 0, 0, 0}

local function spawn_tile()
    local empty_count = 0
    for r = 1, 4 do
        for c = 1, 4 do
            if board[r][c] == 0 then
                empty_count = empty_count + 1
                empty_r[empty_count] = r
                empty_c[empty_count] = c
            end
        end
    end
    if empty_count == 0 then return false end
    local idx = math.random(1, empty_count)
    board[empty_r[idx]][empty_c[idx]] = (math.random() < 0.9) and 2 or 4
    return true
end

local function can_move()
    for r = 1, 4 do
        for c = 1, 4 do
            if board[r][c] == 0 then return true end
            if c < 4 and board[r][c] == board[r][c + 1] then return true end
            if r < 4 and board[r][c] == board[r + 1][c] then return true end
        end
    end
    return false
end

local function reset_game()
    init_board()
    spawn_tile()
    spawn_tile()
end

local function save_state()
    local flat = {}
    for r = 1, 4 do
        for c = 1, 4 do
            table.insert(flat, tostring(board[r][c]))
        end
    end
    smudge.save("board", table.concat(flat, ","))
    smudge.save("score", tostring(score))
    smudge.save("game_over", gameOver and "1" or "0")
end

local function load_state()
    local s = smudge.load("board", "")
    if s and #s > 0 then
        local parts = {}
        for p in string.gmatch(s, "([^,]+)") do
            table.insert(parts, tonumber(p) or 0)
        end
        if #parts == 16 then
            board = {}
            for r = 1, 4 do
                board[r] = {}
                for c = 1, 4 do
                    board[r][c] = parts[(r - 1) * 4 + c]
                end
            end
            score = tonumber(smudge.load("score", "0")) or 0
            gameOver = (smudge.load("game_over", "0") == "1")
            return true
        end
    end
    return false
end

function on_init()
    w, h = smudge.get_bounds()
    math.randomseed(os.time())
    if not load_state() then
        reset_game()
    end
end

local function slide_and_merge(a, b, c, d)
    local nz_count = 0
    if a ~= 0 then nz_count = nz_count + 1; non_zero[nz_count] = a end
    if b ~= 0 then nz_count = nz_count + 1; non_zero[nz_count] = b end
    if c ~= 0 then nz_count = nz_count + 1; non_zero[nz_count] = c end
    if d ~= 0 then nz_count = nz_count + 1; non_zero[nz_count] = d end

    res[1], res[2], res[3], res[4] = 0, 0, 0, 0
    local res_count = 0
    local skip = false
    local gained = 0

    for i = 1, nz_count do
        if skip then
            skip = false
        elseif i < nz_count and non_zero[i] == non_zero[i + 1] then
            local merged = non_zero[i] * 2
            res_count = res_count + 1
            res[res_count] = merged
            gained = gained + merged
            skip = true
        else
            res_count = res_count + 1
            res[res_count] = non_zero[i]
        end
    end

    return gained
end

local function move(dr, dc)
    if gameOver or showResetConfirm then return end
    local moved = false
    local total_gained = 0

    if dc == -1 then -- Left
        for r = 1, 4 do
            local g = slide_and_merge(board[r][1], board[r][2], board[r][3], board[r][4])
            total_gained = total_gained + g
            for c = 1, 4 do
                if board[r][c] ~= res[c] then moved = true end
                board[r][c] = res[c]
            end
        end
    elseif dc == 1 then -- Right
        for r = 1, 4 do
            local g = slide_and_merge(board[r][4], board[r][3], board[r][2], board[r][1])
            total_gained = total_gained + g
            for c = 1, 4 do
                if board[r][5 - c] ~= res[c] then moved = true end
                board[r][5 - c] = res[c]
            end
        end
    elseif dr == -1 then -- Up
        for c = 1, 4 do
            local g = slide_and_merge(board[1][c], board[2][c], board[3][c], board[4][c])
            total_gained = total_gained + g
            for r = 1, 4 do
                if board[r][c] ~= res[r] then moved = true end
                board[r][c] = res[r]
            end
        end
    elseif dr == 1 then -- Down
        for c = 1, 4 do
            local g = slide_and_merge(board[4][c], board[3][c], board[2][c], board[1][c])
            total_gained = total_gained + g
            for r = 1, 4 do
                if board[5 - r][c] ~= res[r] then moved = true end
                board[5 - r][c] = res[r]
            end
        end
    end

    if moved then
        score = score + total_gained
        spawn_tile()
        if not can_move() then
            gameOver = true
        end
    end
end

function on_draw()
    smudge.clear()

    local m = smudge.get_metrics()
    local headerBottomY = m.top_padding + m.header_height

    -- 1. Header with Score Display
    local subtitle = gameOver and "GAME OVER!" or string.format("Score: %d", score)
    smudge.header("2048", subtitle)

    -- 2. Maximize Grid Geometry (exact formulas from TwoZeroFourEightActivity.h)
    local boardSize = 4
    local padding = 6
    local footerHeight = 48

    local availableWidth = w - 16
    local tileSize = math.floor((availableWidth - (padding * (boardSize - 1))) / boardSize)
    local gridPixelSize = (tileSize * boardSize) + (padding * (boardSize - 1))
    local startX = math.floor((w - gridPixelSize) / 2)

    local contentAreaHeight = h - headerBottomY - footerHeight
    local startY = headerBottomY + math.floor((contentAreaHeight - gridPixelSize) / 2)
    local fontYOffset = 18

    -- 3. Render Grid & Tiles
    for r = 1, 4 do
        for c = 1, 4 do
            local x = startX + (c - 1) * (tileSize + padding)
            local y = startY + (r - 1) * (tileSize + padding)
            local val = board[r][c]

            if val == 0 then
                smudge.rect(x, y, tileSize, tileSize, false)
            else
                local valStr = tostring(val)
                local textX = x + math.floor(tileSize / 2)
                local textY = y + math.floor(tileSize / 2) - fontYOffset

                if val >= 2048 then
                    -- Inverted solid black box for high-value targets (2048+)
                    smudge.rect(x, y, tileSize, tileSize, true)
                    smudge.text(textX, textY, valStr, 0, true, "center", false) -- white text
                elseif val >= 512 then
                    smudge.rect_dither(x, y, tileSize, tileSize, true) -- dark gray dither
                    smudge.rect(x, y, tileSize, tileSize, false)
                    smudge.text(textX, textY, valStr, 0, true, "center", true)
                elseif val >= 64 then
                    smudge.rect_dither(x, y, tileSize, tileSize, false) -- light gray dither
                    smudge.rect(x, y, tileSize, tileSize, false)
                    smudge.text(textX, textY, valStr, 0, true, "center", true)
                elseif val >= 8 then
                    smudge.rect(x, y, tileSize, tileSize, false)
                    smudge.rect(x + 2, y + 2, tileSize - 4, tileSize - 4, false)
                    smudge.text(textX, textY, valStr, 0, true, "center", true)
                else
                    smudge.rect(x, y, tileSize, tileSize, false)
                    smudge.text(textX, textY, valStr, 0, true, "center", true)
                end
            end
        end
    end

    -- 4. Modal Confirmation Dialog
    if showResetConfirm then
        local modalW = w - 60
        local modalH = 60
        local modalX = math.floor((w - modalW) / 2)
        local modalY = math.floor((h - modalH) / 2) - 20

        smudge.rect(modalX, modalY, modalW, modalH, true)
        smudge.rect(modalX + 2, modalY + 2, modalW - 4, modalH - 4, false)
        smudge.centered_text(modalY + math.floor(modalH / 2) - fontYOffset, "RESET GAME?", 0, true, false)

        smudge.button_hints("Cancel", "Confirm", "", "")
    else
        smudge.button_hints("Back", "Reset", "Up", "Down")
    end
end

function on_button(btn, pressed)
    if not pressed then return end

    if showResetConfirm then
        if btn == "back" then
            showResetConfirm = false
        elseif btn == "confirm" then
            reset_game()
            showResetConfirm = false
            save_state()
        end
        return
    end

    if btn == "back" then
        save_state()
        smudge.exit()
    elseif btn == "confirm" then
        showResetConfirm = true
    elseif btn == "left" then
        move(-1, 0) -- Bottom button labeled "Up" moves UP
    elseif btn == "right" then
        move(1, 0)  -- Bottom button labeled "Down" moves DOWN
    elseif btn == "up" or btn == "page_back" then
        move(0, -1) -- Top side button moves LEFT
    elseif btn == "down" or btn == "page_forward" then
        move(0, 1)  -- Bottom side button moves RIGHT
    end
end

function on_swipe(dir)
    if dir == "up" then
        move(-1, 0)
    elseif dir == "down" then
        move(1, 0)
    elseif dir == "left" then
        move(0, -1)
    elseif dir == "right" then
        move(0, 1)
    end
end

function on_tap(x, y)
    local m = smudge.get_metrics()
    if y > h - m.button_hints_height then
        if showResetConfirm then
            if x < w / 2 then
                showResetConfirm = false
            else
                reset_game()
                showResetConfirm = false
                save_state()
            end
        else
            if x < w / 4 then
                save_state()
                smudge.exit()
            elseif x < w / 2 then
                showResetConfirm = true
            elseif x < 3 * w / 4 then
                move(-1, 0)
            else
                move(1, 0)
            end
        end
    end
end

function on_exit()
    save_state()
end
