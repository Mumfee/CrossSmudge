dofile("suits.lua")
dofile("engine.lua")
dofile("input.lua")

local RANKS = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"}

function draw_card(cx, cy, card_w, card_h, card, is_selected, is_cursor)
    smudge.rounded_rect(cx, cy, card_w, card_h, 4, true, false)
    if is_selected then
        smudge.rounded_rect(cx - 2, cy - 2, card_w + 4, card_h + 4, 6, false, 2, true)
        smudge.rounded_rect(cx, cy, card_w, card_h, 4, false, 1, true)
    elseif is_cursor then
        smudge.rounded_rect(cx - 2, cy - 2, card_w + 4, card_h + 4, 6, false, 2, true)
        smudge.rounded_rect(cx, cy, card_w, card_h, 4, false, 1, true)
    else
        smudge.rounded_rect(cx, cy, card_w, card_h, 4, false, 1, true)
    end

    if not card then return end

    if not card_u(card) then
        if smudge.rect_dither then
            smudge.rect_dither(cx + 4, cy + 4, card_w - 8, card_h - 8, false)
        else
            smudge.rounded_rect(cx + 4, cy + 4, card_w - 8, card_h - 8, 2, false)
        end
        return
    end

    local s = card_s(card)
    local r = card_r(card)
    local rank_str = RANKS[r] or "?"
    smudge.text(cx + 4, cy + 3, rank_str, 10, true, "left", true)
    draw_suit_14(cx + card_w - 17, cy + 3, s, true)

    local mid_x = cx + math.floor((card_w - 22) / 2)
    local mid_y = cy + math.floor((card_h - 22) / 2)
    draw_suit_22(mid_x, mid_y, s, true)
end

local function draw_reset_dialog()
    local dw = math.min(w - 40, 420)
    local dh = 156
    local dx = math.floor((w - dw) / 2)
    local dy = math.floor((h - dh) / 2)

    smudge.rounded_rect(dx, dy, dw, dh, 8, true, false)
    smudge.rounded_rect(dx, dy, dw, dh, 8, false, 3, true)

    local cx = math.floor(w / 2)
    smudge.text(cx, dy + 22, "RESET GAME?", 12, true, "center", true)
    smudge.text(cx, dy + 56, "Abandon current game and deal fresh?", 10, false, "center", true)

    local bw = math.floor((dw - 36) / 2)
    local bh = 36
    local by = dy + 96
    local b1_x = dx + 12
    local b2_x = dx + dw - bw - 12

    smudge.rounded_rect(b1_x, by, bw, bh, 5, false, 1, true)
    smudge.text(b1_x + math.floor(bw / 2), by + 8, "Cancel", 10, true, "center", true)

    smudge.rounded_rect(b2_x, by, bw, bh, 5, true, true)
    smudge.text(b2_x + math.floor(bw / 2), by + 8, "Reset", 10, true, "center", false)
end

