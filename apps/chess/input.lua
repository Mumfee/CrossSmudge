local function set_c_sq(s)
  local f, r, c = G.mode == "ai_black", ((s - 1) >> 3) + 1, ((s - 1) & 7) + 1
  mark_dirty_rc(G.cr, G.cc)
  G.cr = f and (9 - r) or r; G.cc = f and (9 - c) or c
  mark_dirty_rc(G.cr, G.cc); smudge.request_update()
end

local function nav(dr, dc)
  mark_dirty_rc(G.cr, G.cc)
  local r = ((G.cr - 1 + dr) % 8) + 1
  local c = ((G.cc - 1 + dc) % 8) + 1
  if dr ~= 0 and r == ((dr > 0) and 1 or 8) then c = ((c - 1 + dr) % 8) + 1 end
  if dc ~= 0 and c == ((dc > 0) and 1 or 8) then r = ((r - 1 + dc) % 8) + 1 end
  G.cr, G.cc = r, c
  mark_dirty_rc(G.cr, G.cc); smudge.request_update()
end

local function find_legal_move_dir(dr, dc)
  local lm = G.legal_moves
  if not lm or #lm == 0 then return end
  local flip = G.mode == "ai_black"
  local cur_r, cur_c = G.cr, G.cc
  local best_dest, best_score
  local wrap_dest, min_fwd, min_lat

  for i = 1, #lm do
    local dest_sq = (lm[i] >> 7) & 127
    local r = ((dest_sq - 1) >> 3) + 1
    local c = ((dest_sq - 1) & 7) + 1
    local tr = flip and (9 - r) or r
    local tc = flip and (9 - c) or c

    if tr ~= cur_r or tc ~= cur_c then
      local delta_r = tr - cur_r
      local delta_c = tc - cur_c
      local fwd = delta_r * dr + delta_c * dc
      local lat = (dr ~= 0) and math.abs(delta_c) or math.abs(delta_r)

      if fwd > 0 then
        local score = fwd * 4 + lat * 10
        if not best_score or score < best_score then
          best_score, best_dest = score, dest_sq
        end
      elseif not best_dest and fwd < 0 then
        if not min_fwd or fwd < min_fwd or (fwd == min_fwd and lat < min_lat) then
          min_fwd, min_lat, wrap_dest = fwd, lat, dest_sq
        end
      end
    end
  end

  local chosen = best_dest or wrap_dest
  if chosen then set_c_sq(chosen) end
end

function on_button(b, p)
  if not p then return end
  -- Bottom buttons: btn3/left is UP, btn4/right is DOWN
  local is_up = (b == "btn3" or b == "left")
  local is_down = (b == "btn4" or b == "right")
  -- Top side buttons: up/page_back is LEFT, down/page_forward is RIGHT
  local is_left = (b == "up" or b == "page_back")
  local is_right = (b == "down" or b == "page_forward")
  local is_ok = (b == "confirm" or b == "btn2")
  local is_back = (b == "back" or b == "btn1")

  if G.menu then
    if is_up or is_left then
      G.menu_i = (G.menu_i == 1) and 7 or (G.menu_i - 1)
      smudge.request_update()
    elseif is_down or is_right then
      G.menu_i = (G.menu_i == 7) and 1 or (G.menu_i + 1)
      smudge.request_update()
    elseif is_ok then
      local i = G.menu_i
      if i == 3 then
        G.diff = (G.diff % 3) + 1
        dofile("save.lua"); collectgarbage()
        smudge.request_update()
      else
        G.menu, G.need_full_draw = false, true
        if i == 2 then dofile("new_game.lua"); collectgarbage()
        elseif i == 4 then
          G.mode = (G.mode == "ai_white") and "ai_black" or ((G.mode == "ai_black") and "pass_play" or "ai_white")
          dofile("new_game.lua"); collectgarbage()
        elseif i == 5 then dofile("undo.lua"); collectgarbage()
        elseif i == 6 then
          G.hint_pending = true; G.status = "Thinking..."; G.status_dirty = true
        elseif i == 7 then dofile("save.lua"); smudge.exit() end
        smudge.request_update()
      end
    elseif is_back then
      G.menu, G.need_full_draw = false, true
      smudge.request_update()
    end
    return
  end

  if is_back then
    if G.sel_sq then set_selection(nil, nil)
    else G.menu, G.menu_i = true, 1; smudge.request_update() end
  elseif is_ok then
    if G.game_over then dofile("new_game.lua"); collectgarbage()
    else
      local f = G.mode == "ai_black"
      on_sq((((f and (9 - G.cr) or G.cr) - 1) << 3) + (f and (9 - G.cc) or G.cc))
    end
  else
    local lm = G.sel_sq and G.legal_moves
    if lm and #lm > 0 then
      if is_up then find_legal_move_dir(-1, 0); return
      elseif is_down then find_legal_move_dir(1, 0); return
      elseif is_left then find_legal_move_dir(0, -1); return
      elseif is_right then find_legal_move_dir(0, 1); return end
    end
    if is_up then nav(-1, 0)
    elseif is_down then nav(1, 0)
    elseif is_left then nav(0, -1)
    elseif is_right then nav(0, 1) end
  end
end

function on_tap(x, y)
  if y >= L_H - 64 then
    local c = x // (L_W // 4) + 1
    local btn = (c == 1) and "back" or ((c == 2) and "confirm" or ((c == 3) and "btn3" or "btn4"))
    on_button(btn, true); return
  end
  if G.menu then
    local mx, my = (L_W - 320) // 2, (L_H - 320) // 2
    if x >= mx and x <= mx + 320 and y >= my + 40 and y <= my + 310 then
      local k = (y - (my + 42)) // 38 + 1
      if k >= 1 and k <= 7 then G.menu_i = k; on_button("confirm", true) end
    else G.menu, G.need_full_draw = false, true; smudge.request_update() end
    return
  end
  if x >= L_X and x < L_X + 384 and y >= L_Y and y < L_Y + 384 then
    local dr, dc = (y - L_Y) // 48 + 1, (x - L_X) // 48 + 1
    mark_dirty_rc(G.cr, G.cc); G.cr, G.cc = dr, dc; mark_dirty_rc(dr, dc)
    local f = G.mode == "ai_black"
    on_sq((((f and (9 - dr) or dr) - 1) << 3) + (f and (9 - dc) or dc))
  end
end

function on_swipe(d)
  if G.menu then return end
  local lm = G.sel_sq and G.legal_moves
  local dr, dc = 0, 0
  if d == "up" then dr = -1
  elseif d == "down" then dr = 1
  elseif d == "left" then dc = -1
  elseif d == "right" then dc = 1 end
  if dr ~= 0 or dc ~= 0 then
    if lm and #lm > 0 then find_legal_move_dir(dr, dc)
    else nav(dr, dc) end
  end
end
