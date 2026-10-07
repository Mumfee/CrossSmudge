-- Desk Stand Dashboard
-- Ambient e-ink desk clock, calendar, and overview station

local is_24h = false
local theme_style = 1
local NUM_STYLES = 7
local last_min, last_hour = -1, -1
local needs_full_refresh = false

local MONTHS = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}
local DAYS = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}
local SEGS = {119, 36, 93, 109, 46, 107, 123, 37, 127, 111}

local function is_leap(yr)
    return (yr % 4 == 0 and yr % 100 ~= 0) or (yr % 400 == 0)
end

local function days_in_mo(yr, mo)
    if mo == 2 then return is_leap(yr) and 29 or 28 end
    if mo == 4 or mo == 6 or mo == 9 or mo == 11 then return 30 end
    return 31
end

local function first_wday(yr, mo)
    local t = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4}
    local y = (mo < 3) and (yr - 1) or yr
    return ((y + math.floor(y/4) - math.floor(y/100) + math.floor(y/400) + t[mo] + 1) % 7) + 1
end

local function day_of_yr(yr, mo, dy)
    local d = dy
    for m = 1, mo - 1 do d = d + days_in_mo(yr, m) end
    return d
end

local function save_data()
    if smudge and smudge.write_file then
        smudge.write_file("dashboard.dat", string.format("%d;%d", is_24h and 1 or 0, theme_style))
    end
end

local function load_data()
    if not (smudge and smudge.read_file) then return end
    local s = smudge.read_file("dashboard.dat")
    if not s or s == "" then s = smudge.read_file("dashboard_v1.dat") end
    if s and s ~= "" then
        local p = {}
        for x in string.gmatch(s, "[^;]+") do table.insert(p, x) end
        if #p >= 2 then
            is_24h = (tonumber(p[1]) or 0) == 1
            theme_style = math.max(1, math.min(NUM_STYLES, tonumber(p[2]) or 1))
        end
    end
end

local function draw_digit(x, y, d, dw, dh, th, col)
    local n = tonumber(d) or 0
    local hh = math.floor(dh / 2)
    local thh = math.floor(th / 2)
    if n == 1 then
        local cx = x + math.floor((dw - th) / 2)
        smudge.rect(cx, y, th, dh, true, col)
        smudge.rect(cx - th + 2, y, th - 2, th + 2, true, col)
        smudge.rect(cx - th + 2, y + dh - th + 2, th * 2 - 2, th - 2, true, col)
        return
    end
    local s = SEGS[n + 1] or 0
    if (s & 1) ~= 0 then smudge.rect(((s & 2) ~= 0) and x or (x + th), y, dw - (((s & 2) == 0 and th or 0) + ((s & 4) == 0 and th or 0)), th, true, col) end
    if (s & 2) ~= 0 then smudge.rect(x, y, th, hh + thh, true, col) end
    if (s & 4) ~= 0 then smudge.rect(x + dw - th, y, th, hh + thh, true, col) end
    if (s & 8) ~= 0 then smudge.rect((((s & 2) ~= 0 or (s & 16) ~= 0)) and x or (x + th), y + hh - thh, dw - (((s & 2) == 0 and (s & 16) == 0 and th or 0) + ((s & 4) == 0 and (s & 32) == 0 and th or 0)), th, true, col) end
    if (s & 16) ~= 0 then smudge.rect(x, y + hh - thh, th, dh - hh + thh, true, col) end
    if (s & 32) ~= 0 then smudge.rect(x + dw - th, y + hh - thh, th, dh - hh + thh, true, col) end
    if (s & 64) ~= 0 then smudge.rect(((s & 16) ~= 0) and x or (x + th), y + dh - th, dw - (((s & 16) == 0 and th or 0) + ((s & 32) == 0 and th or 0)), th, true, col) end
end

local function draw_badge(x, y, str, is_dark)
    if not str or str == "" then return end
    local tag_w, tag_h = 42, 22
    smudge.rounded_rect(x, y, tag_w, tag_h, 4, true, not is_dark)
    smudge.text(x + math.floor(tag_w / 2), y + 3, str, 8, true, "center", is_dark)
end

