local M=dofile('core.lua')
for map=0,9 do
 local g=M.new(123,map)
 for i=1,200 do if g.phase==1 then break end;assert(M.apply(g,M.ai(g)));assert(M.valid(g));assert(M.decode(M.encode(g))) end
 assert(g.phase==1)
end
