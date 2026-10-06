local w, h = 480, 800
if smudge.get_bounds then
    w, h = smudge.get_bounds()
end

local in_settings, show_history = false, false
local settings_sel = 1
local today_drank, daily_goal, step_amount = 0, 2000, 250
local units = "ml"
local last_date, last_date_short = "", ""
local streak = 0
local history = {}
local banner_msg, banner_expires = "", 0
local anim_tick, splash_anim = 0, 0

local function show_banner(msg, ms)
    banner_msg = msg
    local now = (smudge.millis and smudge.millis()) or (os.time() * 1000)
    banner_expires = now + (ms or 2200)
end

local UNITS = {"ml", "cups", "oz", "gallons", "L"}
local GOALS = {1000, 1500, 2000, 2500, 3000, 4000}
local STEPS = {100, 200, 250, 500}
local MONTHS = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"}

local function fmt(ml, show_unit)
    local u = units
    if u == "cups" then
        local c = ml / 250.0
        local s = (c == math.floor(c)) and string.format("%d", c) or string.format("%.1f", c)
        if show_unit then
            return s .. (c == 1 and " cup" or " cups")
        end
        return s
    elseif u == "oz" then
        local oz = math.floor((ml / 29.57) + 0.5)
        return show_unit and (oz .. " oz") or tostring(oz)
    elseif u == "gallons" then
        local g = ml / 3785.4
        local s = string.format("%.2f", g)
        return show_unit and (s .. " gal") or s
    elseif u == "L" then
        local l = ml / 1000.0
        local s = (l == math.floor(l)) and string.format("%d", l) or string.format("%.1f", l)
        return show_unit and (s .. " L") or s
    else
        return show_unit and (ml .. " ml") or tostring(ml)
    end
end

local function get_date()
    local t = (smudge.get_date and smudge.get_date()) or (smudge.date and smudge.date()) or os.date("*t")
    return string.format("%04d-%02d-%02d", t.year, t.month, t.day),
           string.format("%s %d", MONTHS[t.month] or "Oct", t.day)
end

local function check_rollover()
    local iso, short = get_date()
    if last_date == "" then
        last_date, last_date_short = iso, short
        return
    end
    if iso ~= last_date then
        local met = (today_drank >= daily_goal)
        table.insert(history, 1, {
            date = (last_date_short ~= "" and last_date_short or last_date),
            drank = today_drank,
            goal = daily_goal,
            met = met
        })
        while #history > 14 do
            table.remove(history)
        end
        streak = met and (streak + 1) or 0
        today_drank, last_date, last_date_short = 0, iso, short
        show_banner("New Day! Water Reset.", 2500)
    end
end

function save_data()
    smudge.write_file("water_state_v1.dat", string.format("%s;%d;%s;%d;%d;%d;%s",
        last_date, today_drank, units, daily_goal, step_amount, streak, last_date_short))
    local hp = {}
    for _, r in ipairs(history) do
        table.insert(hp, string.format("%s,%d,%d,%d", r.date, r.drank, r.goal, r.met and 1 or 0))
    end
    smudge.write_file("water_history_v1.dat", table.concat(hp, "|"))
end

local function load_data()
    local s = smudge.read_file("water_state_v1.dat")
    if s and s ~= "" then
        local p = {}
        for x in string.gmatch(s, "[^;]+") do table.insert(p, x) end
        if #p >= 6 then
            last_date = p[1]
            today_drank = tonumber(p[2]) or 0
            units = p[3] or "ml"
            daily_goal = tonumber(p[4]) or 2000
            step_amount = tonumber(p[5]) or 250
            streak = tonumber(p[6]) or 0
            last_date_short = p[7] or ""
        end
    end
    local hraw = smudge.read_file("water_history_v1.dat")
    if hraw and hraw ~= "" then
        history = {}
        for entry in string.gmatch(hraw, "[^|]+") do
            local f = {}
            for v in string.gmatch(entry, "[^,]+") do table.insert(f, v) end
            if #f >= 4 then
                table.insert(history, {
                    date = f[1],
                    drank = tonumber(f[2]) or 0,
                    goal = tonumber(f[3]) or 2000,
                    met = (tonumber(f[4]) == 1)
                })
            end
        end
    end
    check_rollover()
