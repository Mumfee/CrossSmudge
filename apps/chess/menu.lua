local M = {"Resume", "New Game", "", "", "Undo", "Hint", "Exit"}
local x, y = (L_W - 320) // 2, (L_H - 320) // 2
smudge.rect(x + 4, y + 4, 320, 320, true, true)
smudge.rect(x, y, 320, 320, true, false)
smudge.rect(x, y, 320, 320, false, 2, true)
smudge.centered_text(y + 12, "GAME MENU", 12, true, true)
smudge.line(x + 10, y + 36, x + 310, y + 36, true)
local ds = DIFF_STR[G.diff] or "Med"
local ms = (G.mode == "ai_white") and "vs AI (W)" or ((G.mode == "ai_black") and "vs AI (B)" or "2-Player")
for i = 1, 7 do
  local t = (i == 3) and ("Diff: " .. ds) or ((i == 4) and ("Mode: " .. ms) or M[i])
  local iy, s = y + 42 + (i - 1) * 38, (i == G.menu_i)
  if s then smudge.rounded_rect(x + 8, iy, 304, 32, 4, true, true) end
  smudge.text(x + 16, iy + 6, t, 10, s, "left", not s)
end
