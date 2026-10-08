L_W, L_H = 480, 800
if smudge.get_bounds then L_W, L_H = smudge.get_bounds() end
local top_y, bot_h = 50, 48
if smudge.get_metrics then
  local m = smudge.get_metrics()
  top_y = (m.top_padding or 0) + (m.header_height or 44) + 4
  bot_h = m.button_hints_height or 48
end
L_HINT_H = bot_h
L_X = (L_W - 384) // 2

local avail_v = L_H - bot_h - top_y
local g = math.max(8, (avail_v - 484) // 6)
L_OPP_Y = top_y + g
L_Y = L_OPP_Y + 36 + g + 4
L_PLY_Y = L_Y + 384 + g + 16
L_STAT_Y = L_PLY_Y + 36 + g

local P_CH = "PNBRQK"
local function draw_piece(x, y, p)
  local t = p > 0 and p or -p
  local c = string.sub(P_CH, t, t)
  local col = (p > 0) and "sprites/w" or "sprites/b"
  smudge.draw_sprite(x, y, 36, 36, col .. c .. "_m.raw", true)
  smudge.draw_sprite(x, y, 36, 36, col .. c .. "_b.raw", false)
  if p < 0 then smudge.draw_sprite(x, y, 36, 36, col .. c .. "_d.raw", true) end
end

function draw_sq(dr, dc)
  local sx, sy = L_X + (dc - 1) * 48, L_Y + (dr - 1) * 48
  smudge.rect(sx, sy, 48, 48, true, false)
  if (dr + dc) % 2 == 1 then smudge.rect_dither(sx, sy, 48, 48, false) end
  local flip = G.mode == "ai_black"
  local sq = (((flip and (9 - dr) or dr) - 1) << 3) + (flip and (9 - dc) or dc)
  local p, lm = G.board[sq], G.last_move
  if lm and (sq == (lm & 127) or sq == ((lm >> 7) & 127)) then
    smudge.rect(sx + 1, sy + 1, 46, 46, false, 1, true)
  end
  if (p == 6 and G.w_chk) or (p == -6 and G.b_chk) then
    smudge.rect(sx + 1, sy + 1, 46, 46, false, 3, true)
  end
  if p ~= 0 then draw_piece(sx + 6, sy + 6, p) end
  if G.sel_sq == sq then smudge.rect(sx + 2, sy + 2, 44, 44, false, 2, true) end
  if G.legal_moves then
    for i = 1, #G.legal_moves do
      local m = G.legal_moves[i]
      if ((m >> 7) & 127) == sq then
        local cx, cy = sx + 24, sy + 24
        if p == 0 and (m >> 14) ~= 2 then
          smudge.circle(cx, cy, 7, true, false); smudge.circle(cx, cy, 5, true, true)
        else smudge.rect(sx + 2, sy + 2, 44, 44, false, 2, true) end
      end
    end
  end
  local hm = G.hint_move
  if hm and (sq == (hm & 127) or sq == ((hm >> 7) & 127)) then
    smudge.rect(sx + 3, sy + 3, 42, 42, false, 2, true)
  end
  if G.cr == dr and G.cc == dc then
    smudge.rect(sx + 1, sy + 1, 46, 46, false, 2, true)
    smudge.rect(sx + 4, sy + 4, 40, 40, false, 1, true)
  end
end

function draw_sq_index(sq)
  local flip = G.mode == "ai_black"
  local r, c = ((sq - 1) >> 3) + 1, ((sq - 1) & 7) + 1
  draw_sq(flip and (9 - r) or r, flip and (9 - c) or c)
end

DIFF_STR = {"Easy", "Medium", "Hard"}
local function draw_card(y, is_white)
  local is_turn = (G.turn == (is_white and 1 or -1)) and not G.game_over
  local ds = DIFF_STR[G.diff] or "Medium"
  local name = (G.mode == "pass_play") and (is_white and "Player 1 (White)" or "Player 2 (Black)") or
    (is_white == (G.mode == "ai_black") and ("AI - " .. ds) or (is_white and "White (You)" or "Black (You)"))
  local diff = is_white and G.diff_mat or -G.diff_mat
  smudge.rect(L_X, y, 384, 36, true, false)
  smudge.rounded_rect(L_X, y, 384, 36, 4, false, is_turn and 2 or 1, true)
  if is_turn then smudge.circle(L_X + 16, y + 18, 4, true, true) end
  smudge.text(L_X + 28, y + 8, name, 10, true, "left", true)
  if diff > 0 then smudge.text(L_X + 372, y + 8, "+" .. diff, 10, true, "right", true) end
end

local function draw_status()
  smudge.rect(L_X, L_STAT_Y - 4, 384, 32, true, false)
  if G.game_over then
    smudge.rounded_rect(L_X + 8, L_STAT_Y - 4, 368, 30, 4, true, true)
    smudge.centered_text(L_STAT_Y + 3, G.over_msg, 12, true, false)
  else smudge.centered_text(L_STAT_Y + 3, G.status, 12, true, true) end
end

local R_LBL = {"1", "2", "3", "4", "5", "6", "7", "8"}
local F_LBL = {"a", "b", "c", "d", "e", "f", "g", "h"}
local function draw_board()
  local flip = G.mode == "ai_black"
  local lx, by = L_X - 14, L_Y + 384 + 8
  for i = 1, 8 do
    local r_lbl = R_LBL[flip and i or (9 - i)]
    local f_lbl = F_LBL[flip and (9 - i) or i]
    smudge.text(lx, L_Y + (i - 1) * 48 + 19, r_lbl, 8, false, "center", true)
    smudge.text(L_X + (i - 1) * 48 + 24, by, f_lbl, 8, false, "center", true)
  end
  for dr = 1, 8 do for dc = 1, 8 do draw_sq(dr, dc) end end
  smudge.rect(L_X - 2, L_Y - 2, 388, 388, false, 2, true)
  if G.sel_sq then draw_sq_index(G.sel_sq) end
  draw_sq(G.cr, G.cc)
end

function on_draw()
  if G.menu then
    dofile("menu.lua"); collectgarbage()
    smudge.button_hints("Back", "Select", "Up", "Down")
    return
  end

  if not G.need_full_draw and next(G.dirty) then
    for sq in pairs(G.dirty) do draw_sq_index(sq); G.dirty[sq] = nil end
    if G.status_dirty then
      local flip = G.mode == "ai_black"
      draw_card(L_OPP_Y, flip); draw_card(L_PLY_Y, not flip); draw_status()
      G.status_dirty = false
    end
    local b1 = G.sel_sq and "Deselect" or "Menu"
    local b2 = G.sel_sq and "Move" or "Select"
    smudge.button_hints(b1, b2, "Up", "Down")
    return
  end

  G.need_full_draw, G.status_dirty = false, false
  for k in pairs(G.dirty) do G.dirty[k] = nil end
  smudge.clear()
  local ds = DIFF_STR[G.diff] or "Medium"
  local sub = (G.mode == "pass_play") and "2-Player" or (ds .. " AI")
  smudge.header("Chess", sub)
  local flip = G.mode == "ai_black"
  draw_card(L_OPP_Y, flip)
  draw_board()
  draw_card(L_PLY_Y, not flip)
  draw_status()
  local b1 = G.sel_sq and "Deselect" or "Menu"
  local b2 = G.game_over and "New Game" or (G.sel_sq and "Move" or "Select")
  smudge.button_hints(b1, b2, "Up", "Down")
end
draw = on_draw
