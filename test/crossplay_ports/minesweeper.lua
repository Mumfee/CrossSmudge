local M=dofile('core.lua')
for seed=1,50 do
 local g=M.new(seed); assert(M.action(g,4,5,false));assert(g.status==1 or g.status==2)
 local mines=0
 for i=1,80 do
  if g.b[i]&1~=0 then mines=mines+1 end
  if g.b[i]&2~=0 and M.adjacent(g,i,1)==0 then
   for _,j in ipairs(M.neighbors(i)) do assert(g.b[j]&2~=0) end
  end
 end
 assert(mines==10 and M.adjacent(g,36,1)==0 and g.b[36]&1==0)
 local restored=M.decode(M.encode(g));assert(restored and M.encode(restored)==M.encode(g))
 assert(not M.decode('garbage'))
end
local g=M.new(1);assert(M.action(g,1,1,true));assert(not M.action(g,1,1,false));assert(g.status==0)
M.action(g,1,1,true);M.action(g,1,1,false)
for i=1,80 do if g.b[i]&1==0 then M.action(g,(i-1)%8+1,(i-1)//8+1,false) end end
assert(g.status==2);assert(not M.action(g,1,1,true))
assert(not M.decode('1|0|0'))
print('minesweeper rules OK')
