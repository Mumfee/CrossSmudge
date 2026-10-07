-- Stopwatch
-- Pixel cartoon stopwatch with digital display and lap split recording

local w, h = 480, 800
if smudge and smudge.get_bounds then
    w, h = smudge.get_bounds()
end

local is_running = false
local start_time_ms = 0
local accumulated_ms = 0
local last_split_total = 0
local splits = {}
local scroll_offset = 0

local last_render_ms = 0
local plunger_down = false
local plunger_timer = 0
local split_flash_timer = 0

local function get_now_ms()
    if smudge and smudge.millis then
        return smudge.millis()
    end
    return math.floor(os.time() * 1000)
end

local function get_elapsed_ms()
    if is_running then
        return accumulated_ms + (get_now_ms() - start_time_ms)
    end
    return accumulated_ms
end

local function format_time(ms)
    local total_cs = math.floor(ms / 10)
    local cs = total_cs % 100
    local total_sec = math.floor(ms / 1000)
    local s = total_sec % 60
    local total_min = math.floor(total_sec / 60)
    local m = total_min % 60
    local hr = math.floor(total_min / 60)

    if hr > 0 then
        return string.format("%d:%02d:%02d.%02d", hr, m, s, cs)
    else
        return string.format("%02d:%02d.%02d", m, s, cs)
    end
end

local function save_data()
    if not (smudge and smudge.write_file) then return end
    local el = get_elapsed_ms()
    local lines = { string.format("%d;%d", is_running and 1 or 0, el) }
    for _, sp in ipairs(splits) do
        table.insert(lines, string.format("%d;%d", sp.lap_ms, sp.total_ms))
    end
    smudge.write_file("stopwatch.dat", table.concat(lines, "\n"))
end

local function load_data()
    if not (smudge and smudge.read_file) then return end
    local content = smudge.read_file("stopwatch.dat")
    if not content or content == "" then return end
    local lines = {}
    for line in string.gmatch(content, "[^\r\n]+") do
        table.insert(lines, line)
    end
    if #lines >= 1 then
        local p = {}
        for x in string.gmatch(lines[1], "[^;]+") do table.insert(p, x) end
        if #p >= 2 then
            local saved_running = (tonumber(p[1]) == 1)
            accumulated_ms = tonumber(p[2]) or 0
            if saved_running then
                is_running = true
                start_time_ms = get_now_ms()
            else
                is_running = false
            end
        end
    end
    splits = {}
    for i = 2, #lines do
        local p = {}
        for x in string.gmatch(lines[i], "[^;]+") do table.insert(p, x) end
        if #p >= 2 then
            local lap = tonumber(p[1]) or 0
            local tot = tonumber(p[2]) or 0
            table.insert(splits, { lap_ms = lap, total_ms = tot })
            last_split_total = tot
        end
    end
end

local function trigger_start_stop()
    local now = get_now_ms()
    plunger_down = true
    plunger_timer = now + 160

    if not is_running then
        is_running = true
        start_time_ms = now
    else
        is_running = false
        accumulated_ms = accumulated_ms + (now - start_time_ms)
        save_data()
    end
    if smudge and smudge.request_update then smudge.request_update() end
end

local function trigger_split()
    local now = get_now_ms()
    local elapsed = get_elapsed_ms()
    if not is_running then
        return
    end

    local lap_time = elapsed - last_split_total
    last_split_total = elapsed
    table.insert(splits, { lap_ms = lap_time, total_ms = elapsed })
    scroll_offset = 0
    split_flash_timer = now + 1200
    save_data()
    if smudge and smudge.request_update then smudge.request_update() end
end

local function trigger_reset()
    is_running = false
    accumulated_ms = 0
    start_time_ms = 0
    last_split_total = 0
    splits = {}
    scroll_offset = 0
    split_flash_timer = 0
    save_data()
    if smudge and smudge.request_update then smudge.request_update() end
end

function on_init()
    if smudge and smudge.prevent_sleep then
        smudge.prevent_sleep(true)
    end
    if smudge and smudge.get_bounds then
        w, h = smudge.get_bounds()
    end
    load_data()
end

function on_exit()
    if smudge and smudge.prevent_sleep then
        smudge.prevent_sleep(false)
    end
    save_data()
end

