-- Independent semantic-save review. A valid checksum must not admit a wrong type.
local M=dofile('core.lua')
local g=M.new(2468)
local original=M.encode(g);assert(M.decode(original))
for key,value in pairs(g) do
 if type(value)=='number' then
  g[key]={};local restored=M.decode(M.encode(g));g[key]=value
  assert(not restored,'accepted table in scalar '..key)
  g[key]='bad';restored=M.decode(M.encode(g));g[key]=value
  assert(not restored,'accepted string in scalar '..key)
 elseif type(value)=='table' then
  g[key]=123;local restored=M.decode(M.encode(g));g[key]=value
  assert(not restored,'accepted scalar in table '..key)
 elseif type(value)=='string' then
  g[key]=123;local restored=M.decode(M.encode(g));g[key]=value
  assert(not restored,'accepted scalar in string '..key)
 end
 collectgarbage('collect')
end
assert(M.encode(g)==original)
print('independent save field-type checks passed')
