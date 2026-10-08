local KD = {-17, -15, -10, -6, 6, 10, 15, 17}
local DIRS = {-8, 8, -1, 1, -9, -7, 7, 9}
local CR_M = {[1]=7, [8]=11, [57]=13, [64]=14, [5]=3, [61]=12}
function is_attacked(b, sq, color)
local r, c = ((sq - 1) >> 3) + 1, ((sq - 1) & 7) + 1
local pr = r + (color == 1 and 1 or -1)
if pr >= 1 and pr <= 8 then
local pb = (pr - 1) << 3
if (c > 1 and b[pb + c - 1] == color) or (c < 8 and b[pb + c + 1] == color) then return true end
end
for i = 1, 8 do
local to = sq + KD[i]
if to >= 1 and to <= 64 and b[to] == color * 2 then
local dc = (((to - 1) & 7) + 1) - c
if dc >= -2 and dc <= 2 then return true end
end
end
for i = 1, 8 do
local d = DIRS[i]
local to, prev_c, step = sq, c, 1
while true do
to = to + d
if to < 1 or to > 64 then break end
local tc = ((to - 1) & 7) + 1
local dc = tc - prev_c
if dc < -1 or dc > 1 then break end
prev_c = tc
local p = b[to]
if p ~= 0 then
if p * color > 0 then
local t = p > 0 and p or -p
if t == 5 or (i > 4 and t == 3) or (i <= 4 and t == 4) or (step == 1 and t == 6) then
return true
end
end
break
end
step = step + 1
end
end
return false
end
function in_check(b, color)
local target = color * 6
for i = 1, 64 do
if b[i] == target then return is_attacked(b, i, -color) end
end
return false
end
function make_move(s, m)
local b = s.board
local f, t, fl = m & 127, (m >> 7) & 127, m >> 14
local fp, tp = b[f], b[t]
local undo = (tp + 7) | (s.castling << 4) | (s.ep_sq << 8) | (s.halfmove << 15)
b[t], b[f] = fp, 0
if fl == 2 then b[t + (s.turn == 1 and 8 or -8)] = 0
elseif fl == 4 then
if t == 63 then b[62], b[64] = b[64], 0
elseif t == 59 then b[60], b[57] = b[57], 0
elseif t == 7 then b[6], b[8] = b[8], 0
elseif t == 3 then b[4], b[1] = b[1], 0 end
elseif fl == 8 then b[t] = 5 * s.turn end
s.castling = s.castling & (CR_M[f] or 15) & (CR_M[t] or 15)
s.ep_sq = (fl == 1) and ((f + t) >> 1) or 0
s.halfmove = (fp == 1 or fp == -1 or tp ~= 0) and 0 or (s.halfmove + 1)
if s.turn == -1 then s.fullmove = s.fullmove + 1 end
s.turn = -s.turn
return undo
end
function unmake_move(s, m, undo)
local pt = -s.turn
local b = s.board
local f, t, fl = m & 127, (m >> 7) & 127, m >> 14
b[f] = (fl == 8) and pt or b[t]
b[t] = (undo & 15) - 7
if fl == 2 then b[t + (pt == 1 and 8 or -8)] = -pt
elseif fl == 4 then
if t == 63 then b[64], b[62] = b[62], 0
elseif t == 59 then b[57], b[60] = b[60], 0
elseif t == 7 then b[8], b[6] = b[6], 0
elseif t == 3 then b[1], b[4] = b[4], 0 end
end
if s.turn == -1 then s.fullmove = s.fullmove - 1 end
s.castling = (undo >> 4) & 15
s.ep_sq = (undo >> 8) & 127
s.halfmove = (undo >> 15) & 127
s.turn = pt
end
function gen_moves(s, filter_sq, out_pool, first_only)
local mv, mc = out_pool or {}, 0
local b, turn = s.board, s.turn
local s_sq, e_sq = filter_sq or 1, filter_sq or 64
for sq = s_sq, e_sq do
local p = b[sq]
if p * turn > 0 then
local t = p > 0 and p or -p
local r, c = ((sq - 1) >> 3) + 1, ((sq - 1) & 7) + 1
if t == 1 then
local fwd = (turn == 1) and -8 or 8
local f1 = sq + fwd
local nr = r + ((turn == 1) and -1 or 1)
if nr >= 1 and nr <= 8 then
local is_pr = (nr == 1 or nr == 8) and (8 << 14) or 0
if b[f1] == 0 then
mc = mc + 1; mv[mc] = sq | (f1 << 7) | is_pr
if r == ((turn == 1) and 7 or 2) and b[f1 + fwd] == 0 then
mc = mc + 1; mv[mc] = sq | ((f1 + fwd) << 7) | (1 << 14)
end
end
local c1 = f1 - 1
if c > 1 then
if b[c1] * turn < 0 then mc = mc + 1; mv[mc] = sq | (c1 << 7) | is_pr
elseif c1 == s.ep_sq then mc = mc + 1; mv[mc] = sq | (c1 << 7) | (2 << 14) end
end
local c2 = f1 + 1
if c < 8 then
if b[c2] * turn < 0 then mc = mc + 1; mv[mc] = sq | (c2 << 7) | is_pr
elseif c2 == s.ep_sq then mc = mc + 1; mv[mc] = sq | (c2 << 7) | (2 << 14) end
end
end
elseif t == 2 then
for i = 1, 8 do
local to = sq + KD[i]
if to >= 1 and to <= 64 then
local dc = (((to - 1) & 7) + 1) - c
if dc >= -2 and dc <= 2 and b[to] * turn <= 0 then
mc = mc + 1; mv[mc] = sq | (to << 7)
end
end
end
elseif t >= 3 then
local s_idx = (t == 3) and 5 or 1
local e_idx = (t == 4) and 4 or 8
local max_step = (t == 6) and 1 or 7
for i = s_idx, e_idx do
local d = DIRS[i]
local to, prev_c, step = sq, c, 0
while step < max_step do
to = to + d
if to < 1 or to > 64 then break end
local tc = ((to - 1) & 7) + 1
local dc = tc - prev_c
if dc < -1 or dc > 1 then break end
prev_c = tc
local dp = b[to]
if dp == 0 then mc = mc + 1; mv[mc] = sq | (to << 7)
else
if dp * turn < 0 then mc = mc + 1; mv[mc] = sq | (to << 7) end
break
end
step = step + 1
end
end
if t == 6 then
local base = (turn == 1) and 56 or 0
local k_sq, opp = base + 5, -turn
if (s.castling & ((turn == 1) and 1 or 4)) ~= 0 and b[base + 6] == 0 and b[base + 7] == 0 and
not is_attacked(b, k_sq, opp) and not is_attacked(b, base + 6, opp) and not is_attacked(b, base + 7, opp) then
mc = mc + 1; mv[mc] = k_sq | ((base + 7) << 7) | (4 << 14)
end
if (s.castling & ((turn == 1) and 2 or 8)) ~= 0 and b[base + 4] == 0 and b[base + 3] == 0 and b[base + 2] == 0 and
not is_attacked(b, k_sq, opp) and not is_attacked(b, base + 3, opp) and not is_attacked(b, base + 4, opp) then
mc = mc + 1; mv[mc] = k_sq | ((base + 3) << 7) | (4 << 14)
end
end
end
end
end
local lc = 0
for i = 1, mc do
local m = mv[i]
local undo = make_move(s, m)
if not in_check(s.board, -s.turn) then
lc = lc + 1; mv[lc] = m
if first_only then
unmake_move(s, m, undo)
for j = lc + 1, #mv do mv[j] = nil end
return mv
end
end
unmake_move(s, m, undo)
end
for i = lc + 1, #mv do mv[i] = nil end
return mv
end
