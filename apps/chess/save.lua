if G and G.board then
  if G.game_over then
    smudge.save("game", "")
    return
  end
  local t = {}
  for i = 1, 64 do t[i] = string.char((G.board[i] or 0) + 65) end
  local lm = G.last_move
  local lf = lm and (lm & 127) or 0
  local lt = lm and ((lm >> 7) & 127) or 0
  local hm = G.halfmove or 0
  t[65] = string.char(
    48 + (G.cr or 7),
    48 + (G.cc or 5),
    (G.turn == 1 and 119 or 98),
    48 + (G.diff or 2),
    35 + (G.castling or 15),
    35 + (G.ep_sq or 0),
    48 + (hm // 10),
    48 + (hm % 10),
    35 + lf,
    35 + lt,
    (G.mode == "ai_black" and 98 or (G.mode == "pass_play" and 112 or 119)),
    48
  )
  smudge.save("game", table.concat(t))
end
