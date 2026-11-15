local M=dofile('core.lua')
for id=1,64 do
 local g=M.new(123,id);local p=M.case(g)
 for _,clue in ipairs(p.clues) do assert(M.holds(clue,p.solution,p.murder)) end
 for c=1,p.cats do g.picks[c]=p.solution[c][p.murder] end
 assert(M.accuse(g));assert(g.phase==1);assert(M.decode(M.encode(g)))
end