local function draw_clock_cards(x, y, w, h, hour_str, min_str, ampm_str, is_dark, is_minimal)
    local fg = not is_dark
    local box_gap = 14
    local card_w = math.floor((w - box_gap) / 2)
    local dh = math.min(h - 32, 94)
    local dw = math.floor(dh * 0.55)
    local thick = math.max(6, math.floor(dw * 0.18))
    local d_gap = 12
    local digits_total_w = dw * 2 + d_gap
    local d_margin_x = math.floor((card_w - digits_total_w) / 2)
    local d_y = y + math.floor((h - dh) / 2)

    if not is_minimal then
        smudge.rounded_rect(x, y, card_w, h, 8, is_dark)
        smudge.rounded_rect(x, y, card_w, h, 8, false, fg)
    else
        smudge.rect(x, y, card_w, h, false, fg)
    end

    draw_digit(x + d_margin_x, d_y, string.sub(hour_str, 1, 1), dw, dh, thick, fg)
    draw_digit(x + d_margin_x + dw + d_gap, d_y, string.sub(hour_str, 2, 2), dw, dh, thick, fg)

    local colon_x = x + card_w + math.floor(box_gap / 2)
    local cy = y + math.floor(h / 2)
    smudge.circle(colon_x, cy - 14, 3, true, fg)
    smudge.circle(colon_x, cy + 14, 3, true, fg)

    local min_x = x + card_w + box_gap
    if not is_minimal then
        smudge.rounded_rect(min_x, y, card_w, h, 8, is_dark)
        smudge.rounded_rect(min_x, y, card_w, h, 8, false, fg)
    else
        smudge.rect(min_x, y, card_w, h, false, fg)
    end

    local min_margin = (ampm_str and ampm_str ~= "") and math.max(16, d_margin_x - 18) or d_margin_x
    draw_digit(min_x + min_margin, d_y, string.sub(min_str, 1, 1), dw, dh, thick, fg)
    draw_digit(min_x + min_margin + dw + d_gap, d_y, string.sub(min_str, 2, 2), dw, dh, thick, fg)

    if ampm_str and ampm_str ~= "" then
        draw_badge(min_x + card_w - 48, y + h - 28, ampm_str, is_dark)
    end
end

local function draw_bar(x, y, w, h, pct, is_dark)
    local fg = not is_dark
    smudge.rounded_rect(x, y, w, h, math.floor(h / 2), false, fg)
    local fill_w = math.max(0, math.min(w - 4, math.floor((w - 4) * (pct / 100))))
    if fill_w > 0 then
        smudge.rounded_rect(x + 2, y + 2, fill_w, h - 4, math.max(1, math.floor((h - 4) / 2)), true, fg)
    end
end

local function draw_calendar(cx, y, w, h, yr, mo, current_day, is_dark, is_minimal)
    local fg = not is_dark
    local left_x = cx - math.floor(w / 2)
    if not is_minimal then
        smudge.rounded_rect(left_x, y, w, h, 8, is_dark)
        smudge.rounded_rect(left_x, y, w, h, 8, false, fg)
    end

    smudge.text(cx, y + 10, string.format("%s %d", string.upper(MONTHS[mo] or ""), yr), 10, true, "center", fg)
    smudge.line(left_x + 10, y + 36, left_x + w - 10, y + 36, fg)

    local day_names = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"}
    local col_w = math.floor((w - 20) / 7)
    local grid_left = left_x + 10

    for i = 1, 7 do
        smudge.text(grid_left + (i - 1) * col_w + math.floor(col_w / 2), y + 42, day_names[i], 8, true, "center", fg)
    end
    smudge.line(left_x + 10, y + 62, left_x + w - 10, y + 62, fg)

    local start_col = first_wday(yr, mo)
    local num_days = days_in_mo(yr, mo)
    local row_h = math.floor((h - 76) / 6)
    local day_num = 1

    for row = 0, 5 do
        local row_top = y + 68 + (row * row_h)
        for col = 1, 7 do
            if (row == 0 and col < start_col) or day_num > num_days then
                -- empty
            else
                local cell_cx = grid_left + (col - 1) * col_w + math.floor(col_w / 2)
                if day_num == current_day then
                    local hl_w = math.min(col_w - 4, 30)
                    local hl_h = 22
                    local hl_x = cell_cx - math.floor(hl_w / 2)
                    local hl_y = row_top + math.floor((row_h - hl_h) / 2)
                    smudge.rounded_rect(hl_x, hl_y, hl_w, hl_h, 5, true, fg)
                    smudge.text(cell_cx, hl_y + 3, tostring(day_num), 8, true, "center", not fg)
                else
                    local text_y = row_top + math.floor((row_h - 14) / 2)
                    smudge.text(cell_cx, text_y, tostring(day_num), 10, false, "center", fg)
                end
                day_num = day_num + 1
            end
        end
    end
