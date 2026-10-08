local PV = {1, 3, 3, 5, 9, 0}
local POOL = {}

function reset_selection()
  if G.sel_sq then mark_dirty_sq(G.sel_sq) end
  local lm = G.legal_moves
  if lm then for i = 1, #lm do mark_dirty_sq((lm[i] >> 7) & 127) end end
  local hm = G.hint_move
  if hm then mark_dirty_sq(hm & 127); mark_dirty_sq((hm >> 7) & 127) end
  G.sel_sq, G.legal_moves, G.hint_move = nil, nil, nil
end

function update_turn_status()
  local c = in_check(G.board, G.turn)
  G.w_chk, G.b_chk = (G.turn == 1 and c), (G.turn == -1 and c)
  G.status = c and ((G.turn == 1) and "White in Check!" or "Black in Check!") or ((G.turn == 1) and "White to move" or "Black to move")
end

function update_stats()
  local d, b = 0, G.board
  for i = 1, 64 do
    local p = b[i]
    if p ~= 0 then d = d + (p > 0 and PV[p] or -PV[-p]) end
  end
  G.diff_mat = d
end


local function check_over()
  local mv = gen_moves(G, nil, POOL, true)
  if #mv == 0 then
    return true, in_check(G.board, G.turn) and ((G.turn == 1) and "Black wins (Checkmate)" or "White wins (Checkmate)") or "Draw (Stalemate)"
  end
  if G.halfmove >= 100 then return true, "Draw (50 moves)" end
  return false, nil
end

function exec_move(m)
  local f, t = m & 127, (m >> 7) & 127
  local prev_lm = G.last_move
  local u = make_move(G, m)
  local h = G.history
  h[#h + 1] = m; h[#h + 1] = u
  if #h > 12 then table.remove(h, 1); table.remove(h, 1) end
  G.last_move = m
  reset_selection()
  local ov, rsn = check_over()
  if ov then
    G.game_over, G.over_msg, G.status = true, rsn, rsn
  else
    update_turn_status()
    if (G.mode == "ai_white" and G.turn == -1) or (G.mode == "ai_black" and G.turn == 1) then
      G.ai_pending = true
      G.ai_start_time = (smudge.millis and smudge.millis() or 0) + 150
      G.status = "AI thinking..."
    end
  end
  update_stats()
  mark_dirty_sq(f); mark_dirty_sq(t)
  if prev_lm then mark_dirty_sq(prev_lm & 127); mark_dirty_sq((prev_lm >> 7) & 127) end
  local sp = m >> 14
  if sp == 4 then
    local r1 = (t == 63) and 64 or ((t == 59) and 57 or ((t == 7) and 8 or 1))
    local r2 = (t == 63) and 62 or ((t == 59) and 60 or ((t == 7) and 6 or 4))
    mark_dirty_sq(r1); mark_dirty_sq(r2)
  elseif sp == 2 then
    mark_dirty_sq(f > 32 and (t + 8) or (t - 8))
  end
  if G.w_chk or G.b_chk then
    for i = 1, 64 do local p = G.board[i]; if p == 6 or p == -6 then mark_dirty_sq(i) end end
  end
  mark_dirty_rc(G.cr, G.cc)
  G.status_dirty = true
  collectgarbage()
  dofile("save.lua"); collectgarbage()
  smudge.request_update()
end

function set_selection(new_sq, moves)
  reset_selection()
  G.sel_sq, G.legal_moves = new_sq, moves
  if new_sq then
    mark_dirty_sq(new_sq)
    if moves and #moves > 0 then
      G.sel_move_idx = 1
      for i = 1, #moves do mark_dirty_sq((moves[i] >> 7) & 127) end
      local dest_sq = (moves[1] >> 7) & 127
      local flip = G.mode == "ai_black"
      local r, c = ((dest_sq - 1) >> 3) + 1, ((dest_sq - 1) & 7) + 1
      mark_dirty_rc(G.cr, G.cc)
      G.cr = flip and (9 - r) or r; G.cc = flip and (9 - c) or c
      mark_dirty_rc(G.cr, G.cc)
    else
      G.status = "No legal moves"; G.status_dirty = true
    end
  end
  mark_dirty_rc(G.cr, G.cc)
  smudge.request_update()
end

function on_sq(sq)
  if G.game_over or G.ai_busy or G.ai_pending then return end
  if (G.mode == "ai_white" and G.turn ~= 1) or (G.mode == "ai_black" and G.turn ~= -1) then return end
  local p = G.board[sq]
  if not G.sel_sq then
    if p * G.turn > 0 then set_selection(sq, gen_moves(G, sq, POOL)) end
  else
    local chosen
    local lm = G.legal_moves
    if lm then
      for i = 1, #lm do
        local m = lm[i]
        if ((m >> 7) & 127) == sq then chosen = m; break end
      end
    end
    if chosen then
      exec_move(chosen)
    elseif p * G.turn > 0 then
      set_selection(sq, gen_moves(G, sq, POOL))
    else
      G.status = "Select legal move"; G.status_dirty = true; smudge.request_update()
    end
  end
end

function ai_step()
  if not G.ai_busy or G.game_over then return end
  G.ai_depth = (G.diff == 1) and 1 or ((G.diff == 3) and 3 or 2)
  local bm = dofile("ai.lua")
  G.ai_busy = false
  if bm then exec_move(bm)
  else G.game_over, G.status, G.need_full_draw = true, "No moves", true; smudge.request_update() end
end
