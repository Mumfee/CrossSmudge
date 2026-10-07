-- Solitaire Input, Navigation, and Interaction Handlers

function nav_left()
    if cur_zone == 1 then
        cur_col = cur_col - 1
        if cur_col == 3 then cur_col = 2 end
        if cur_col < 1 then cur_col = 7 end
    else
        cur_col = (cur_col > 1) and (cur_col - 1) or 7
        local pile = tableau[cur_col]
        cur_card = math.max(1, #pile)
    end
    if smudge.request_update then smudge.request_update() end
end

function nav_right()
    if cur_zone == 1 then
        cur_col = cur_col + 1
        if cur_col == 3 then cur_col = 4 end
        if cur_col > 7 then cur_col = 1 end
    else
        cur_col = (cur_col < 7) and (cur_col + 1) or 1
        local pile = tableau[cur_col]
        cur_card = math.max(1, #pile)
    end
    if smudge.request_update then smudge.request_update() end
end

function nav_up()
    if cur_zone == 2 then
        local pile = tableau[cur_col]
        if cur_card > 1 and pile[cur_card - 1] and card_u(pile[cur_card - 1]) then
            cur_card = cur_card - 1
        else
            cur_zone = 1
            if cur_col == 3 then cur_col = 2 end
        end
    end
    if smudge.request_update then smudge.request_update() end
end

function nav_down()
    if cur_zone == 1 then
        cur_zone = 2
        local pile = tableau[cur_col]
        cur_card = math.max(1, #pile)
    elseif cur_zone == 2 then
        local pile = tableau[cur_col]
        if cur_card < #pile then
            cur_card = cur_card + 1
        end
    end
    if smudge.request_update then smudge.request_update() end
end

function on_button(btn, pressed)
    if not pressed then return end

    if (btn == "btn1" or btn == "back") and back_suppress_release then
        back_suppress_release = false
        return
    end

    if show_reset_confirm then
        if btn == "btn1" or btn == "back" then
            show_reset_confirm = false
            if smudge.request_update then smudge.request_update() end
            return
        elseif btn == "btn2" or btn == "confirm" then
            show_reset_confirm = false
            new_game()
            return
        end
        return
    end

    if btn == "btn1" or btn == "back" then
        smudge.exit()
    elseif btn == "btn2" or btn == "confirm" then
        if is_won then
            new_game()
            return
        end

        if sel_zone > 0 then
            if cur_zone == 2 then
                execute_move(2, cur_col)
            elseif cur_zone == 1 and cur_col >= 4 then
                execute_move(1, cur_col)
            else
                sel_zone = 0
                if smudge.request_update then smudge.request_update() end
            end
        else
            if cur_zone == 1 then
                if cur_col == 1 then
                    flip_stock()
                elseif cur_col == 2 and #waste > 0 then
                    local c = waste[#waste]
                    if not try_auto_foundation(c, 1, 2) then
                        sel_zone = 1
                        sel_col = 2
                        sel_card = #waste
                        if smudge.request_update then smudge.request_update() end
                    end
                end
            elseif cur_zone == 2 then
                local pile = tableau[cur_col]
                if #pile > 0 then
                    local c = pile[#pile]
                    if card_u(c) then
                        local card_idx = cur_card or #pile
                        if card_idx == #pile and try_auto_foundation(c, 2, cur_col) then
                            -- moved to foundation
                        else
                            sel_zone = 2
                            sel_col = cur_col
                            sel_card = card_idx
                            if smudge.request_update then smudge.request_update() end
                        end
                    end
                end
            end
        end
    elseif btn == "btn3" or btn == "left" then
        nav_up()
    elseif btn == "btn4" or btn == "right" then
        nav_down()
    elseif btn == "up" or btn == "page_back" then
        nav_left()
    elseif btn == "down" or btn == "page_forward" then
        nav_right()
    end
end

function on_swipe(dir)
    if show_reset_confirm then return end
    if dir == "left" then nav_left()
    elseif dir == "right" then nav_right()
    elseif dir == "up" then nav_up()
    elseif dir == "down" then nav_down()
    end
end

function on_touch(action, tx, ty)
    if action ~= "tap" and action ~= "click" then return end

    if show_reset_confirm then
        local dw = math.min(w - 48, 380)
        local dh = 150
        local dx = math.floor((w - dw) / 2)
        local dy = math.floor((h - dh) / 2)
        local bw = math.floor((dw - 36) / 2)
        local bh = 34
        local by = dy + 92
        local b1_x = dx + 12
        local b2_x = dx + dw - bw - 12

        if ty >= by and ty <= by + bh and tx >= b1_x and tx <= b1_x + bw then
            show_reset_confirm = false
            if smudge.request_update then smudge.request_update() end
            return
        end

        if ty >= by and ty <= by + bh and tx >= b2_x and tx <= b2_x + bw then
            show_reset_confirm = false
            new_game()
            return
        end

        show_reset_confirm = false
        if smudge.request_update then smudge.request_update() end
        return
    end

    if is_won then
        new_game()
        return
    end

    if ty >= (h - 60) then
        local btn_w = math.floor(w / 4)
        if tx < btn_w then
            smudge.exit()
        elseif tx < btn_w * 2 then
            on_button("confirm", true)
        elseif tx < btn_w * 3 then
            nav_up()
        else
            nav_down()
        end
        return
    end

    if ty <= 50 and tx >= (w - 120) then
        show_reset_confirm = true
        if smudge.request_update then smudge.request_update() end
        return
    end

    local total_slots = 7
    local card_w = 56
    local gap = 10
    local slot_step = card_w + gap
    local total_w = total_slots * card_w + (total_slots - 1) * gap
    local margin = math.floor((w - total_w) / 2)
    local card_h = math.floor(card_w * 1.36)
    local up_y = 128
    local div_y = up_y + card_h + 10
    local tab_y = div_y + 10

    if ty >= up_y and ty <= up_y + card_h then
        local sx = margin
        if tx >= sx and tx <= sx + card_w then
            flip_stock()
            return
        end

        local wx = margin + slot_step
        if tx >= wx and tx <= wx + card_w and #waste > 0 then
            local c = waste[#waste]
            if not try_auto_foundation(c, 1, 2) then
                if sel_zone == 1 then
                    sel_zone = 0
                else
                    sel_zone = 1
                    sel_col = 2
                    sel_card = #waste
                end
                if smudge.request_update then smudge.request_update() end
            end
            return
        end

        for f = 1, 4 do
            local fx = margin + (f + 2) * slot_step
            if tx >= fx and tx <= fx + card_w then
                if sel_zone > 0 then
                    execute_move(1, f + 3)
                end
                return
            end
        end
    end

    if ty >= tab_y then
        local col = math.floor((tx - margin) / slot_step) + 1
        if col >= 1 and col <= 7 then
            local pile = tableau[col]

            if #pile == 0 then
                if sel_zone > 0 then
                    execute_move(2, col)
                end
                return
            end

            local face_down_step = (#pile > 8) and 10 or 12
            local face_up_step = (#pile > 8) and 16 or 22
            local cy = tab_y
            local tapped_idx = #pile

            for i = 1, #pile - 1 do
                local step = card_u(pile[i]) and face_up_step or face_down_step
                if ty >= cy and ty < cy + step then
                    tapped_idx = i
                    break
                end
                cy = cy + step
            end

            local tapped_card = pile[tapped_idx]
            if tapped_card and card_u(tapped_card) then
                if sel_zone > 0 then
                    if not execute_move(2, col) then
                        sel_zone = 2
                        sel_col = col
                        sel_card = tapped_idx
                        if smudge.request_update then smudge.request_update() end
                    end
                else
                    if tapped_idx == #pile then
                        if not try_auto_foundation(tapped_card, 2, col) then
                            sel_zone = 2
                            sel_col = col
                            sel_card = tapped_idx
                            if smudge.request_update then smudge.request_update() end
                        end
                    else
                        sel_zone = 2
                        sel_col = col
                        sel_card = tapped_idx
                        if smudge.request_update then smudge.request_update() end
                    end
                end
            end
            return
        end
    end

    if ty >= 84 and ty <= 122 then
        if not auto_complete_step() then
            show_banner("No auto moves available", 1200)
        end
        return
    end
end
