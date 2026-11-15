local M=dofile('core.lua')
local g=M.new(1)
for _,c in ipairs({1,2,1,2,1,2,1})do assert(M.action(g,c,1))end
assert(g.status==1);assert(not M.action(g,3,1))
assert(M.decode(M.encode(g)))
local h=M.new(1)
for _,c in ipairs({1,2,1,2,3,2,1})do M.action(h,c,1)end
assert(M.choose(h)==2,'must take own win before blocking')
local h=M.new(1)
for _,c in ipairs({1,2,1,2,1,3})do M.action(h,c,1)end
assert(M.choose(h)==1,'must win vertically')
local h=M.new(1)
for _,c in ipairs({1,2,1,3,1})do M.action(h,c,1)end
assert(M.choose(h)==1,'must block immediate win')
for seed=1,20 do
 local h=M.new(seed)
 for i=1,42 do if h.status~=0 then break end
  local c=(i*seed)%7+1
  while not M.action(h,c,1)do c=c%7+1 end
  assert(M.decode(M.encode(h)))
 end
 assert(h.status~=0)
end
assert(not M.decode('1122334455667788'))
print('connect four rules OK')