end

local function add_water(delta)
    local old = today_drank
    today_drank = math.max(0, today_drank + delta)
    splash_anim = 8
    if delta > 0 then
        if old < daily_goal and today_drank >= daily_goal then
            show_banner("Goal Reached! Fantastic!", 3000)
        else
            show_banner("+" .. fmt(delta, true) .. " logged!", 2200)
        end
    else
        show_banner("-" .. fmt(math.abs(delta), true) .. " removed", 2000)
    end
    save_data()
end

function on_init()
    if smudge.get_bounds then
        w, h = smudge.get_bounds()
    end
    load_data()
end

function on_update()
    anim_tick = anim_tick + 1
    if splash_anim > 0 then
        splash_anim = splash_anim - 1
    end
    if banner_msg ~= "" and banner_expires > 0 then
        local now = (smudge.millis and smudge.millis()) or (os.time() * 1000)
        if now >= banner_expires then
            banner_msg = ""
            banner_expires = 0
            if smudge.request_update then
                smudge.request_update()
            end
        end
    end
end

local function draw_btn(x, y, bw, bh, txt, sel, fid)
    smudge.rounded_rect(x, y, bw, bh, 6, sel)
    smudge.text(x + math.floor(bw / 2), y + math.floor((bh - 14) / 2), txt, fid or 10, true, "center", not sel)
end

local function draw_cup(cx, cy, pct)
    local ty, by = cy, cy + 220
    local ch = by - ty
    local tw, bw = 176, 136
    local ltx, rtx = cx - math.floor(tw / 2), cx + math.floor(tw / 2)
    local lbx, rbx = cx - math.floor(bw / 2), cx + math.floor(bw / 2)

    -- Straw (centered)
    local sx = cx - 7
    smudge.rect(sx, ty - 32, 14, 38, true, false)
    smudge.rect(sx, ty - 32, 14, 38, false)
    for s = ty - 28, ty - 4, 8 do
        smudge.thick_line(sx, s, sx + 13, s + 5, 2)
    end

    -- Water
    local max_h = 184
    local wh = math.floor(math.min(1.0, math.max(0.0, pct)) * max_h)
    local wty = by - 10 - wh
    if wh > 0 and smudge.rect_dither then
        local nb = math.max(1, math.floor(wh / 4))
        for b = 0, nb - 1 do
            local y = wty + (b * 4)
            if y < by - 8 then
                local cw = tw - math.floor((tw - bw) * ((y - ty) / ch))
                smudge.rect_dither(cx - math.floor(cw / 2) + 3, y, cw - 6, 4)
            end
        end
        -- Animated surface wave
        local sw = tw - math.floor((tw - bw) * ((wty - ty) / ch)) - 6
        local stx = cx - math.floor(sw / 2)
        local px, py = nil, nil
        for x = stx, stx + sw, 3 do
            local wy = wty + math.floor(math.sin((x / 10.0) + (anim_tick * 0.4)) * 2.5)
            if px then smudge.line(px, py, x, wy) end
            px, py = x, wy
        end
        -- Floating bubbles
        local b1 = by - 12 - math.floor(wh * 0.35)
        local b2 = by - 12 - math.floor(wh * 0.65)
        if b1 > wty + 8 and b1 < by - 14 then
            smudge.circle(cx - 32, b1, 3, true, false)
            smudge.circle(cx - 32, b1, 3, false)
        end
        if b2 > wty + 8 and b2 < by - 14 then
            smudge.circle(cx + 36, b2, 4, true, false)
            smudge.circle(cx + 36, b2, 4, false)
        end
    end

    -- Glass Outlines & Rim
    smudge.rounded_rect(ltx - 6, ty - 6, tw + 12, 16, 7, false)
    smudge.thick_line(ltx, ty + 10, lbx, by, 3)
    smudge.thick_line(rtx, ty + 10, rbx, by, 3)
    smudge.thick_line(lbx - 4, by, rbx + 4, by, 3)

    -- Measurement Ticks
    for _, tp in ipairs({0.25, 0.50, 0.75, 1.00}) do
        local y = by - 10 - math.floor(tp * max_h)
        local yr = (y - ty) / ch
        local lx = ltx + math.floor((lbx - ltx) * yr)
        local rx = rtx - math.floor((rtx - rbx) * yr)
        smudge.line(lx + 4, y, lx + 12, y)
        smudge.line(rx - 12, y, rx - 4, y)
    end

    -- Cute Expressive Face
    local fy = cy + 120
    if pct < 0.25 then
        -- Thirsty droopy expression
        smudge.circle(cx - 24, fy, 5, true)
        smudge.circle(cx + 24, fy, 5, true)
        smudge.thick_line(cx - 8, fy + 14, cx + 8, fy + 14, 2)
    elseif pct < 1.0 then
        -- Happy smiling expression with sparkle eyes
        smudge.circle(cx - 24, fy, 5, true)
        smudge.circle(cx + 24, fy, 5, true)
        smudge.circle(cx - 22, fy - 2, 2, true, false)
        smudge.circle(cx + 26, fy - 2, 2, true, false)
        smudge.thick_line(cx - 12, fy + 12, cx - 4, fy + 18, 2)
        smudge.thick_line(cx - 4, fy + 18, cx + 4, fy + 18, 2)
        smudge.thick_line(cx + 4, fy + 18, cx + 12, fy + 12, 2)
    else
        -- Goal Reached: wide happy eyes, open smiling mouth & blush dots!
        smudge.thick_line(cx - 26, fy + 4, cx - 18, fy - 2, 2)
        smudge.thick_line(cx - 18, fy - 2, cx - 10, fy + 4, 2)
        smudge.thick_line(cx + 10, fy + 4, cx + 18, fy - 2, 2)
        smudge.thick_line(cx + 18, fy - 2, cx + 26, fy + 4, 2)

        smudge.rounded_rect(cx - 15, fy + 10, 30, 16, 6, true, false)
        smudge.rounded_rect(cx - 15, fy + 10, 30, 16, 6, false, true)
        smudge.thick_line(cx - 14, fy + 10, cx + 14, fy + 10, 2)

        smudge.circle(cx - 36, fy + 8, 3, true, true)
        smudge.circle(cx + 36, fy + 8, 3, true, true)
    end

    -- Splash animation droplets
    if splash_anim > 0 then
        local o = 8 - splash_anim
        smudge.circle(cx - 45 - o, ty - 8 - o, 3, true)
        smudge.circle(cx + 48 + o, ty - 10 - o, 3, true)
    end
