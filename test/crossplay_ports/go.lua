local M=dofile('core.lua')
for _,n in ipairs({9,13,19}) do
 local g=M.new(123,n)
 for i=1,n*n*5 do if g.phase~=0 then break end;assert(M.play(g,M.ai(g)));assert(M.valid(g));assert(M.decode(M.encode(g))) end
 assert(g.phase==1);M.accept(g,1);M.accept(g,2);assert(g.phase==2)
end
