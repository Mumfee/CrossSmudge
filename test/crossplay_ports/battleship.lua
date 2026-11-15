local M=dofile('core.lua')
for seed=1,20 do
 local g=M.new(seed);assert(M.decode(M.encode(g)))
 for row=1,5 do assert(M.action(g,1,row)) end
 assert(g.phase==1);assert(not M.action(g,0,1))
 while g.status==0 do
  local i
  if g.turn==1 then for j=1,100 do if g.sb:sub(j,j)=='0' then i=j;break end end else i=M.choose(g) end
  local seat=g.turn;assert(i and M.fire(g,i));local turn=g.turn;g.turn=seat;assert(not M.fire(g,i));g.turn=turn
  assert(M.decode(M.encode(g)))
 end
end
print('Battleship: placement, fleet geometry, 20 full games, save validation')
-- Changing an unseen fleet cannot change the gunner's choice.
local g=M.new(42);for row=1,5 do assert(M.action(g,1,row)) end
local first=M.choose(g);local original=g.a;g.a=g.b;assert(M.choose(g)==first);g.a=original
