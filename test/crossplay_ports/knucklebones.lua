local M=dofile('core.lua')
local g=M.new(1);g.b[1]=3;g.b[2]=3;g.b[3]=5;assert(M.column_score(g,1,1)==17)
g.b[10]=3;g.b[11]=3;g.turn=1;g.die=3;g.b[3]=0
assert(M.action(g,1,1));assert(g.b[10]==0 and g.b[11]==0);assert(M.column_score(g,1,1)==27)
for seed=1,100 do
 local h=M.new(seed)
 for i=1,500 do
  if h.status~=0 then break end
  local snapshot=M.encode(h);local c=M.choose(h);assert(M.encode(h)==snapshot)
  assert(M.action(h,c,1));assert(M.decode(M.encode(h)))
 end
 assert(h.status~=0 and h.die==0)
end
assert(not M.decode('garbage'))
print('knucklebones rules OK')
