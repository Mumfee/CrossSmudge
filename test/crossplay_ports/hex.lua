local M=dofile('core.lua');local g=M.new();assert(M.distance(g,1)==11 and M.distance(g,2)==11)
for y=1,10 do assert(M.action(g,1,y));assert(M.action(g,11,y)) end
assert(M.action(g,1,11) and g.status==1);assert(M.decode(M.encode(g)))
assert(not M.action(g,2,2))
g=M.new();for y=1,10 do g.b[(y-1)*11+6]=1 end;g.turn=1
local s=M.begin(g);local done,move
repeat done,move=M.step(s) until done
assert(move==115 or move==116);assert(g.b[move]==0);g.b[move]=1;assert(M.distance(g,1)==0)
-- Full boards cannot draw: enumerate deterministic legal random games.
for seed=1,100 do
 g=M.new();local r=seed
 while g.status==0 do r=(r*1103515245+12345)&0x7fffffff;local i=r%121+1
  while g.b[i]~=0 do i=i%121+1 end
  assert(M.action(g,(i-1)%11+1,(i-1)//11+1))
 end
 assert(M.decode(M.encode(g)))
end
print('Hex: connections, no-draw games, search tactics, saves')
