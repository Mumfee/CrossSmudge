local M=dofile('core.lua');local g=M.new(123)
for i=1,2000 do
 if g.phase==3 then break end
 if g.phase==2 then g.round=g.round+1;M.deal(g,g.seed+1,g.ender~=0 and 3-g.ender or 3-g.starter) else assert(M.apply(g,M.ai(g))) end
 assert(M.valid(g));assert(M.decode(M.encode(g)))
end
assert(g.phase==3)
