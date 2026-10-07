local w, h = 480, 800
if smudge.get_bounds then
    w, h = smudge.get_bounds()
end

-- Increments and configuration
local STEP_OPTIONS = {
    5, 10, 15, 20, 30, 45, 60, 90, 120, 180, 300, 600, 900, 1200, 1800, 3600
}

local step_top_sec = 300 -- default 5 min
local step_bot_sec = 60  -- default 1 min
local total_sec = 15 * 60 -- default 15 min
local remaining_sec = total_sec

local has_started = false
local is_running = false
local is_alarm = false
local flash_state = false
local flash_timer = 0
local last_flash_ms = 0
local last_tick_ms = 0
local last_rendered_sec = -1
local anim_tick = 0

local in_settings = false
local setting_row = 1 -- 1: Top/Bottom, 2: Bottom -/+
local confirm_held_start = nil
local confirm_suppress_release = false

local banner_msg = ""
local banner_expires = 0

local function show_banner(msg, ms)
    banner_msg = msg
    local now = (smudge.millis and smudge.millis()) or (os.time() * 1000)
    banner_expires = now + (ms or 1500)
end

local function format_step(sec)
    if sec < 60 then
        return string.format("%d sec", sec)
    elseif sec < 3600 then
        local m = math.floor(sec / 60)
        local s = sec % 60
        if s > 0 then
            return string.format("%dm %ds", m, s)
        else
            return string.format("%d min", m)
        end
    else
        local h = math.floor(sec / 3600)
        local m = math.floor((sec % 3600) / 60)
        if m > 0 then
            return string.format("%dh %dm", h, m)
        else
            return string.format("%d hr", h)
        end
    end
end

local function format_time(sec)
    local s = math.max(0, math.floor(sec))
    local h = math.floor(s / 3600)
    local m = math.floor((s % 3600) / 60)
    local rem_s = s % 60
    if h > 0 then
        return string.format("%02d:%02d:%02d", h, m, rem_s)
    else
        return string.format("%02d:%02d", m, rem_s)
    end
end

local function save_data()
    smudge.write_file("hourglass.dat", string.format("%d;%d;%d",
        math.floor(total_sec), math.floor(step_top_sec), math.floor(step_bot_sec)))
end

local function load_data()
    local s = smudge.read_file("hourglass.dat")
    if s and s ~= "" then
        local p = {}
        for x in string.gmatch(s, "[^;]+") do table.insert(p, x) end
        if #p >= 3 then
            local saved_total = tonumber(p[1]) or (15 * 60)
            local saved_top = tonumber(p[2]) or 300
            local saved_bot = tonumber(p[3]) or 60
            if saved_total and saved_total >= 1 then total_sec = saved_total end
            if saved_top and saved_top >= 1 then step_top_sec = saved_top end
            if saved_bot and saved_bot >= 1 then step_bot_sec = saved_bot end
            remaining_sec = total_sec
        end
    end
end

function on_init()
    if smudge and smudge.prevent_sleep then
        smudge.prevent_sleep(true)
    end
    if smudge.get_bounds then
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

local function step_index(current_sec)
    local best_i = 1
    local min_diff = math.abs(STEP_OPTIONS[1] - current_sec)
    for i = 2, #STEP_OPTIONS do
        local diff = math.abs(STEP_OPTIONS[i] - current_sec)
        if diff < min_diff then
            min_diff = diff
            best_i = i
        end
    end
    return best_i
end