end

local function draw_today()
    local _, short_d = get_date()
    smudge.header("Water Tracker", short_d)
    local cx = math.floor(w / 2)
    local margin = 24
    local card_w = w - (margin * 2)
    local card_x = margin
    local pct = (daily_goal > 0) and (today_drank / daily_goal) or 0

    -- Top Goal / Banner Bar (safely below 84px header line)
    local by = 96
    if banner_msg ~= "" then
        smudge.rounded_rect(card_x, by, card_w, 36, 6, true)
        smudge.text(cx, by + 9, banner_msg, 12, true, "center", false)
    else
        smudge.rounded_rect(card_x, by, card_w, 36, 6, false)
        smudge.text(cx, by + 9, string.format("Hydration Goal: %d%% (%s / %s)",
            math.floor(pct * 100), fmt(today_drank, false), fmt(daily_goal, true)), 10, true, "center", true)
    end

    -- Centered Cup Artwork
    draw_cup(cx, 175, pct)

    -- Large Amount Display
    smudge.text(cx, 415, fmt(today_drank, true), 18, true, "center", true)

    -- Dashboard Card (y = 460..675)
    local card_y = 460
    local card_h = 215
    smudge.rounded_rect(card_x, card_y, card_w, card_h, 8, false)

    -- Row 1: Daily Goal & Streak
    local pad = 18
    local lx = card_x + pad
    local rx = card_x + card_w - pad
    smudge.text(lx, card_y + 16, "Daily Goal", 10, false, "left", true)
    smudge.text(lx, card_y + 38, fmt(daily_goal, true), 12, true, "left", true)

    smudge.text(rx, card_y + 16, "Streak", 10, false, "right", true)
    smudge.text(rx, card_y + 38, string.format("%d Day%s", streak, (streak == 1) and "" or "s"), 12, true, "right", true)

    -- Divider 1
    smudge.line(card_x + 14, card_y + 68, card_x + card_w - 14, card_y + 68)

    -- Row 2: Progress Bar
    smudge.text(lx, card_y + 78, "Today's Intake Progress", 10, false, "left", true)
    smudge.text(rx, card_y + 78, string.format("%d%%", math.floor(pct * 100)), 11, true, "right", true)

    local bar_x = lx
    local bar_w = card_w - (pad * 2)
    local bar_y, bar_h = card_y + 108, 16
    smudge.rounded_rect(bar_x, bar_y, bar_w, bar_h, 5, false)
    local bar_fill = math.max(0, math.min(bar_w - 4, math.floor(pct * (bar_w - 4))))
    if bar_fill > 0 then
        smudge.rounded_rect(bar_x + 2, bar_y + 2, bar_fill, bar_h - 4, 3, true)
    end

    -- Divider 2
    smudge.line(card_x + 14, card_y + 140, card_x + card_w - 14, card_y + 140)

    -- Row 3: Status Message & Hint
    local tip
    if pct >= 1.0 then
        tip = "Goal achieved! Excellent hydration today!"
    elseif pct >= 0.75 then
        tip = "Almost there! One more cup reaches goal."
    elseif pct >= 0.50 then
        tip = "Halfway there! Keep sipping regularly."
    elseif pct >= 0.25 then
        tip = "Good start! Regular sips boost energy."
    else
        tip = "Start your day with a fresh glass of water!"
    end
    smudge.text(cx, card_y + 154, tip, 10, true, "center", true)
    smudge.text(cx, card_y + 182, string.format("Tap/Press [+] to log +%s", fmt(step_amount, true)), 9, false, "center", true)

    -- Optional touch quick-add button (only on touch-enabled screens)
    if smudge.has_touch and smudge.has_touch() then
        draw_btn(cx - 90, 690, 180, 46, "+ " .. fmt(step_amount, true), true, 11)
    end

    smudge.button_hints("Exit", "Settings", "-" .. fmt(step_amount, true), "+" .. fmt(step_amount, true))
