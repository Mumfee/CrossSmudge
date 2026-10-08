local s = smudge.load("game", "")
local ok = false
if s and #s >= 76 and string.byte(s, 76) ~= 49 then
  for i = 1, 64 do G.board[i] = string.byte(s, i) - 65 end
  G.cr, G.cc = string.byte(s, 65) - 48, string.byte(s, 66) - 48
  G.turn = (string.byte(s, 67) == 119) and 1 or -1
  G.diff = string.byte(s, 68) - 48
  G.castling = string.byte(s, 69) - 35
  G.ep_sq = string.byte(s, 70) - 35
  G.halfmove = (string.byte(s, 71) - 48) * 10 + (string.byte(s, 72) - 48)
  local lf, lt = string.byte(s, 73) - 35, string.byte(s, 74) - 35
  G.last_move = (lf > 0) and (lf | (lt << 7)) or nil
  local m = string.byte(s, 75)
  G.mode = (m == 98) and "ai_black" or ((m == 112) and "pass_play" or "ai_white")
  G.game_over, G.history = false, {}
  reset_selection(); update_turn_status(); update_stats()
  if (G.mode == "ai_white" and G.turn == -1) or (G.mode == "ai_black" and G.turn == 1) then
    G.ai_pending = true
    G.ai_start_time = (smudge.millis and smudge.millis() or 0) + 150
    G.status = "AI thinking..."
  end
  G.need_full_draw = true
  ok = true
end
if not ok then
  dofile("new_game.lua")
end