end

local function draw_year_stats(cx, y, w, h, yr, doy, total_days, is_dark)
    local fg = not is_dark
    local left_x = cx - math.floor(w / 2)
    smudge.rounded_rect(left_x, y, w, h, 8, is_dark)

    local col_w = math.floor(w / 3)
    local c1 = left_x + math.floor(col_w / 2)
    local c2 = left_x + col_w + math.floor(col_w / 2)
    local c3 = left_x + col_w * 2 + math.floor(col_w / 2)

    local yr_pct = math.floor((doy / total_days) * 100)
    smudge.text(c1, y + 10, "YEAR PROGRESS", 8, false, "center", fg)
    smudge.text(c1, y + 26, string.format("%d%%", yr_pct), 12, true, "center", fg)
    smudge.text(c1, y + 48, string.format("Day %d / %d", doy, total_days), 8, false, "center", fg)

    local wk = math.floor((doy - 1) / 7) + 1
    smudge.text(c2, y + 10, "WEEK", 8, false, "center", fg)
    smudge.text(c2, y + 26, string.format("W%02d", wk), 12, true, "center", fg)
    smudge.text(c2, y + 48, string.format("%d of 52", wk), 8, false, "center", fg)

    smudge.text(c3, y + 10, "COUNTDOWN", 8, false, "center", fg)
    smudge.text(c3, y + 26, tostring(total_days - doy), 12, true, "center", fg)
    smudge.text(c3, y + 48, "days left", 8, false, "center", fg)

    smudge.line(left_x + col_w, y + 8, left_x + col_w, y + h - 8, fg)
    smudge.line(left_x + col_w * 2, y + 8, left_x + col_w * 2, y + h - 8, fg)
end

