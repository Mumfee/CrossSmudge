local PV = {100, 320, 330, 500, 900, 20000}
local PL = {{}, {}, {}}
local function eval(s)
  local v, b = 0, s.board
  for i = 1, 64 do
    local p = b[i]
    if p ~= 0 then
      local a = p > 0 and p or -p
      local val = PV[a]
      if a == 1 then val = val + ((p > 0) and (8 - (((i - 1) >> 3) + 1)) or (((i - 1) >> 3))) * 10
      elseif a == 4 then
        local r = ((i - 1) >> 3) + 1
        if (p > 0 and r == 2) or (p < 0 and r == 7) then val = val + 30 end
      end
      v = v + (p > 0 and val or -val)
    end
  end
  return v * s.turn
end
local function search(s, d, a, b)
  if d == 0 then return eval(s), nil end
  local mv = gen_moves(s, nil, PL[d])
  if #mv == 0 then return in_check(s.board, s.turn) and (-100000 - d) or 0, nil end
  local bv, bm = -999999, mv[1]
  for i = 1, #mv do
    local m = mv[i]
    local u = make_move(s, m)
    local v = -search(s, d - 1, -b, -a)
    unmake_move(s, m, u)
    if v > bv then bv = v; bm = m end
    if v > a then a = v end
    if a >= b then break end
  end
  return bv, bm
end
local d = G.ai_depth or 2
local _, bm = search(G, d, -999999, 999999)
return bm
