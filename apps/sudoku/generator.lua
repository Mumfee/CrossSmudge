-- Sudoku procedural board generator for CrossSmudge
-- Fills solution grid and carves clues based on difficulty

local steps = {1, 2, 4, 5, 7, 8}

local function is_safe(grid, row, col, num)
    for x = 1, 9 do
        local r_idx = (row - 1) * 9 + x
        local c_idx = (x - 1) * 9 + col
        if grid[r_idx] == num or grid[c_idx] == num then
            return false
        end
    end
    local sr = row - ((row - 1) % 3)
    local sc = col - ((col - 1) % 3)
    for r = 0, 2 do
        for c = 0, 2 do
            local idx = (sr + r - 1) * 9 + (sc + c)
            if grid[idx] == num then
                return false
            end
        end
    end
    return true
end

local function fill_grid(grid, cell_idx, counter)
    counter[1] = counter[1] + 1
    if counter[1] > 400 then return false end
    if cell_idx > 81 then return true end
    local row = math.floor((cell_idx - 1) / 9) + 1
    local col = ((cell_idx - 1) % 9) + 1

    if grid[cell_idx] ~= 0 then
        return fill_grid(grid, cell_idx + 1, counter)
    end

    local start = math.random(1, 9)
    local step = steps[math.random(1, 6)]

    for i = 0, 8 do
        local num = ((start + i * step - 1) % 9) + 1
        if is_safe(grid, row, col, num) then
            grid[cell_idx] = num
            if fill_grid(grid, cell_idx + 1, counter) then
                return true
            end
            grid[cell_idx] = 0
        end
    end
    return false
end

local function fill_box(grid, sr, sc)
    local nums = {1, 2, 3, 4, 5, 6, 7, 8, 9}
    for i = 9, 2, -1 do
        local j = math.random(1, i)
        nums[i], nums[j] = nums[j], nums[i]
    end
    local idx = 1
    for r = 0, 2 do
        for c = 0, 2 do
            grid[(sr + r - 1) * 9 + (sc + c)] = nums[idx]
            idx = idx + 1
        end
    end
end

local function generate_board(b, s, init_b, diff)
    local counter = {0}
    local solved = false
    local attempts = 0

    while not solved and attempts < 15 do
        attempts = attempts + 1
        counter[1] = 0
        for i = 1, 81 do
            s[i] = 0
            b[i] = 0
            init_b[i] = false
        end

        fill_box(s, 1, 1)
        fill_box(s, 4, 4)
        fill_box(s, 7, 7)
        solved = fill_grid(s, 1, counter)
    end

    for i = 1, 81 do
        b[i] = s[i]
    end

    local remove_count = (diff == 1) and 35 or ((diff == 2) and 45 or 52)
    while remove_count > 0 do
        local idx = math.random(1, 81)
        if b[idx] ~= 0 then
            b[idx] = 0
            remove_count = remove_count - 1
        end
    end

    for i = 1, 81 do
        init_b[i] = (b[i] ~= 0)
    end
end

return generate_board