end

local function draw_history()
    smudge.header("Hydration History", "Past Days")
    local cx = math.floor(w / 2)
    local margin = 24
    local card_w = w - (margin * 2)
    local card_x = margin

    -- Top Summary Card (safely below 84px header line)
    smudge.rounded_rect(card_x, 98, card_w, 82, 8, false)
    local tp, gm = 0, 0
    for _, r in ipairs(history) do
        tp = tp + r.drank
        if r.met then gm = gm + 1 end
    end
    local avg = (#history > 0) and math.floor(tp / #history) or 0

    smudge.text(cx, 110, string.format("Streak: %d Day(s)   •   Goals Met: %d / %d",
        streak, gm, math.max(1, #history)), 11, true, "center", true)
    smudge.text(cx, 142, string.format("Average Daily Intake: %s", fmt(avg, true)), 10, false, "center", true)

    if #history == 0 then
        smudge.rounded_rect(card_x, 206, card_w, 220, 8, false)
        smudge.text(cx, 286, "No past days recorded yet.", 12, true, "center", true)
        smudge.text(cx, 320, "Daily hydration totals will be archived here", 10, false, "center", true)
        smudge.text(cx, 345, "automatically at midnight each day.", 10, false, "center", true)
    else
        local count = math.min(7, #history)
        local row_h = 64
        local row_gap = 8
        for i = 1, count do
            local r = history[i]
            local ry = 206 + (i - 1) * (row_h + row_gap)

            -- Card outline
            smudge.rounded_rect(card_x, ry, card_w, row_h, 6, false)

            -- Left: Date & Intake
            local pad = 16
            local lx = card_x + pad
            smudge.text(lx, ry + 12, r.date, 12, true, "left", true)
            smudge.text(lx, ry + 36, string.format("%s / %s", fmt(r.drank, false), fmt(r.goal, true)), 10, false, "left", true)

            -- Right: Status Badge
            local badge_w, badge_h = 96, 32
            local badge_x = card_x + card_w - pad - badge_w
            local badge_y = ry + 16

            -- Middle: Progress Bar
            local fr = math.min(1.0, r.drank / math.max(1, r.goal))
            local bar_x = lx + 130
            local bar_w = math.max(60, (badge_x - 16) - bar_x)
            local bar_h = 16
            smudge.rounded_rect(bar_x, ry + 24, bar_w, bar_h, 4, false)
            local fp = math.floor(fr * (bar_w - 4))
            if fp > 0 then
                smudge.rounded_rect(bar_x + 2, ry + 26, fp, bar_h - 4, 3, true)
            end

            if r.met then
                smudge.rounded_rect(badge_x, badge_y, badge_w, badge_h, 6, true)
                smudge.text(badge_x + math.floor(badge_w / 2), badge_y + 8, "MET", 10, true, "center", false)
            else
                smudge.rounded_rect(badge_x, badge_y, badge_w, badge_h, 6, false)
                smudge.text(badge_x + math.floor(badge_w / 2), badge_y + 8, string.format("%d%%", math.floor(fr * 100)), 10, true, "center", true)
            end
        end
    end

    smudge.button_hints("Back", "Settings", nil, nil)
end

local function draw_settings()
    smudge.header("Water Settings", "Preferences")
    local cx = math.floor(w / 2)
    local margin = 24
    local card_w = w - (margin * 2)
    local card_x = margin

    local items = {
        {"Measurement Unit", "Units across app", "< " .. string.upper(units) .. " >"},
        {"Daily Goal", "Target volume per day", "< " .. fmt(daily_goal, true) .. " >"},
        {"Increment Step", "Step per press", "< +" .. fmt(step_amount, true) .. " >"},
        {"Reset Today", "Reset logged count to 0", "[ Reset ]"},
        {"Past Days", "View 14-day history", "[ View > ]"}
    }

    local start_y = 96
    local row_h = 68
    local row_gap = 8

    for i, it in ipairs(items) do
        local ry = start_y + (i - 1) * (row_h + row_gap)
        local sel = (settings_sel == i)

        -- Background card
        smudge.rounded_rect(card_x, ry, card_w, row_h, 8, sel)

        -- Left column: Title and Subtitle
        local pad = 18
        smudge.text(card_x + pad, ry + 14, it[1], 12, true, "left", not sel)
        smudge.text(card_x + pad, ry + 38, it[2], 9, false, "left", not sel)

        -- Right column: Value Badge
        smudge.text(card_x + card_w - pad, ry + 25, it[3], 12, true, "right", not sel)
    end

    -- Clean Instructions Card
    local infoy = start_y + 5 * (row_h + row_gap) + 12
    smudge.rounded_rect(card_x, infoy, card_w, 215, 8, false)
    smudge.text(cx, infoy + 12, "Hardware Button Controls", 10, true, "center", true)
    smudge.line(card_x + 14, infoy + 42, card_x + card_w - 14, infoy + 42)

    local lx = card_x + 20
    smudge.text(lx, infoy + 52, "• Left / Right (Buttons 3 & 4):", 10, true, "left", true)
    smudge.text(lx + 20, infoy + 74, "Move selection Up / Down", 10, false, "left", true)

    smudge.text(lx, infoy + 102, "• Up / Down (Side Buttons):", 10, true, "left", true)
    smudge.text(lx + 20, infoy + 124, "Adjust unit, goal, or step amount", 10, false, "left", true)

    smudge.text(lx, infoy + 152, "• Confirm / Select (Button 2):", 10, true, "left", true)
    smudge.text(lx + 20, infoy + 174, "Activate row or open history", 10, false, "left", true)

    smudge.button_hints("Back", "Select", "Up", "Down")
end

function on_draw()
    if smudge.get_bounds then
        w, h = smudge.get_bounds()
    end
    smudge.clear()
    if show_history then
        draw_history()
    elseif in_settings then
        draw_settings()
    else
        draw_today()
    end
end

local function cycle_u(dir)
    local idx = 1
    for i, u in ipairs(UNITS) do
        if u == units then idx = i break end
    end
    idx = idx + (dir or 1)
    if idx > #UNITS then idx = 1 elseif idx < 1 then idx = #UNITS end
    units = UNITS[idx]
    save_data()
end

local function cycle_g(dir)
    local ci, md = 1, 999999
    for i, g in ipairs(GOALS) do
        local d = math.abs(g - daily_goal)
        if d < md then md, ci = d, i end
    end
    ci = ci + (dir or 1)
    if ci > #GOALS then ci = 1 elseif ci < 1 then ci = #GOALS end
    daily_goal = GOALS[ci]
    save_data()
end

local function cycle_s(dir)
    local ci, md = 1, 999999
    for i, s in ipairs(STEPS) do
        local d = math.abs(s - step_amount)
        if d < md then md, ci = d, i end
    end
    ci = ci + (dir or 1)
    if ci > #STEPS then ci = 1 elseif ci < 1 then ci = #STEPS end
    step_amount = STEPS[ci]
    save_data()
end

function on_button(btn)
    if show_history then
        if btn == "btn1" or btn == "btn2" or btn == "back" or btn == "confirm" then
            show_history, in_settings = false, true
        end
        return
    end

    if in_settings then
        if btn == "btn1" or btn == "back" then
            in_settings = false
            return
        elseif btn == "btn3" or btn == "left" then
            -- Left button moves UP in menu
            settings_sel = (settings_sel == 1) and 5 or (settings_sel - 1)
            return
        elseif btn == "btn4" or btn == "right" then
            -- Right button moves DOWN in menu
            settings_sel = (settings_sel == 5) and 1 or (settings_sel + 1)
            return
        elseif btn == "up" or btn == "page_back" then
            -- Up button / page back cycles value left
            if settings_sel == 1 then cycle_u(-1)
            elseif settings_sel == 2 then cycle_g(-1)
            elseif settings_sel == 3 then cycle_s(-1) end
            return
        elseif btn == "down" or btn == "page_forward" then
            -- Down button / page forward cycles value right
            if settings_sel == 1 then cycle_u(1)
            elseif settings_sel == 2 then cycle_g(1)
            elseif settings_sel == 3 then cycle_s(1) end
            return
        elseif btn == "btn2" or btn == "confirm" then
            -- Confirm button / select: cycles value or triggers action
            if settings_sel == 1 then cycle_u(1)
            elseif settings_sel == 2 then cycle_g(1)
            elseif settings_sel == 3 then cycle_s(1)
            elseif settings_sel == 4 then
                today_drank = 0
                show_banner("Today Water Reset", 2200)
                save_data()
                in_settings = false
            elseif settings_sel == 5 then
                show_history = true
            end
            return
        end
        return
    end

    -- Today screen buttons
    if btn == "btn1" or btn == "back" then
        smudge.exit()
    elseif btn == "btn2" or btn == "confirm" then
        in_settings = true
    elseif btn == "btn3" or btn == "left" or btn == "page_back" then
        add_water(-step_amount)
    elseif btn == "btn4" or btn == "right" or btn == "page_forward" then
        add_water(step_amount)
    elseif btn == "up" then
        add_water(-step_amount)
    elseif btn == "down" then
        add_water(step_amount)
    end
end

function on_touch(action, tx, ty)
    if action ~= "tap" and action ~= "click" then return end
    if show_history then
        if ty > 700 or ty < 80 then
            show_history, in_settings = false, true
        end
        return
    end

    if in_settings then
        if ty < 80 then
            in_settings = false
            return
        end
        local start_y = 96
        local row_h = 68
        local row_gap = 8
        for i = 1, 5 do
            local ry = start_y + (i - 1) * (row_h + row_gap)
            if ty >= ry and ty < ry + row_h then
                settings_sel = i
                if i == 1 then cycle_u(1)
                elseif i == 2 then cycle_g(1)
                elseif i == 3 then cycle_s(1)
                elseif i == 4 then
                    today_drank = 0
                    show_banner("Today Water Reset", 2200)
                    save_data()
                    in_settings = false
                elseif i == 5 then
                    show_history = true
                end
                return
            end
        end
        return
    end

    -- Today screen touch zones
    local cx = math.floor(w / 2)
    if ty >= 120 and ty <= 390 and math.abs(tx - cx) <= 120 then
        add_water(step_amount)
        return
    end
    if smudge.has_touch and smudge.has_touch() and ty >= 670 and ty <= 740 and math.abs(tx - cx) <= 100 then
        add_water(step_amount)
        return
    end
    if ty <= 80 and tx >= (w - 120) then
        in_settings = true
        return
    end
end