function on_draw()
    if smudge.get_bounds then w, h = smudge.get_bounds() end
    smudge.clear()

    local cx = math.floor(w / 2)
    smudge.header("Solitaire", "Klondike")

    local total_slots = 7
    local card_w = 56
    local gap = 10
    local slot_step = card_w + gap
    local total_w = total_slots * card_w + (total_slots - 1) * gap
    local margin = math.floor((w - total_w) / 2)
    local card_h = math.floor(card_w * 1.36)

    local by = 86
    local bh = 34
    smudge.rounded_rect(margin, by, total_w, bh, 5, false)

    if banner_msg ~= "" then
        smudge.rounded_rect(margin, by, total_w, bh, 5, true)
        smudge.text(cx, by + 2, banner_msg, 10, true, "center", false)
    else
        local f_total = #foundations[1] + #foundations[2] + #foundations[3] + #foundations[4]
        local status_str = string.format("Moves: %d   •   Foundations: %d/52", moves_count, f_total)
        smudge.text(cx, by + 2, status_str, 10, true, "center", true)
    end

    local up_y = 128
    -- 1. Stock
    local stock_x = margin
    local is_cur_stock = (cur_zone == 1 and cur_col == 1)
    if #stock > 0 then
        draw_card(stock_x, up_y, card_w, card_h, stock[#stock], false, is_cur_stock)
        smudge.text(stock_x + math.floor(card_w / 2), up_y + math.floor(card_h / 2) - 8, tostring(#stock), 12, true, "center", false)
    else
        draw_card(stock_x, up_y, card_w, card_h, nil, false, is_cur_stock)
        local mid_x = stock_x + math.floor(card_w / 2)
        local mid_y = up_y + math.floor(card_h / 2)
        smudge.circle(mid_x, mid_y, 11, false, true)
    end

    -- 2. Waste
    local waste_x = margin + slot_step
    local is_cur_waste = (cur_zone == 1 and cur_col == 2)
    if #waste > 0 then
        local is_sel = (sel_zone == 1)
        draw_card(waste_x, up_y, card_w, card_h, waste[#waste], is_sel, is_cur_waste)
    else
        draw_card(waste_x, up_y, card_w, card_h, nil, false, is_cur_waste)
    end

    -- 3. Foundations 1..4
    for f = 1, 4 do
        local fx = margin + (f + 2) * slot_step
        local pile = foundations[f]
        local is_sel = (sel_zone == 3 and sel_col == f)
        local is_cur = (cur_zone == 1 and cur_col == (f + 3))

        if #pile > 0 then
            draw_card(fx, up_y, card_w, card_h, pile[#pile], is_sel, is_cur)
        else
            draw_card(fx, up_y, card_w, card_h, nil, is_sel, is_cur)
            draw_suit_22(fx + math.floor((card_w - 22) / 2), up_y + math.floor((card_h - 22) / 2), f, true)
        end
    end

    local div_y = up_y + card_h + 10
    smudge.line(margin, div_y, margin + total_w, div_y)

    -- 4. Tableau (Cols 1..7)
    local tab_y = div_y + 10
    for col = 1, 7 do
        local tx = margin + (col - 1) * slot_step
        local pile = tableau[col]

        if #pile == 0 then
            local is_cur = (cur_zone == 2 and cur_col == col)
            draw_card(tx, tab_y, card_w, card_h, nil, false, is_cur)
        else
            local cy = tab_y
            local face_down_step = (#pile > 8) and 10 or 12
            local face_up_step = (#pile > 8) and 16 or 22

            for idx, card in ipairs(pile) do
                local is_sel = (sel_zone == 2 and sel_col == col and idx >= sel_card)
                local is_cur = (cur_zone == 2 and cur_col == col and idx == cur_card)
                draw_card(tx, cy, card_w, card_h, card, is_sel, is_cur)

                if idx < #pile then
                    cy = cy + (card_u(card) and face_up_step or face_down_step)
                end
            end
        end
    end

    if is_won then
        local vy = 280
        local vh = 160
        local vw = w - (margin * 4)
        local vx = margin * 2
        smudge.rounded_rect(vx, vy, vw, vh, 8, true)
        smudge.rounded_rect(vx + 3, vy + 3, vw - 6, vh - 6, 6, false, 2, false)
        smudge.text(cx, vy + 24, "VICTORY!", 16, true, "center", false)
        smudge.text(cx, vy + 60, string.format("Completed in %d moves", moves_count), 11, true, "center", false)
        smudge.text(cx, vy + 92, "Press [Move] to play again", 10, false, "center", false)
    end

    if show_reset_confirm then
        draw_reset_dialog()
        smudge.button_hints("Cancel", "Reset", "", "")
    else
        local act_label = (sel_zone > 0) and "Move" or "Select"
        smudge.button_hints("Exit", act_label, "Up", "Down")
    end
end

function on_init()
    if smudge.get_bounds then w, h = smudge.get_bounds() end
    if smudge.load then
        games_won = tonumber(smudge.load("games_won", "0")) or 0
    end
    if not load_game() then
        collectgarbage("collect")
        new_game()
    end
    collectgarbage("collect")
end

function on_update()
    local now = (smudge.millis and smudge.millis()) or (os.time() * 1000)
    if banner_msg ~= "" and banner_expires > 0 and now >= banner_expires then
        banner_msg = ""
        banner_expires = 0
        if smudge.request_update then smudge.request_update() end
    end

    if smudge.is_button_down and smudge.is_button_down("back") then
        if not back_held_start and not show_reset_confirm then
            back_held_start = now
        elseif back_held_start and (now - back_held_start) >= 500 and not show_reset_confirm then
            show_reset_confirm = true
            back_suppress_release = true
            back_held_start = nil
            if smudge.request_update then smudge.request_update() end
        end
    else
        back_held_start = nil
    end
end

function on_exit()
    save_game()
end