local function draw_landscape_view(is_dark, dt, hour_str, min_str, ampm_str, doy, total_days, day_pct)
    local fg = not is_dark
    local left_x, card_w, right_x, top_y, card_h = 24, 364, 412, 36, 414

    if is_dark then smudge.rect(0, 0, 800, 480, true, true) end
    smudge.text(400, 10, "DESK STAND", 10, true, "center", fg)
    smudge.text(776, 10, is_dark and "Landscape Dark" or "Landscape Stand", 8, false, "right", fg)

    -- Left Card (Clock + Progress + Year Overview)
    smudge.rounded_rect(left_x, top_y, card_w, card_h, 8, is_dark)
    smudge.rounded_rect(left_x, top_y, card_w, card_h, 8, false, fg)

    local date_str = string.format("%s, %s %d", (DAYS[dt.wday] or "Today"):sub(1,3), (MONTHS[dt.month] or ""):sub(1,3), dt.day)
    smudge.text(left_x + 14, top_y + 10, date_str, 8, true, "left", fg)
    local batt_pct = (smudge.get_battery and smudge.get_battery()) or 100
    smudge.text(left_x + card_w - 14, top_y + 10, string.format("%s • %d%%", ampm_str, batt_pct), 8, true, "right", fg)
    smudge.line(left_x + 10, top_y + 30, left_x + card_w - 10, top_y + 30, fg)

    draw_clock_cards(left_x + 14, top_y + 38, card_w - 28, 116, hour_str, min_str, "", is_dark, false)
    smudge.line(left_x + 10, top_y + 160, left_x + card_w - 10, top_y + 160, fg)

    smudge.text(left_x + 14, top_y + 170, "DAY PROGRESS", 8, true, "left", fg)
    smudge.text(left_x + card_w - 14, top_y + 170, string.format("%d%%", day_pct), 8, true, "right", fg)
    draw_bar(left_x + 14, top_y + 194, card_w - 28, 8, day_pct, is_dark)
    smudge.line(left_x + 10, top_y + 210, left_x + card_w - 10, top_y + 210, fg)

    local yr_pct = math.floor((doy / total_days) * 100)
    local days_left = total_days - doy

    smudge.text(left_x + 14, top_y + 220, "YEAR PROGRESS", 8, true, "left", fg)
    smudge.text(left_x + card_w - 14, top_y + 220, string.format("%d%%", yr_pct), 8, true, "right", fg)
    draw_bar(left_x + 14, top_y + 244, card_w - 28, 8, yr_pct, is_dark)
    smudge.line(left_x + 10, top_y + 260, left_x + card_w - 10, top_y + 260, fg)

    local qtr = math.floor((dt.month - 1) / 3) + 1
    local wk = math.floor((doy - 1) / 7) + 1
    smudge.text(left_x + 14, top_y + 270, string.format("Day %d of %d", doy, total_days), 8, false, "left", fg)
    smudge.text(left_x + card_w - 14, top_y + 270, string.format("%d days left in %d", days_left, dt.year), 8, false, "right", fg)
    smudge.text(left_x + 14, top_y + 294, string.format("Quarter %d of 4", qtr), 8, false, "left", fg)
    smudge.text(left_x + card_w - 14, top_y + 294, string.format("Week %02d of 52", wk), 8, false, "right", fg)
    smudge.line(left_x + 10, top_y + 318, left_x + card_w - 10, top_y + 318, fg)

    local season = (dt.month >= 3 and dt.month <= 5) and "Spring" or (dt.month >= 6 and dt.month <= 8) and "Summer" or (dt.month >= 9 and dt.month <= 11) and "Autumn" or "Winter"
    smudge.text(left_x + math.floor(card_w / 2), top_y + 334, string.format("%s Station", season), 10, true, "center", fg)
    smudge.text(left_x + math.floor(card_w / 2), top_y + 366, "Ambient e-Ink Desk Stand", 8, false, "center", fg)

    -- Right Card (Calendar + Month Progress)
    smudge.rounded_rect(right_x, top_y, card_w, card_h, 8, is_dark)
    smudge.rounded_rect(right_x, top_y, card_w, card_h, 8, false, fg)

    smudge.text(right_x + math.floor(card_w / 2), top_y + 10, string.format("%s %d", string.upper(MONTHS[dt.month] or ""), dt.year), 10, true, "center", fg)
    smudge.line(right_x + 10, top_y + 40, right_x + card_w - 10, top_y + 40, fg)

    local day_names = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"}
    local col_w = math.floor((card_w - 20) / 7)
    local grid_left = right_x + 10
    for i = 1, 7 do
        smudge.text(grid_left + (i - 1) * col_w + math.floor(col_w / 2), top_y + 46, day_names[i], 8, true, "center", fg)
    end
    smudge.line(right_x + 10, top_y + 66, right_x + card_w - 10, top_y + 66, fg)

    local start_col = first_wday(dt.year, dt.month)
    local num_days = days_in_mo(dt.year, dt.month)
    local day_num = 1
    for row = 0, 5 do
        local row_top = top_y + 72 + (row * 24)
        for col = 1, 7 do
            if (row == 0 and col < start_col) or day_num > num_days then
                -- empty
            else
                local cx = grid_left + (col - 1) * col_w + math.floor(col_w / 2)
                if day_num == dt.day then
                    local hl_w, hl_h = math.min(col_w - 4, 30), 20
                    local hl_x = cx - math.floor(hl_w / 2)
                    local hl_y = row_top + 2
                    smudge.rounded_rect(hl_x, hl_y, hl_w, hl_h, 5, true, fg)
                    smudge.text(cx, hl_y + 3, tostring(day_num), 8, true, "center", not fg)
                else
                    smudge.text(cx, row_top + 4, tostring(day_num), 8, false, "center", fg)
                end
                day_num = day_num + 1
            end
        end
    end

    smudge.line(right_x + 10, top_y + 222, right_x + card_w - 10, top_y + 222, fg)

    local total_mo_days = days_in_mo(dt.year, dt.month)
    local mo_pct = math.floor((dt.day / total_mo_days) * 100)

    smudge.text(right_x + 14, top_y + 230, string.format("%s PROGRESS", string.upper(MONTHS[dt.month] or "")), 8, true, "left", fg)
    smudge.text(right_x + card_w - 14, top_y + 230, string.format("%d%%", mo_pct), 8, true, "right", fg)
    draw_bar(right_x + 14, top_y + 254, card_w - 28, 8, mo_pct, is_dark)
    smudge.line(right_x + 10, top_y + 270, right_x + card_w - 10, top_y + 270, fg)

    smudge.text(right_x + 14, top_y + 280, string.format("Day %d of %d in %s", dt.day, total_mo_days, MONTHS[dt.month] or ""), 8, false, "left", fg)
    smudge.text(right_x + card_w - 14, top_y + 280, string.format("%d days left", total_mo_days - dt.day), 8, false, "right", fg)
    smudge.text(right_x + 14, top_y + 304, string.format("Week %02d of 52", wk), 8, false, "left", fg)
    smudge.text(right_x + card_w - 14, top_y + 304, string.format("%d weeks remaining", 52 - wk), 8, false, "right", fg)
    smudge.line(right_x + 10, top_y + 328, right_x + card_w - 10, top_y + 328, fg)

    smudge.text(right_x + math.floor(card_w / 2), top_y + 344, string.format("%s Season (Month %d of 3)", season, ((dt.month - 1) % 3) + 1), 8, false, "center", fg)

    smudge.text(400, 460, "[Back] Exit    •    [OK] 12/24H    •    [<] Style -    •    [>] Style +", 8, false, "center", fg)
