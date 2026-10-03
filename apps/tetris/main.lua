-- Tetris game for CrossSmudge (Lua)
-- Faithful port of original C++ TetrisActivity with real-time gravity, 7 piece patterns, dithering & background render

local kGridWidth = 10
local kGridHeight = 20
local grid = {}

-- 7 Standard Tetrominoes in 4 Rotations (matching original C++ kTetrominoes)
local kTetrominoes = {
    -- 1: I
    {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
     {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
     {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
    -- 2: J
    {{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}},
    -- 3: L
    {{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
     {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}},
    -- 4: O
    {{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}},
    -- 5: S
    {{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
     {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    -- 6: T
    {{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    -- 7: Z
    {{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}}
}

-- 7 Unique Styles mapped to the 7 pieces (matching C++ kPieceStyles)
-- 1: SolidBlack, 2: SolidWhite, 3: SegmentedWhite, 4: SolidLightGray, 5: SegmentedLightGray, 6: SolidDarkGray, 7: SegmentedDarkGray
local kPieceStyles = {
    1, -- 1: I -> SolidBlack
    5, -- 2: J -> SegmentedLightGray
    4, -- 3: L -> SolidLightGray
    2, -- 4: O -> SolidWhite
    6, -- 5: S -> SolidDarkGray
    3, -- 6: T -> SegmentedWhite
    7  -- 7: Z -> SegmentedDarkGray
}

local current_piece = 1
local current_rot = 1
local piece_x = 3
local piece_y = 0
local next_piece = 2

local score = 0
local lines_cleared = 0
local level = 1
local is_game_over = false
local show_reset_confirm = false

local last_fall_time = 0
local last_drop_hold_time = 0

-- Line Clear Animation State (matching C++ kClearDelayMs = 150)
local is_clearing_lines = false
local clear_start_time = 0
local lines_to_clear = {}

local w, h = 480, 800

local function init_grid()
    grid = {}
    lines_to_clear = {}
    for y = 0, kGridHeight - 1 do
        grid[y] = {}
        lines_to_clear[y] = false
        for x = 0, kGridWidth - 1 do
            grid[y][x] = 0
        end
    end
end

local function can_move(p_idx, test_x, test_y, rot)
    local shape = kTetrominoes[p_idx][rot]
    for py = 1, 4 do
        for px = 1, 4 do
            if shape[py][px] ~= 0 then
                local targetX = test_x + px - 1
                local targetY = test_y + py - 1

                if targetX < 0 or targetX >= kGridWidth or targetY >= kGridHeight then
                    return false
                end
                if targetY >= 0 and grid[targetY][targetX] ~= 0 then
                    return false
                end
            end
        end
    end
    return true
end

local function spawn_piece()
    current_piece = next_piece
    next_piece = math.random(1, 7)
    current_rot = 1
    piece_x = 3
    piece_y = 0

    if not can_move(current_piece, piece_x, piece_y, current_rot) then
        is_game_over = true
    end
end

local function check_and_start_line_clear()
    local cleared_count = 0
    for y = 0, kGridHeight - 1 do
        local full = true
        for x = 0, kGridWidth - 1 do
            if grid[y][x] == 0 then
                full = false
                break
            end
        end
        lines_to_clear[y] = full
        if full then
            cleared_count = cleared_count + 1
        end
    end

    if cleared_count > 0 then
        is_clearing_lines = true
        clear_start_time = smudge.millis()
    else
        spawn_piece()
    end
end

local function finish_line_clear()
    local cleared = 0
    local y = kGridHeight - 1
    while y >= 0 do
        if lines_to_clear[y] then
            cleared = cleared + 1
            for pullY = y, 1, -1 do
                for x = 0, kGridWidth - 1 do
                    grid[pullY][x] = grid[pullY - 1][x]
                end
                lines_to_clear[pullY] = lines_to_clear[pullY - 1]
            end
            for x = 0, kGridWidth - 1 do
                grid[0][x] = 0
            end
            lines_to_clear[0] = false
            -- Re-check same row index
        else
            y = y - 1
        end
    end

    is_clearing_lines = false

    if cleared > 0 then
        lines_cleared = lines_cleared + cleared
        level = math.floor(lines_cleared / 10) + 1

        if cleared == 1 then score = score + 100 * level
        elseif cleared == 2 then score = score + 300 * level
        elseif cleared == 3 then score = score + 500 * level
        elseif cleared >= 4 then score = score + 800 * level
        end
    end

    spawn_piece()
end

local function lock_piece()
    local shape = kTetrominoes[current_piece][current_rot]
    for py = 1, 4 do
        for px = 1, 4 do
            if shape[py][px] ~= 0 then
                local targetX = piece_x + px - 1
                local targetY = piece_y + py - 1
                if targetY >= 0 and targetY < kGridHeight and targetX >= 0 and targetX < kGridWidth then
                    grid[targetY][targetX] = current_piece
                end
            end
        end
    end

    check_and_start_line_clear()
end

local function save_game_state()
    local rows = {}
    for y = 0, kGridHeight - 1 do
        table.insert(rows, table.concat(grid[y], ","))
    end
    local gridStr = table.concat(rows, ";")
    local data = string.format("%d|%d|%d|%d|%d|%d|%d|%d|%s",
        score, lines_cleared, level, current_piece, current_rot, piece_x, piece_y, next_piece, gridStr)
    smudge.save("state", data)
end

local function load_game_state()
    local data = smudge.load("state")
    if not data or data == "" then return false end
    local parts = {}
    for part in string.gmatch(data, "[^|]+") do
        table.insert(parts, part)
    end
    if #parts < 9 then return false end
    score = tonumber(parts[1]) or 0
    lines_cleared = tonumber(parts[2]) or 0
    level = tonumber(parts[3]) or 1
    current_piece = tonumber(parts[4]) or 1
    current_rot = tonumber(parts[5]) or 1
    piece_x = tonumber(parts[6]) or 3
    piece_y = tonumber(parts[7]) or 0
    next_piece = tonumber(parts[8]) or 2
    local gridStr = parts[9]
    local y = 0
    for row in string.gmatch(gridStr, "[^;]+") do
        if y < kGridHeight then
            local x = 0
            for val in string.gmatch(row, "[^,]+") do
                if x < kGridWidth then
                    grid[y][x] = tonumber(val) or 0
                    x = x + 1
                end
            end
            y = y + 1
        end
    end
    return true
end

local function delete_save_state()
    smudge.save("state", "")
end

local function start_new_game()
    init_grid()
    score = 0
    lines_cleared = 0
    level = 1
    is_game_over = false
    show_reset_confirm = false
    is_clearing_lines = false
    next_piece = math.random(1, 7)
    spawn_piece()
    last_fall_time = smudge.millis()
    last_drop_hold_time = smudge.millis()
end

function on_init()
    w, h = smudge.get_bounds()
    math.randomseed(os.time())
    init_grid()
    if not load_game_state() then
        start_new_game()
    else
        last_fall_time = smudge.millis()
        last_drop_hold_time = smudge.millis()
    end
    smudge.request_update()
end

function on_exit()
    if not is_game_over and not show_reset_confirm then
        save_game_state()
    else
        delete_save_state()
    end
end

-- Game loop: matching C++ TetrisActivity::loop()
function on_update()
    if show_reset_confirm or is_game_over then
        return
    end

    local now = smudge.millis()

    -- 4. Line Clear Flash Animation Wait (150ms)
    if is_clearing_lines then
        if now - clear_start_time >= 150 then
            finish_line_clear()
            smudge.request_update()
        end
        return
    end

    -- 6. Hold-to-Drop (Continuous Fast Soft Drop every 50ms)
    if smudge.is_button_down("confirm") then
        if now - last_drop_hold_time >= 50 then
            last_drop_hold_time = now
            if can_move(current_piece, piece_x, piece_y + 1, current_rot) then
                piece_y = piece_y + 1
                score = score + 1
                last_fall_time = now
                smudge.request_update()
            else
                lock_piece()
                smudge.request_update()
            end
        end
    end

    -- 7. Standard Gravity / Automatic Fall
    local fall_interval = math.max(120, 800 - ((level - 1) * 70))
    if now - last_fall_time >= fall_interval then
        last_fall_time = now
        if can_move(current_piece, piece_x, piece_y + 1, current_rot) then
            piece_y = piece_y + 1
            smudge.request_update()
        else
            lock_piece()
            smudge.request_update()
        end
    end
end

local function draw_block(x, y, size, style)
    if style == 1 then
        -- SolidBlack
        smudge.rect(x, y, size, size, true, true)
    elseif style == 2 then
        -- SolidWhite
        smudge.rect(x, y, size, size, false, true)
        if size > 2 then
            smudge.rect(x + 1, y + 1, size - 2, size - 2, true, false)
        end
    elseif style == 3 then
        -- SegmentedWhite
        smudge.rect(x, y, size, size, false, true)
        if size > 4 then
            smudge.rect(x + 1, y + 1, size - 2, size - 2, true, false)
            smudge.rect(x + 2, y + 2, size - 4, size - 4, false, true)
        end
    elseif style == 4 then
        -- SolidLightGray
        smudge.rect(x, y, size, size, false, true)
        if size > 2 then
            smudge.rect_dither(x + 1, y + 1, size - 2, size - 2, false)
        end
    elseif style == 5 then
        -- SegmentedLightGray
        smudge.rect(x, y, size, size, false, true)
        if size > 4 then
            smudge.rect_dither(x + 1, y + 1, size - 2, size - 2, false)
            smudge.rect(x + 2, y + 2, size - 4, size - 4, false, true)
        end
    elseif style == 6 then
        -- SolidDarkGray
        smudge.rect(x, y, size, size, false, true)
        if size > 2 then
            smudge.rect_dither(x + 1, y + 1, size - 2, size - 2, true)
        end
    elseif style == 7 then
        -- SegmentedDarkGray
        smudge.rect(x, y, size, size, false, true)
        if size > 4 then
            smudge.rect_dither(x + 1, y + 1, size - 2, size - 2, true)
            smudge.rect(x + 2, y + 2, size - 4, size - 4, false, true)
        end
    else
        smudge.rect(x, y, size, size, true, true)
    end
end

-- Render function: matching C++ TetrisActivity::render()
function on_draw()
    smudge.clear()

    local m = smudge.get_metrics()
    local topBound = m.top_padding + m.header_height
    local bottomBound = h - m.button_hints_height
    local availableHeight = bottomBound - topBound - 12

    -- 1. Fill Main Background with Light Gray Dither (Classic look restored!)
    smudge.rect_dither(0, topBound, w, bottomBound - topBound, false)

    -- 2. Header with Score, Lines, Level
    local headerRight = string.format("Score:%d | Lns:%d | Lvl:%d", score, lines_cleared, level)
    smudge.header("Tetris", headerRight)

    -- Calculate grid tile dimensions
    local blockSize = math.min(math.floor(availableHeight / kGridHeight), math.floor((w - 110) / kGridWidth))
    local boardPixelW = blockSize * kGridWidth
    local boardPixelH = blockSize * kGridHeight
    local boardX = 20
    local boardY = topBound + 6 + math.floor((availableHeight - boardPixelH) / 2)

    -- 3. Draw Solid White Board Area & Outer Boundary
    smudge.rect(boardX, boardY, boardPixelW, boardPixelH, true, false) -- Solid white board
    smudge.rect(boardX - 2, boardY - 2, boardPixelW + 4, boardPixelH + 4, false, true) -- Black outline

    -- 4. Draw Grid Cells
    for y = 0, kGridHeight - 1 do
        if is_clearing_lines and lines_to_clear[y] then
            -- Full row flash animation in solid black
            smudge.rect(boardX, boardY + (y * blockSize), boardPixelW, blockSize, true, true)
        else
            for x = 0, kGridWidth - 1 do
                local cellX = boardX + (x * blockSize)
                local cellY = boardY + (y * blockSize)
                local cellVal = grid[y][x]

                if cellVal > 0 then
                    local style = kPieceStyles[cellVal] or 1
                    draw_block(cellX, cellY, blockSize, style)
                else
                    -- Background dot grid
                    local dotX = cellX + math.floor(blockSize / 2)
                    local dotY = cellY + math.floor(blockSize / 2)
                    smudge.pixel(dotX, dotY, true)
                end
            end
        end
    end

    -- 5. Draw Current Falling Piece
    if not is_game_over and not is_clearing_lines then
        local shape = kTetrominoes[current_piece][current_rot]
        local style = kPieceStyles[current_piece] or 1
        for py = 1, 4 do
            for px = 1, 4 do
                if shape[py][px] ~= 0 then
                    local targetX = piece_x + px - 1
                    local targetY = piece_y + py - 1
                    if targetY >= 0 and targetY < kGridHeight and targetX >= 0 and targetX < kGridWidth then
                        local bx = boardX + (targetX * blockSize)
                        local by = boardY + (targetY * blockSize)
                        draw_block(bx, by, blockSize, style)
                    end
                end
            end
        end
    end

    -- 6. Sidebar - Next Piece Box Preview
    local sidebarX = boardX + boardPixelW + 16
    local sidebarY = boardY + 12

    smudge.text(sidebarX, sidebarY, "Next", 12, true, "left")
    sidebarY = sidebarY + 28

    local previewBoxSize = 56
    smudge.rounded_rect(sidebarX, sidebarY, previewBoxSize, previewBoxSize, 4, true, false)
    smudge.rounded_rect(sidebarX, sidebarY, previewBoxSize, previewBoxSize, 4, false, true, 1)

    -- Center piece inside preview box
    local pBlockSize = 10
    local nextShape = kTetrominoes[next_piece][1]
    local nextStyle = kPieceStyles[next_piece] or 1

    local minX, maxX, minY, maxY = 5, -1, 5, -1
    for py = 1, 4 do
        for px = 1, 4 do
            if nextShape[py][px] ~= 0 then
                if px < minX then minX = px end
                if px > maxX then maxX = px end
                if py < minY then minY = py end
                if py > maxY then maxY = py end
            end
        end
    end

    local shapePixelW = (maxX - minX + 1) * pBlockSize
    local shapePixelH = (maxY - minY + 1) * pBlockSize
    local startPreviewX = sidebarX + math.floor((previewBoxSize - shapePixelW) / 2)
    local startPreviewY = sidebarY + math.floor((previewBoxSize - shapePixelH) / 2)

    for py = minY, maxY do
        for px = minX, maxX do
            if nextShape[py][px] ~= 0 then
                local bx = startPreviewX + (px - minX) * pBlockSize
                local by = startPreviewY + (py - minY) * pBlockSize
                draw_block(bx, by, pBlockSize, nextStyle)
            end
        end
    end

    -- 7. Modal Confirmation Dialog
    if show_reset_confirm then
        local modalW = w - 60
        local modalH = 60
        local modalX = math.floor((w - modalW) / 2)
        local modalY = math.floor((h - modalH) / 2) - 20

        smudge.rect(modalX, modalY, modalW, modalH, true, true)
        smudge.rect(modalX + 2, modalY + 2, modalW - 4, modalH - 4, false, false)
        smudge.centered_text(modalY + 16, "RESET GAME?", 12, true, false)
    elseif is_game_over then
        local modalW = 180
        local modalH = 70
        local modalX = math.floor((w - modalW) / 2)
        local modalY = boardY + math.floor((boardPixelH - modalH) / 2)

        smudge.rounded_rect(modalX, modalY, modalW, modalH, 6, true, false)
        smudge.rounded_rect(modalX, modalY, modalW, modalH, 6, false, true, 2)
        smudge.centered_text(modalY + 16, "GAME OVER", 12, true, true)
        smudge.centered_text(modalY + 40, "Press Drop to Play", 10, false, true)
    end

    -- 8. Footer Button Hints
    if show_reset_confirm then
        smudge.button_hints("Cancel", "Confirm", "", "")
    elseif is_game_over then
        smudge.button_hints("Back", "Restart", "", "")
    else
        smudge.button_hints("Back", "Drop", "Left", "Right")
    end
end

-- Button handling: matching C++ TetrisActivity::loop()
function on_button(btn, pressed)
    if not pressed then return end

    -- 1. Reset Confirmation Modal Handling
    if show_reset_confirm then
        if btn == "back" then
            show_reset_confirm = false
            smudge.request_update()
            return
        end
        if btn == "confirm" then
            show_reset_confirm = false
            start_new_game()
            smudge.request_update()
            return
        end
        return
    end

    -- 2. Standard Exit
    if btn == "back" then
        if not is_game_over then
            save_game_state()
        else
            delete_save_state()
        end
        smudge.exit()
        return
    end

    -- 3. Game Over Restart
    if is_game_over then
        if btn == "confirm" then
            start_new_game()
            smudge.request_update()
        end
        return
    end

    -- 4. Line Clear Animation Wait
    if is_clearing_lines then
        return
    end

    -- 5. In-Game Controls
    if btn == "down" or btn == "page_forward" then
        show_reset_confirm = true
        smudge.request_update()
        return
    end

    if btn == "left" then
        if can_move(current_piece, piece_x - 1, piece_y, current_rot) then
            piece_x = piece_x - 1
            smudge.request_update()
        end
    elseif btn == "right" then
        if can_move(current_piece, piece_x + 1, piece_y, current_rot) then
            piece_x = piece_x + 1
            smudge.request_update()
        end
    elseif btn == "up" or btn == "page_back" then
        -- Up Button = Rotate Piece (with wall-kick left, then wall-kick right)
        local nextRot = (current_rot % 4) + 1
        if can_move(current_piece, piece_x, piece_y, nextRot) then
            current_rot = nextRot
            smudge.request_update()
        elseif can_move(current_piece, piece_x - 1, piece_y, nextRot) then
            piece_x = piece_x - 1
            current_rot = nextRot
            smudge.request_update()
        elseif can_move(current_piece, piece_x + 1, piece_y, nextRot) then
            piece_x = piece_x + 1
            current_rot = nextRot
            smudge.request_update()
        end
    end
end

-- Touch handling for touchscreen devices
function on_tap(x, y)
    local m = smudge.get_metrics()
    if y > h - m.button_hints_height then
        if show_reset_confirm then
            if x < w / 2 then
                on_button("back", true)
            else
                on_button("confirm", true)
            end
            return
        end
        if is_game_over then
            if x < w / 2 then
                on_button("back", true)
            else
                on_button("confirm", true)
            end
            return
        end
        if x < w / 4 then
            on_button("back", true)
        elseif x < w / 2 then
            -- Tapping drop button does a manual soft-drop step
            if not is_clearing_lines then
                if can_move(current_piece, piece_x, piece_y + 1, current_rot) then
                    piece_y = piece_y + 1
                    score = score + 1
                    last_fall_time = smudge.millis()
                    smudge.request_update()
                else
                    lock_piece()
                    smudge.request_update()
                end
            end
        elseif x < 3 * w / 4 then
            on_button("left", true)
        else
            on_button("right", true)
        end
        return
    end

    if show_reset_confirm or is_game_over or is_clearing_lines then
        return
    end

    -- Screen zone taps:
    -- Top 1/3: rotate
    -- Bottom 1/3: soft drop step
    -- Left: move left
    -- Right: move right
    if y < h / 3 then
        on_button("up", true)
    elseif y > 2 * h / 3 then
        if can_move(current_piece, piece_x, piece_y + 1, current_rot) then
            piece_y = piece_y + 1
            score = score + 1
            last_fall_time = smudge.millis()
            smudge.request_update()
        else
            lock_piece()
            smudge.request_update()
        end
    elseif x < w / 2 then
        on_button("left", true)
    else
        on_button("right", true)
    end
end
