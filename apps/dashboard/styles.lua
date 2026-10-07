-- Desk Stand Styles & Layouts
-- Restored exact published layout logic from v1.6.5.3

function draw_calendar(cx, y, w, h, yr, mo, current_day, is_dark, is_minimal)
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

function draw_year_stats(cx, y, w, h, yr, doy, total_days, is_dark)
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

function draw_landscape_view(is_dark, dt, hour_str, min_str, ampm_str, doy, total_days, day_pct)
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
end