end

function on_init()
    if smudge.prevent_sleep then smudge.prevent_sleep(true) end
    load_data()
    local dt = smudge.get_date and smudge.get_date()
    if dt then last_min, last_hour = dt.min, dt.hour end
end

function on_exit()
    if smudge.prevent_sleep then smudge.prevent_sleep(false) end
    if smudge.set_orientation then smudge.set_orientation("portrait") end
    save_data()
end

function on_update()
    local dt = smudge.get_date and smudge.get_date()
    if not dt then return end
    if dt.min ~= last_min then
        last_min = dt.min
        if dt.hour ~= last_hour then
            last_hour = dt.hour
            needs_full_refresh = true
        end
        if smudge.request_update then smudge.request_update() end
    end
end

function on_draw()
    if theme_style >= 6 then
        if smudge.set_orientation then smudge.set_orientation("landscape") end
    else
        if smudge.set_orientation then smudge.set_orientation("portrait") end
    end

    smudge.clear()

    local w, h = 480, 800
    if smudge.get_bounds then w, h = smudge.get_bounds() end
    local cx = math.floor(w / 2)
    local card_x, card_w = 24, w - 48

    local dt = (smudge.get_date and smudge.get_date()) or {
        year = 2026, month = 10, day = 6, hour = 14, min = 45, wday = 3
    }
    local doy = day_of_yr(dt.year, dt.month, dt.day)
    local total_days = is_leap(dt.year) and 366 or 365
    local day_pct = math.floor(((dt.hour * 60 + dt.min) / 1440.0) * 100)

    local ampm_str = ""
    local disp_hr = dt.hour
    if not is_24h then
        ampm_str = (dt.hour >= 12) and "PM" or "AM"
        disp_hr = dt.hour % 12
        if disp_hr == 0 then disp_hr = 12 end
    else
        ampm_str = "24H"
    end
    local hour_str = string.format("%02d", disp_hr)
    local min_str = string.format("%02d", dt.min)

    local date_header = string.format("%s, %s %d", (DAYS[dt.wday] or "Today"):sub(1,3), (MONTHS[dt.month] or ""):sub(1,3), dt.day)

    if theme_style == 1 then
        smudge.header("Desk Stand", date_header)
        draw_clock_cards(card_x, 86, card_w, 130, hour_str, min_str, ampm_str, false, false)
        smudge.text(card_x, 226, "DAYLIGHT PROGRESS", 8, false, "left", true)
        smudge.text(card_x + card_w, 226, string.format("%d%%", day_pct), 8, false, "right", true)
        draw_bar(card_x, 252, card_w, 8, day_pct, false)
        draw_calendar(cx, 276, card_w, 324, dt.year, dt.month, dt.day, false, false)
        draw_year_stats(cx, 616, card_w, 92, dt.year, doy, total_days, false)

    elseif theme_style == 2 then
        smudge.header("Desk Stand", date_header)
        local big_w = math.floor((card_w - 18) / 2)
        local dh, dw = 104, 54
        local th = math.floor(dw * 0.18)
        local d_gap = 12
        local d_total = dw * 2 + d_gap
        local hour_margin = math.floor((big_w - d_total) / 2)
        local min_margin = (ampm_str and ampm_str ~= "") and math.max(16, hour_margin - 18) or hour_margin

        smudge.rounded_rect(card_x, 86, big_w, 130, 8, false)
        draw_digit(card_x + hour_margin, 96, string.sub(hour_str, 1, 1), dw, dh, th, true)
        draw_digit(card_x + hour_margin + dw + d_gap, 96, string.sub(hour_str, 2, 2), dw, dh, th, true)

        local min_bx = card_x + big_w + 18
        smudge.rounded_rect(min_bx, 86, big_w, 130, 8, false)
        draw_digit(min_bx + min_margin, 96, string.sub(min_str, 1, 1), dw, dh, th, true)
        draw_digit(min_bx + min_margin + dw + d_gap, 96, string.sub(min_str, 2, 2), dw, dh, th, true)

        if ampm_str and ampm_str ~= "" then
            draw_badge(min_bx + big_w - 48, 86 + 130 - 28, ampm_str, false)
        end

        local cy = 86 + 65
        smudge.circle(card_x + big_w + 9, cy - 16, 4, true, true)
        smudge.circle(card_x + big_w + 9, cy + 16, 4, true, true)

        smudge.text(card_x, 226, string.format("DAY %d OF %d", doy, total_days), 8, false, "left", true)
        smudge.text(card_x + card_w, 226, string.format("%d%% PASSED", day_pct), 8, false, "right", true)
        draw_bar(card_x, 252, card_w, 8, day_pct, false)
        draw_calendar(cx, 276, card_w, 422, dt.year, dt.month, dt.day, false, false)

    elseif theme_style == 3 then
        smudge.header("Desk Stand", "Calendar Planner")
        smudge.rounded_rect(card_x, 86, card_w, 50, 6, false)
        smudge.text(card_x + 14, 98, string.format("%s:%s %s", hour_str, min_str, ampm_str), 14, true, "left", true)
        smudge.text(card_x + card_w - 14, 102, string.format("%s, %s %d", DAYS[dt.wday] or "", MONTHS[dt.month] or "", dt.day), 10, true, "right", true)
        draw_calendar(cx, 146, card_w, 444, dt.year, dt.month, dt.day, false, false)

        local total_mo_days = days_in_mo(dt.year, dt.month)
        local mo_pct = math.floor((dt.day / total_mo_days) * 100)
        smudge.rounded_rect(card_x, 604, card_w, 108, 8, false)
        smudge.text(card_x + 14, 614, string.format("%s PROGRESS", string.upper(MONTHS[dt.month] or "")), 9, true, "left", true)
        smudge.text(card_x + card_w - 14, 614, string.format("%d%%", mo_pct), 9, true, "right", true)
        draw_bar(card_x + 14, 642, card_w - 28, 8, mo_pct, false)
        smudge.line(card_x + 12, 664, card_x + card_w - 12, 664, true)
        smudge.text(card_x + 14, 676, string.format("Day %d of %d in %s", dt.day, total_mo_days, MONTHS[dt.month] or ""), 8, false, "left", true)
        smudge.text(card_x + card_w - 14, 676, string.format("%d days left in month", total_mo_days - dt.day), 8, false, "right", true)

    elseif theme_style == 4 then
        smudge.rect(0, 0, w, h, true, true)
        smudge.text(cx, 16, "DESK STAND", 10, true, "center", false)
        smudge.line(card_x, 38, card_x + card_w, 38, false)
        local date_str = string.format("%s, %s %d, %d", DAYS[dt.wday] or "Today", MONTHS[dt.month] or "", dt.day, dt.year)
        smudge.text(card_x, 48, date_str, 10, true, "left", false)
        local batt_pct = (smudge.get_battery and smudge.get_battery()) or 100
        smudge.text(card_x + card_w, 48, string.format("%d%%", batt_pct), 10, true, "right", false)
        draw_clock_cards(card_x, 74, card_w, 130, hour_str, min_str, ampm_str, true, false)
        smudge.text(card_x, 218, "DAYLIGHT PROGRESS", 8, false, "left", false)
        smudge.text(card_x + card_w, 218, string.format("%d%%", day_pct), 8, false, "right", false)
        draw_bar(card_x, 244, card_w, 8, day_pct, true)
        draw_calendar(cx, 268, card_w, 330, dt.year, dt.month, dt.day, true, false)
        draw_year_stats(cx, 612, card_w, 94, dt.year, doy, total_days, true)

    elseif theme_style == 5 then
        smudge.header("Desk Stand", "Minimal Studio")
        local date_full = string.format("%s, %s %d, %d", (DAYS[dt.wday] or ""):upper(), (MONTHS[dt.month] or ""):upper(), dt.day, dt.year)
        smudge.text(cx, 96, date_full, 12, true, "center", true)
        draw_clock_cards(card_x, 130, card_w, 130, hour_str, min_str, ampm_str, false, true)

        smudge.line(card_x, 274, card_x + card_w, 274, true)
        smudge.text(card_x, 286, string.format("DAY %d / %d", doy, total_days), 8, true, "left", true)
        smudge.text(card_x + card_w, 286, string.format("%d%% PASSED", day_pct), 8, true, "right", true)
        draw_bar(card_x, 312, card_w, 8, day_pct, false)
        draw_calendar(cx, 334, card_w, 370, dt.year, dt.month, dt.day, false, true)

    elseif theme_style == 6 or theme_style == 7 then
        draw_landscape_view(theme_style == 7, dt, hour_str, min_str, ampm_str, doy, total_days, day_pct)
    end

    if theme_style <= 5 then
        local b2_label = is_24h and "12-Hour" or "24-Hour"
        smudge.button_hints("Exit", b2_label, "Style -", "Style +")
    end

    if needs_full_refresh then
        needs_full_refresh = false
        if smudge.full_refresh then smudge.full_refresh() end
    end
