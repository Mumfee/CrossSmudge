local h = G.history
if #h >= 2 then
  local p = (G.mode ~= "pass_play" and G.turn == (G.mode == "ai_white" and 1 or -1)) and 2 or 1
  for _ = 1, p do
    if #h >= 2 then unmake_move(G, table.remove(h, #h - 1), table.remove(h)) end
  end
  G.last_move = #h >= 2 and h[#h - 1] or nil
  G.game_over = false; reset_selection(); update_turn_status(); update_stats()
  G.need_full_draw = true; smudge.request_update()
  dofile("save.lua"); collectgarbage()
end