local function adjust_step(row, dir)
    local cur = (row == 1) and step_top_sec or step_bot_sec
    local idx = step_index(cur)
    idx = math.max(1, math.min(#STEP_OPTIONS, idx + dir))
    if row == 1 then
        step_top_sec = STEP_OPTIONS[idx]
    else
        step_bot_sec = STEP_OPTIONS[idx]
    end
    save_data()
    if smudge.request_update then smudge.request_update() end
end

local function dismiss_alarm()
    is_alarm = false
    is_running = false
    has_started = false
    remaining_sec = total_sec
    flash_state = false
    show_banner("Ready", 1200)
    if smudge.request_update then smudge.request_update() end
end

local function reset_timer()
    is_running = false
    has_started = false
    remaining_sec = total_sec
    last_rendered_sec = -1
    show_banner("Timer Reset", 1200)
    if smudge.request_update then smudge.request_update() end
end

local function toggle_start_pause()
    if is_alarm then
        dismiss_alarm()
        return
    end
    if not has_started then
        has_started = true
        is_running = true
        last_tick_ms = (smudge.millis and smudge.millis()) or (os.time() * 1000)
        show_banner("Timer Started", 1200)
    else
        is_running = not is_running
        if is_running then
            last_tick_ms = (smudge.millis and smudge.millis()) or (os.time() * 1000)
            show_banner("Timer Resumed", 1200)
        else
            show_banner("Timer Paused", 1200)
        end
    end
    if smudge.request_update then smudge.request_update() end
end

local function adjust_duration(amount)
    if is_running or has_started or is_alarm then return end
    total_sec = math.max(5, math.min(86400, total_sec + amount))
    remaining_sec = total_sec
    last_rendered_sec = -1
    save_data()
    if smudge.request_update then smudge.request_update() end
end

function on_update()
    anim_tick = anim_tick + 1
    local now = (smudge.millis and smudge.millis()) or (os.time() * 1000)

    -- Auto-clear banner
    if banner_msg ~= "" and banner_expires > 0 then
        if now >= banner_expires then
            banner_msg = ""
            banner_expires = 0
            if smudge.request_update then smudge.request_update() end
        end
    end

    -- Hold Start for Settings
    if smudge.is_button_down and (smudge.is_button_down("confirm") or smudge.is_button_down("btn2")) then
        if not confirm_held_start and not in_settings and not is_alarm then
            confirm_held_start = now
        elseif confirm_held_start and (now - confirm_held_start) >= 500 and not in_settings and not is_alarm then
            in_settings = true
            confirm_suppress_release = true
            confirm_held_start = nil
            if smudge.request_update then smudge.request_update() end
        end
    else
        confirm_held_start = nil
    end

    -- Alarm Flashing state (toggles screen inversion every 500ms)
    if is_alarm then
        if not last_flash_ms then last_flash_ms = now end
        if (now - last_flash_ms) >= 500 then
            last_flash_ms = now
            flash_state = not flash_state
            if smudge.request_update then smudge.request_update() end
        end
        return
    end

    -- Timer countdown execution
    if is_running then
        local delta_ms = now - last_tick_ms
        if delta_ms >= 200 then
            last_tick_ms = now
            remaining_sec = math.max(0, remaining_sec - (delta_ms / 1000.0))

            if remaining_sec <= 0 then
                remaining_sec = 0
                is_running = false
                is_alarm = true
                last_flash_ms = now
                flash_state = true
                show_banner("TIME'S UP!", 5000)
                if smudge.full_refresh then smudge.full_refresh() end
                if smudge.request_update then smudge.request_update() end
            else
                local cur_sec = math.floor(remaining_sec)
                if cur_sec ~= last_rendered_sec then
                    last_rendered_sec = cur_sec
                    if smudge.request_update then smudge.request_update() end
                end
            end
        end
    end
end

-- ----------------------------------------------------------------------------
-- Drawing Routines
-- ----------------------------------------------------------------------------

local function draw_hourglass(cx, cy, pct)
    -- Hourglass geometry (lowered to cy = 155, height = 250px)
    local top_y = cy
    local hg_h = 250
    local bot_y = cy + hg_h
    local mid_y = cy + math.floor(hg_h / 2)
    local max_w = 176
    local half_w = math.floor(max_w / 2)
    local neck_w = 22
    local half_n = math.floor(neck_w / 2)

    -- 1. Wooden Top Pedestal
    smudge.rounded_rect(cx - half_w - 18, top_y - 14, max_w + 36, 14, 6, true)
    smudge.rounded_rect(cx - half_w - 10, top_y - 24, max_w + 20, 10, 4, false)
    smudge.circle(cx - half_w - 6, top_y - 7, 2, true, false)
    smudge.circle(cx + half_w + 6, top_y - 7, 2, true, false)

    -- 2. Wooden Bottom Pedestal
    smudge.rounded_rect(cx - half_w - 18, bot_y, max_w + 36, 14, 6, true)
    smudge.rounded_rect(cx - half_w - 10, bot_y + 14, max_w + 20, 10, 4, false)
    smudge.circle(cx - half_w - 6, bot_y + 7, 2, true, false)
    smudge.circle(cx + half_w + 6, bot_y + 7, 2, true, false)

    -- 3. Side Support Pillars
    local pl_x = cx - half_w - 10
    local pr_x = cx + half_w + 10
    smudge.thick_line(pl_x, top_y, pl_x, bot_y, 4)
    smudge.thick_line(pr_x, top_y, pr_x, bot_y, 4)
    smudge.thick_line(pl_x - 3, mid_y - 20, pl_x + 3, mid_y - 20, 2)
    smudge.thick_line(pl_x - 3, mid_y + 20, pl_x + 3, mid_y + 20, 2)
    smudge.thick_line(pr_x - 3, mid_y - 20, pr_x + 3, mid_y - 20, 2)
    smudge.thick_line(pr_x - 3, mid_y + 20, pr_x + 3, mid_y + 20, 2)

    -- Glass Contours (organic cosine curve with slender waist)
    local neck_h = 10
    local neck_top = mid_y - math.floor(neck_h / 2)
    local neck_bot = mid_y + math.floor(neck_h / 2)
    local bulb_top_h = neck_top - top_y
    local bulb_bot_h = bot_y - neck_bot

    -- Draw glass walls
    local prev_ly, prev_lx, prev_rx = top_y, cx - half_w, cx + half_w
    for y = top_y + 2, neck_top, 2 do
        local t = (y - top_y) / bulb_top_h
        local w_y = math.floor(half_n + (half_w - half_n) * (0.5 * (1.0 + math.cos(t * math.pi))))
        local lx = cx - w_y
        local rx = cx + w_y
        smudge.thick_line(prev_lx, prev_ly, lx, y, 2)
        smudge.thick_line(prev_rx, prev_ly, rx, y, 2)
        prev_ly, prev_lx, prev_rx = y, lx, rx
    end
    -- Neck straight section
    smudge.thick_line(cx - half_n, neck_top, cx - half_n, neck_bot, 2)
    smudge.thick_line(cx + half_n, neck_top, cx + half_n, neck_bot, 2)
    prev_ly, prev_lx, prev_rx = neck_bot, cx - half_n, cx + half_n
    for y = neck_bot + 2, bot_y, 2 do
        local t = (y - neck_bot) / bulb_bot_h
        local w_y = math.floor(half_n + (half_w - half_n) * (0.5 * (1.0 - math.cos(t * math.pi))))
        local lx = cx - w_y
        local rx = cx + w_y
        smudge.thick_line(prev_lx, prev_ly, lx, y, 2)
        smudge.thick_line(prev_rx, prev_ly, rx, y, 2)
        prev_ly, prev_lx, prev_rx = y, lx, rx
    end

    -- 4. Sand Rendering
    -- Top bulb sand: funnels inward, flows through neck channel, and tapers to nozzle tip.
    -- pct = remaining fraction (1.0 = full top, 0.0 = full bottom)
    local nozzle_h = 6
    local sand_max_y = neck_bot + nozzle_h
    if pct > 0.005 and smudge.rect_dither then
        local max_dip = is_running and 22 or 14
        local dip_h = math.floor(max_dip * math.sin(pct * math.pi))
        if pct > 0.05 and dip_h < 6 then
            dip_h = 6
        end
        local sand_edge_y = math.floor(top_y + 4 + (1.0 - pct) * (bulb_top_h - 8))
        local sand_center_y = math.min(sand_max_y - 2, sand_edge_y + dip_h)

        for y = sand_edge_y, sand_max_y, 2 do
            local w_y = 0
            if y < neck_top then
                local t = (y - top_y) / bulb_top_h
                w_y = math.floor(half_n + (half_w - half_n) * (0.5 * (1.0 + math.cos(t * math.pi)))) - 4
            elseif y <= neck_bot then
                w_y = half_n - 3
            else
                local prog = (y - neck_bot) / nozzle_h
                w_y = math.max(1, math.floor((half_n - 3) * (1.0 - prog)))
            end

            if w_y > 0 then
                if y < sand_center_y and dip_h > 0 then
                    -- Funnel region: parabolic crater sloping down toward neck center
                    local prog = (y - sand_edge_y) / math.max(1, (sand_center_y - sand_edge_y))
                    local crater_w = math.floor(w_y * (1.0 - (prog ^ 0.75)))
                    local wing_w = w_y - crater_w
                    if wing_w > 1 then
                        smudge.rect_dither(cx - w_y, y, wing_w, 2)
                        smudge.rect_dither(cx + crater_w, y, wing_w, 2)
                    end
                else
                    -- Full width sand below crater vertex
                    smudge.rect_dither(cx - w_y, y, w_y * 2, 2)
                end
            end
        end

        -- Animated falling sand particles along the crater slopes when running
        if is_running and dip_h > 4 and pct > 0.05 then
            for i = 0, 2 do
                local p_prog = ((anim_tick * 0.2 + i * 0.33) % 1.0)
                local py = math.floor(sand_edge_y + p_prog * (sand_center_y - sand_edge_y))
                if py >= sand_edge_y and py < sand_max_y - 2 then
                    local pw = half_n - 3
                    if py < neck_top then
                        local pt = (py - top_y) / bulb_top_h
                        pw = math.floor(half_n + (half_w - half_n) * (0.5 * (1.0 + math.cos(pt * math.pi)))) - 4
                    end
                    local p_prog2 = (py - sand_edge_y) / math.max(1, (sand_center_y - sand_edge_y))
                    local cw = math.floor(pw * (1.0 - (p_prog2 ^ 0.75)))
                    if i % 2 == 0 then
                        smudge.rect(cx - cw - 1, py, 2, 2, true)
                    else
                        smudge.rect(cx + cw - 1, py, 2, 2, true)
                    end
                end
            end
        end
    end

    -- Bottom bulb sand: fills from bot_y - 4 upwards
    local bot_fill_h = math.floor((1.0 - pct) * (bulb_bot_h - 6))
    local bot_sand_y = bot_y - 4 - bot_fill_h
    local mound_h = 0
    if bot_fill_h > 2 and smudge.rect_dither then
        for y = bot_sand_y, bot_y - 4, 2 do
            local t = (y - neck_bot) / bulb_bot_h
            local w_y = math.floor(half_n + (half_w - half_n) * (0.5 * (1.0 - math.cos(t * math.pi)))) - 4
            if w_y > 2 then
                smudge.rect_dither(cx - w_y, y, w_y * 2, 2)
            end
        end
        -- Dynamic conical sand mound where falling stream lands
        if pct > 0.01 and pct < 0.99 then
            mound_h = math.min(12, math.floor(6 + bot_fill_h * 0.15))
            for my = bot_sand_y - mound_h, bot_sand_y - 1, 2 do
                local prog = (bot_sand_y - my) / mound_h
                local mw = math.floor(math.min(26, half_n + bot_fill_h * 0.4) * (1.0 - prog))
                local max_mw = half_n - 3
                if my >= neck_bot then
                    local t_m = (my - neck_bot) / bulb_bot_h
                    max_mw = math.floor(half_n + (half_w - half_n) * (0.5 * (1.0 - math.cos(t_m * math.pi)))) - 4
                end
                if mw > max_mw then mw = max_mw end
                if mw > 1 then
                    smudge.rect_dither(cx - mw, my, mw * 2, 2)
                end
            end

            -- Animated splash / bouncing sand grains at mound apex when running
            if is_running and pct > 0.02 then
                local apex_y = bot_sand_y - mound_h
                local splash_phase = anim_tick % 3
                if splash_phase == 0 then
                    smudge.rect(cx - 3, apex_y - 2, 2, 2, true)
                elseif splash_phase == 1 then
                    smudge.rect(cx + 2, apex_y - 1, 2, 2, true)
                else
                    smudge.rect(cx - 2, apex_y - 3, 2, 2, true)
                    smudge.rect(cx + 3, apex_y - 2, 1, 1, true)
                end
            end
        end
    end

    -- Falling sand stream through neck (active when running, frozen when paused)
    if has_started and pct > 0.005 then
        local stream_start_y = sand_max_y
        local stream_end_y = math.max(neck_bot, bot_sand_y - mound_h)
        if stream_end_y >= stream_start_y then
            for y = stream_start_y, stream_end_y, 4 do
                local stream_w = 1
                if is_running then
                    stream_w = ((y + anim_tick * 2) % 6 < 3) and 2 or 1
                else
                    stream_w = (y % 6 < 3) and 2 or 1
                end
                smudge.rect(cx - math.floor(stream_w / 2), y, stream_w, 3, true)
            end
        end
    end

    -- 5. Cute Face & Eyes in Top Bulb (Water cup style)
    local eye_y = top_y + 46

    if is_alarm then
        -- Celebratory / happy expression (> <)
        for _, ex in ipairs({cx - 24, cx + 24}) do
            smudge.thick_line(ex - 7, eye_y - 4, ex, eye_y, 2)
            smudge.thick_line(ex - 7, eye_y + 4, ex, eye_y, 2)
        end
        -- Wide happy open mouth
        smudge.circle(cx, eye_y + 14, 5, true, true)
        smudge.circle(cx, eye_y + 14, 5, false, false)
    else
        -- Water Cup style adorable face:
        -- White backing so eyes pop cleanly over sand
        smudge.circle(cx - 24, eye_y, 7, true, false)
        smudge.circle(cx + 24, eye_y, 7, true, false)

        -- Solid black circles radius 5
        smudge.circle(cx - 24, eye_y, 5, true, true)
        smudge.circle(cx + 24, eye_y, 5, true, true)

        -- Tiny white sparkle radius 2 in upper-right
        smudge.circle(cx - 22, eye_y - 2, 2, true, false)
        smudge.circle(cx + 26, eye_y - 2, 2, true, false)

        -- Water Cup cheerful wide smile
        smudge.thick_line(cx - 12, eye_y + 12, cx - 4, eye_y + 18, 2)
        smudge.thick_line(cx - 4, eye_y + 18, cx + 4, eye_y + 18, 2)
        smudge.thick_line(cx + 4, eye_y + 18, cx + 12, eye_y + 12, 2)
    end

    -- Glass reflection highlights
    smudge.thick_line(cx - half_w + 10, top_y + 12, cx - half_w + 18, top_y + 36, 2)
    smudge.thick_line(cx + half_w - 10, bot_y - 12, cx + half_w - 18, bot_y - 36, 2)
end

local function draw_settings_dialog(cx, w, h)
    local mw = 430
    local mh = 300
    local mx = math.floor((w - mw) / 2)
    local my = 195

    -- Background shadow and modal frame
    smudge.rounded_rect(mx - 2, my - 2, mw + 4, mh + 4, 12, true, true)
    smudge.rounded_rect(mx, my, mw, mh, 10, true, false)
    smudge.rounded_rect(mx, my, mw, mh, 10, false, true)

    -- Title
    smudge.text(cx, my + 12, "Adjust Increments", 12, true, "center", true)
    smudge.line(mx + 16, my + 42, mx + mw - 16, my + 42)

    local box_w = 260
    local box_h = 36
    local box_x = cx - math.floor(box_w / 2)

    -- Row 1: Top / Bottom Hardware Buttons
    local r1_y = my + 52
    smudge.text(mx + 24, r1_y, "Top / Bottom Buttons (hardware):", 10, true, "left", true)

    local box1_y = r1_y + 28
    if setting_row == 1 then
        smudge.rounded_rect(box_x - 2, box1_y - 2, box_w + 4, box_h + 4, 6, true, true)
        smudge.rounded_rect(box_x, box1_y, box_w, box_h, 5, true, false)
        smudge.rounded_rect(box_x, box1_y, box_w, box_h, 5, false, true)
        smudge.text(cx, box1_y + 8, "<   " .. format_step(step_top_sec) .. "   >", 12, true, "center", true)
    else
        smudge.rounded_rect(box_x, box1_y, box_w, box_h, 5, false, true)
        smudge.text(cx, box1_y + 8, format_step(step_top_sec), 11, false, "center", true)
    end

    -- Divider
    smudge.line(mx + 20, my + 126, mx + mw - 20, my + 126)

    -- Row 2: Bottom Soft Buttons (- / +)
    local r2_y = my + 136
    smudge.text(mx + 24, r2_y, "Bottom - / + Buttons (screen):", 10, true, "left", true)

    local box2_y = r2_y + 28
    if setting_row == 2 then
        smudge.rounded_rect(box_x - 2, box2_y - 2, box_w + 4, box_h + 4, 6, true, true)
        smudge.rounded_rect(box_x, box2_y, box_w, box_h, 5, true, false)
        smudge.rounded_rect(box_x, box2_y, box_w, box_h, 5, false, true)
        smudge.text(cx, box2_y + 8, "<   " .. format_step(step_bot_sec) .. "   >", 12, true, "center", true)
    else
        smudge.rounded_rect(box_x, box2_y, box_w, box_h, 5, false, true)
        smudge.text(cx, box2_y + 8, format_step(step_bot_sec), 11, false, "center", true)
    end

    -- Divider 2
    smudge.line(mx + 20, my + 210, mx + mw - 20, my + 210)

    -- Instructions
    smudge.text(cx, my + 224, "[Select] Switch Row   •   [-] / [+] Change", 10, false, "center", true)
    smudge.text(cx, my + 252, "Press [Done] to save & return", 10, true, "center", true)

    -- Button hints for modal
    smudge.button_hints("Done", "Select", "-", "+")
end

function on_draw()
    if smudge.get_bounds then
        w, h = smudge.get_bounds()
    end
    smudge.clear()

    local cx = math.floor(w / 2)
    local margin = 24
    local card_w = w - (margin * 2)
    local card_x = margin

    -- 1. Header
    smudge.header("Hourglass", "")

    -- 2. Top Banner / Status Pill (y = 98..132, safely below 84px header line)
    local by = 98
    local bh = 34
    if banner_msg ~= "" then
        smudge.rounded_rect(card_x, by, card_w, bh, 6, true)
        smudge.text(cx, by + 7, banner_msg, 11, true, "center", false)
    else
        smudge.rounded_rect(card_x, by, card_w, bh, 6, false)
        local status_str = is_alarm and "TIME'S UP! (PRESS ANY KEY)" or (is_running and "RUNNING" or (has_started and "PAUSED" or "READY"))
        smudge.text(cx, by + 7, status_str, 10, true, "center", true)
    end

    -- 3. Centered Hourglass Animation (y = 170..410)
    local pct = (total_sec > 0) and math.max(0, math.min(1.0, remaining_sec / total_sec)) or 0
    draw_hourglass(cx, 170, pct)

    -- 4. Large Digital Time Readout (y = 464)
    local time_str = format_time(remaining_sec)
    smudge.text(cx, 464, time_str, 28, true, "center", true)

    -- 5. Sub-Card Area (y = 520..616)
    if has_started then
        -- Elapsed progress bar & info
        local elapsed_pct = math.floor((1.0 - pct) * 100)
        local bar_x = card_x + 16
        local bar_w = card_w - 32
        local bar_y = 528
        local bar_h = 18

        smudge.rounded_rect(bar_x, bar_y, bar_w, bar_h, 5, false)
        local bar_fill = math.max(0, math.min(bar_w - 4, math.floor((1.0 - pct) * (bar_w - 4))))
        if bar_fill > 0 then
            smudge.rounded_rect(bar_x + 2, bar_y + 2, bar_fill, bar_h - 4, 3, true)
        end

        smudge.text(cx, 560, string.format("Progress: %d%%  •  Total: %s", elapsed_pct, format_time(total_sec)), 10, true, "center", true)
        local prompt = is_alarm and "TIME'S UP! Press any key to dismiss" or (is_running and "Timer running smoothly" or "Paused • Press [Start] to resume")
        smudge.text(cx, 592, prompt, 10, true, "center", true)
    else
        -- Clean increments information card
        local cy_card = 520
        local ch_card = 96
        smudge.rounded_rect(card_x, cy_card, card_w, ch_card, 8, false)

        local pad = 18
        local lx = card_x + pad
        local rx = card_x + card_w - pad

        -- Row 1: Top/Bottom buttons step
        smudge.text(lx, cy_card + 14, "Top / Bottom buttons", 10, false, "left", true)
        smudge.text(rx, cy_card + 14, "± " .. format_step(step_top_sec), 11, true, "right", true)

        -- Divider
        smudge.line(card_x + 14, cy_card + 46, card_x + card_w - 14, cy_card + 46)

        -- Row 2: Bottom - / + buttons step
        smudge.text(lx, cy_card + 60, "Bottom - / + buttons", 10, false, "left", true)
        smudge.text(rx, cy_card + 60, "± " .. format_step(step_bot_sec), 11, true, "right", true)

        -- Subtitle prompt
        smudge.text(cx, 638, "Hold [Start] to customize step sizes", 9, false, "center", true)
    end

    -- 6. Bottom Button Hints
    if not has_started then
        smudge.button_hints("Exit", "Start", "-", "+")
    else
        local b2_label = is_running and "Pause" or "Start"
        smudge.button_hints("Exit", b2_label, "Reset", "")
    end

    -- 7. Settings Modal Overlay
    if in_settings then
        draw_settings_dialog(cx, w, h)
    end

    -- 8. Alarm Screen Flashing
    if is_alarm and flash_state and smudge.invert_screen then
        smudge.invert_screen()
    end
end

function on_button(btn, pressed)
    if not pressed then return end

    -- Dismiss alarm on any button
    if is_alarm then
        dismiss_alarm()
        return
    end

    -- Check if confirm was held (suppress release trigger)
    if (btn == "btn2" or btn == "confirm") and confirm_suppress_release then
        confirm_suppress_release = false
        return
    end

    -- Settings modal interaction
    if in_settings then
        if btn == "btn1" or btn == "back" then
            in_settings = false
            save_data()
            show_banner("Settings Saved", 1200)
            if smudge.request_update then smudge.request_update() end
        elseif btn == "btn2" or btn == "confirm" then
            setting_row = (setting_row == 1) and 2 or 1
            if smudge.request_update then smudge.request_update() end
        elseif btn == "btn3" or btn == "left" then
            adjust_step(setting_row, -1)
        elseif btn == "btn4" or btn == "right" then
            adjust_step(setting_row, 1)
        elseif btn == "up" or btn == "page_back" then
            setting_row = 1
            if smudge.request_update then smudge.request_update() end
        elseif btn == "down" or btn == "page_forward" then
            setting_row = 2
            if smudge.request_update then smudge.request_update() end
        end
        return
    end

    -- Main screen button interaction
    if btn == "btn1" or btn == "back" then
        smudge.exit()
    elseif btn == "btn2" or btn == "confirm" then
        toggle_start_pause()
    elseif btn == "btn3" or btn == "left" then
        if has_started then
            reset_timer()
        else
            adjust_duration(-step_bot_sec)
        end
    elseif btn == "btn4" or btn == "right" then
        if not has_started then
            adjust_duration(step_bot_sec)
        end
    elseif btn == "up" or btn == "page_back" then
        adjust_duration(-step_top_sec)
    elseif btn == "down" or btn == "page_forward" then
        adjust_duration(step_top_sec)
    end
end

function on_touch(action, tx, ty)
    if action ~= "tap" and action ~= "click" then return end
    local cx = math.floor(w / 2)

    if is_alarm then
        dismiss_alarm()
        return
    end

    if in_settings then
        local mw = 390
        local mh = 330
        local mx = math.floor((w - mw) / 2)
        local my = 230

        -- Done button area or tap outside dialog
        if ty >= my + 260 and ty <= my + 310 then
            in_settings = false
            save_data()
            show_banner("Settings Saved", 1200)
            if smudge.request_update then smudge.request_update() end
            return
        end

        -- Row 1 tap (Top/Bottom buttons step)
        if ty >= my + 60 and ty <= my + 120 then
            setting_row = 1
            if tx < cx then
                adjust_step(1, -1)
            else
                adjust_step(1, 1)
            end
            return
        end

        -- Row 2 tap (Bottom -/+ buttons step)
        if ty >= my + 145 and ty <= my + 215 then
            setting_row = 2
            if tx < cx then
                adjust_step(2, -1)
            else
                adjust_step(2, 1)
            end
            return
        end
        return
    end

    -- Tap Hourglass or Big Time: Start/Pause
    if ty >= 150 and ty <= 480 and math.abs(tx - cx) <= 120 then
        toggle_start_pause()
        return
    end

    -- Tap Increments card: open settings
    if ty >= 508 and ty <= 630 and not has_started then
        in_settings = true
        if smudge.request_update then smudge.request_update() end
        return
    end
end