function on_update()
    local now = get_now_ms()

    -- Plunger mechanical spring return
    if plunger_down and now >= plunger_timer then
        plunger_down = false
        if smudge and smudge.request_update then smudge.request_update() end
    end

    -- Periodic redraw while running (180ms ~ 5.5 fps partial refresh)
    if is_running and (now - last_render_ms) >= 180 then
        last_render_ms = now
        if smudge and smudge.request_update then smudge.request_update() end
    end
end

local function draw_stopwatch_art(cx, cy, elapsed_ms)
    local R = 100
    local top_y = cy - R

    -- Top metal lanyard loop (clean 24px clearance below header line)
    smudge.circle(cx, top_y - 18, 16, false)
    smudge.circle(cx, top_y - 18, 14, false)

    -- Top crown plunger
    local py = plunger_down and (top_y - 6) or (top_y - 10)
    smudge.rounded_rect(cx - 14, py, 28, 8, 3, true)
    smudge.rect(cx - 6, py + 8, 12, top_y - (py + 8), true)

    -- Angled secondary buttons (ears at 10 and 2 o'clock)
    local l_bx, l_by = cx - 64, cy - 64
    smudge.rect(l_bx - 8, l_by - 8, 12, 7, true)
    local r_bx, r_by = cx + 64, cy - 64
    smudge.rect(r_bx - 4, r_by - 8, 12, 7, true)

    -- Outer circular casing (double bezel)
    smudge.circle(cx, cy, R, false)
    smudge.circle(cx, cy, R - 1, false)
    smudge.circle(cx, cy, R - 5, false)

    -- 12 Dial tick marks
    for hr = 0, 11 do
        local angle = hr * (2 * math.pi / 12) - (math.pi / 2)
        local r1 = R - 7
        local is_cardinal = (hr % 3 == 0)
        local r2 = is_cardinal and (R - 15) or (R - 11)
        local x1 = cx + math.floor(math.cos(angle) * r1)
        local y1 = cy + math.floor(math.sin(angle) * r1)
        local x2 = cx + math.floor(math.cos(angle) * r2)
        local y2 = cy + math.floor(math.sin(angle) * r2)
        if is_cardinal then
            smudge.thick_line(x1, y1, x2, y2, 2, true)
        else
            smudge.line(x1, y1, x2, y2, true)
        end
    end

    -- Second sweep indicator dot on rim when running
    if is_running then
        local sec_frac = (elapsed_ms % 60000) / 60000
        local sec_ang = sec_frac * 2 * math.pi - (math.pi / 2)
        local dot_x = cx + math.floor(math.cos(sec_ang) * (R - 9))
        local dot_y = cy + math.floor(math.sin(sec_ang) * (R - 9))
        smudge.circle(dot_x, dot_y, 4, true)
    end

    -- Cute Cartoon Face
    local eye_y = cy - 34
    for _, ex in ipairs({cx - 32, cx + 32}) do
        if is_running then
            -- Blinked / effort squint eyes (^ ^) - trying hard!
            smudge.thick_line(ex - 8, eye_y, ex, eye_y - 4, 2, true)
            smudge.thick_line(ex, eye_y - 4, ex + 8, eye_y, 2, true)
        else
            -- Wide sparkly eyes
            smudge.rounded_rect(ex - 8, eye_y - 10, 16, 20, 6, true)
            -- Double white pupil shines (big top-right, small bottom-left)
            smudge.circle(ex + 3, eye_y - 4, 3, true, false)
            smudge.circle(ex - 3, eye_y + 3, 2, true, false)
        end
        -- Cheeks: 2 cute diagonal blush lines
        smudge.line(ex - 11, eye_y + 14, ex - 7, eye_y + 18, true)
        smudge.line(ex - 7, eye_y + 14, ex - 3, eye_y + 18, true)
    end

    -- Energetic sweat drop on cheek when running hard
    if is_running and ((elapsed_ms % 2000) > 400) then
        local sx, sy = cx + 54, cy - 26
        smudge.circle(sx, sy, 3, true)
        smudge.line(sx, sy - 4, sx + 2, sy - 1, true)
    end

    -- Cute smile
    smudge.thick_line(cx - 6, cy - 14, cx, cy - 12, 2, true)
    smudge.thick_line(cx, cy - 12, cx + 6, cy - 14, 2, true)

    -- Digital Clock LCD Window
    local win_w, win_h = 164, 48
    local win_x = cx - math.floor(win_w / 2)
    local win_y = cy + 4
    smudge.rounded_rect(win_x, win_y, win_w, win_h, 7, false)
    smudge.rounded_rect(win_x + 2, win_y + 2, win_w - 4, win_h - 4, 5, false)

    -- Digital time display
    local time_str = format_time(elapsed_ms)
    smudge.text(cx, win_y + 9, time_str, 16, true, "center", true)

    -- State Pill Badge below clock window
    local tag_w, tag_h = 88, 22
    local tag_x = cx - math.floor(tag_w / 2)
    local tag_y = cy + 58

    if is_running then
        smudge.rounded_rect(tag_x, tag_y, tag_w, tag_h, 6, true)
        smudge.text(cx, tag_y + 2, "RUNNING", 8, true, "center", false)
    elseif elapsed_ms > 0 then
        smudge.rounded_rect(tag_x, tag_y, tag_w, tag_h, 6, false)
        smudge.text(cx, tag_y + 2, "STOPPED", 8, true, "center", true)
    else
        smudge.rounded_rect(tag_x, tag_y, tag_w, tag_h, 6, false)
        smudge.text(cx, tag_y + 2, "READY", 8, true, "center", true)
    end
end

local function draw_splits_card(card_x, sy, card_w, card_h)
    smudge.rounded_rect(card_x, sy, card_w, card_h, 8, false)

    -- Header row
    smudge.text(card_x + 16, sy + 12, "SPLITS", 10, true, "left", true)

    -- Best split calculation
    local best_ms = nil
    local worst_ms = nil
    if #splits > 0 then
        best_ms = splits[1].lap_ms
        worst_ms = splits[1].lap_ms
        for _, sp in ipairs(splits) do
            if sp.lap_ms < best_ms then best_ms = sp.lap_ms end
            if sp.lap_ms > worst_ms then worst_ms = sp.lap_ms end
        end

        -- Lap count pill badge (offset to card_x + 106 to prevent overlap)
        local cnt_str = string.format("%d %s", #splits, #splits == 1 and "LAP" or "LAPS")
        local pw = 64
        smudge.rounded_rect(card_x + 106, sy + 10, pw, 22, 5, false)
        smudge.text(card_x + 106 + math.floor(pw / 2), sy + 12, cnt_str, 8, true, "center", true)

        if #splits >= 2 then
            local best_str = string.format("BEST: +%s", format_time(best_ms))
            smudge.text(card_x + card_w - 16, sy + 12, best_str, 9, true, "right", true)
        end
    end

    smudge.line(card_x + 12, sy + 36, card_x + card_w - 12, sy + 36, true)

    -- Column headers
    local col_y = sy + 44
    smudge.text(card_x + 18, col_y, "LAP", 9, true, "left", true)
    smudge.text(card_x + math.floor(card_w / 2), col_y, "SPLIT TIME", 9, true, "center", true)
    smudge.text(card_x + card_w - 18, col_y, "TOTAL TIME", 9, true, "right", true)
    smudge.line(card_x + 12, sy + 68, card_x + card_w - 12, sy + 68, true)

    -- Splits list or empty state
    local num_splits = #splits
    if num_splits == 0 then
        local empty_cy = sy + math.floor((card_h - 70) / 2) + 14
        -- Cute mini stopwatch icon in empty state
        smudge.circle(card_x + math.floor(card_w / 2), empty_cy - 24, 18, false)
        smudge.rounded_rect(card_x + math.floor(card_w / 2) - 4, empty_cy - 46, 8, 4, 1, true)
        smudge.circle(card_x + math.floor(card_w / 2), empty_cy - 24, 2, true)
        smudge.line(card_x + math.floor(card_w / 2), empty_cy - 24, card_x + math.floor(card_w / 2) + 6, empty_cy - 30, true)

        smudge.text(card_x + math.floor(card_w / 2), empty_cy + 10, "No Splits Recorded", 12, true, "center", true)
        smudge.text(card_x + math.floor(card_w / 2), empty_cy + 38, "Press [Start] then [Split] to record laps", 9, false, "center", true)
    else
        local max_visible = 6
        local start_idx = math.max(1, num_splits - scroll_offset - max_visible + 1)
        local end_idx = math.max(1, num_splits - scroll_offset)

        local row_y = sy + 76
        local row_h = 44

        -- Display from newest to oldest
        local display_count = 0
        for i = end_idx, start_idx, -1 do
            local sp = splits[i]
            local is_best = (num_splits >= 2 and sp.lap_ms == best_ms)

            -- Lap badge
            local badge_w, badge_h = 46, 24
            local bx = card_x + 16
            local by = row_y + 8
            if is_best then
                smudge.rounded_rect(bx, by, badge_w, badge_h, 5, true)
                smudge.text(bx + math.floor(badge_w / 2), by + 3, string.format("#%02d", i), 8, true, "center", false)
            else
                smudge.rounded_rect(bx, by, badge_w, badge_h, 5, false)
                smudge.text(bx + math.floor(badge_w / 2), by + 3, string.format("#%02d", i), 8, true, "center", true)
            end

            -- Split lap time
            local split_str = string.format("+%s", format_time(sp.lap_ms))
            if is_best then
                split_str = string.format("* +%s", format_time(sp.lap_ms))
            end
            smudge.text(card_x + math.floor(card_w / 2), row_y + 8, split_str, 10, is_best, "center", true)

            -- Total cumulative time
            local tot_str = format_time(sp.total_ms)
            smudge.text(card_x + card_w - 18, row_y + 8, tot_str, 10, false, "right", true)

            -- Row separator line
            if display_count < max_visible - 1 and i > start_idx then
                smudge.line(card_x + 14, row_y + row_h - 2, card_x + card_w - 14, row_y + row_h - 2, true)
            end

            row_y = row_y + row_h
            display_count = display_count + 1
        end

        -- Scroll hint if more splits exist
        if num_splits > max_visible then
            local footer_str = string.format("Showing %d-%d of %d (scroll with UP/DOWN)", start_idx, end_idx, num_splits)
            smudge.text(card_x + math.floor(card_w / 2), sy + card_h - 20, footer_str, 8, false, "center", true)
        end
    end
end

function on_draw()
    smudge.clear()

    local elapsed_ms = get_elapsed_ms()
    local status_sub = is_running and "Running" or (elapsed_ms > 0 and "Paused" or "Ready")
    smudge.header("Stopwatch", status_sub)

    local cx = math.floor(w / 2)
    local cy = 230

    -- Draw Cartoon Stopwatch
    draw_stopwatch_art(cx, cy, elapsed_ms)

    -- Draw Splits Card under the stopwatch
    local card_x = 24
    local card_w = w - 48
    local sy = 352
    local card_h = 380
    draw_splits_card(card_x, sy, card_w, card_h)

    -- Bottom button hints
    local b2_label = is_running and "Stop" or "Start"
    smudge.button_hints("Exit", b2_label, "Split", "Reset")
end

function on_button(btn, pressed)
    if not pressed then return end

    if btn == "btn1" or btn == "back" then
        save_data()
        smudge.exit()
    elseif btn == "btn2" or btn == "confirm" then
        trigger_start_stop()
    elseif btn == "btn3" or btn == "left" or btn == "page_back" then
        trigger_split()
    elseif btn == "btn4" or btn == "right" or btn == "page_forward" or btn == "down" then
        trigger_reset()
    elseif btn == "up" then
        if #splits > 6 then
            scroll_offset = math.min(#splits - 6, scroll_offset + 1)
            if smudge and smudge.request_update then smudge.request_update() end
        end
    end
end

function on_tap(x, y)
    -- Tap cartoon stopwatch to toggle Start / Stop
    local cx = math.floor(w / 2)
    local cy = 230
    local dx = x - cx
    local dy = y - cy
    if (dx * dx + dy * dy) <= (106 * 106) then
        trigger_start_stop()
        return
    end

    -- Tap bottom button regions
    if y >= 740 then
        local btn_w = math.floor(w / 4)
        if x < btn_w then
            save_data()
            smudge.exit()
        elseif x < btn_w * 2 then
            trigger_start_stop()
        elseif x < btn_w * 3 then
            trigger_split()
        else
            trigger_reset()
        end
        return
    end

    -- Tap splits card to scroll
    if y >= 352 and y <= 732 then
        if y < 540 and #splits > 6 then
            scroll_offset = math.min(#splits - 6, scroll_offset + 1)
            if smudge and smudge.request_update then smudge.request_update() end
        elseif y >= 540 and scroll_offset > 0 then
            scroll_offset = math.max(0, scroll_offset - 1)
            if smudge and smudge.request_update then smudge.request_update() end
        end
    end
end
