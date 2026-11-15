local M=dofile('core.lua')
local g=M.new(123)
for i=1,600 do
 if g.phase==2 then break end
 if g.phase==1 then local w=M.winner(g);g.round=g.round+1;M.deal(g,g.seed+1,w>0 and 3-w or g.starter) else assert(M.apply(g,M.ai(g))) end
 assert(M.valid(g));assert(M.decode(M.encode(g)))
end
assert(g.phase==2)
local x=M.new(1);x.hands[1][1]=0;assert(not M.legal(x,{4,1,1}))
