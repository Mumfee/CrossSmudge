local b = G.board
for i = 1, 64 do b[i] = 0 end
local SR = "\4\2\3\5\6\3\2\4"
for i = 1, 8 do
  local p = string.byte(SR, i)
  b[i], b[8 + i], b[48 + i], b[56 + i] = -p, -1, 1, p
end
G.turn, G.castling, G.ep_sq, G.halfmove, G.fullmove = 1, 15, 0, 0, 1
G.history, G.last_move = {}, nil
G.game_over, G.over_msg, G.ai_busy = false, "", false
G.w_chk, G.b_chk = false, false
G.cr, G.cc = (G.mode == "ai_black") and 2 or 7, 5
G.need_full_draw = true
reset_selection(); update_stats()
if G.mode == "ai_black" then
  G.status = "AI thinking..."; G.ai_pending = true
  G.ai_start_time = (smudge.millis and smudge.millis() or 0) + 150
else G.status = "White to move"; G.ai_pending = nil end
smudge.request_update()