end

function on_button(btn, pressed)
    if not pressed then return end
    if btn == "btn1" or btn == "back" then
        if smudge.prevent_sleep then smudge.prevent_sleep(false) end
        if smudge.set_orientation then smudge.set_orientation("portrait") end
        smudge.exit()
    elseif btn == "btn2" or btn == "confirm" then
        is_24h = not is_24h
        save_data()
        if smudge.request_update then smudge.request_update() end
    elseif btn == "btn3" or btn == "left" or btn == "page_back" then
        theme_style = theme_style - 1
        if theme_style < 1 then theme_style = NUM_STYLES end
        save_data()
        if smudge.request_update then smudge.request_update() end
    elseif btn == "btn4" or btn == "right" or btn == "page_forward" then
        theme_style = theme_style + 1
        if theme_style > NUM_STYLES then theme_style = 1 end
        save_data()
        if smudge.request_update then smudge.request_update() end
    end
end

function on_touch(action, tx, ty)
    if action ~= "tap" and action ~= "click" then return end
    local w, h = 480, 800
    if smudge.get_bounds then w, h = smudge.get_bounds() end

    if theme_style >= 6 and w >= 800 then
        if tx < 400 then
            is_24h = not is_24h
            save_data()
            if smudge.request_update then smudge.request_update() end
        else
            theme_style = theme_style + 1
            if theme_style > NUM_STYLES then theme_style = 1 end
            save_data()
            if smudge.request_update then smudge.request_update() end
        end
        return
    end

    if ty <= 135 then
        is_24h = not is_24h
        save_data()
        if smudge.request_update then smudge.request_update() end
        return
    end
    if ty >= h - 70 and tx < math.floor(w / 2) then
        theme_style = theme_style - 1
        if theme_style < 1 then theme_style = NUM_STYLES end
        save_data()
        if smudge.request_update then smudge.request_update() end
        return
    end
    if ty >= h - 70 and tx >= math.floor(w / 2) then
        theme_style = theme_style + 1
        if theme_style > NUM_STYLES then theme_style = 1 end
        save_data()
        if smudge.request_update then smudge.request_update() end
        return
    end
    theme_style = theme_style + 1
    if theme_style > NUM_STYLES then theme_style = 1 end
    save_data()
    if smudge.request_update then smudge.request_update() end
end
