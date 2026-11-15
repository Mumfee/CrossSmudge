-- Accusations use the killer's category picks, independently of pencil notes.
local M=dofile('core.lua')
for _,id in ipairs({1,17,33,49}) do
 local g=M.new(123,id);local p=M.case(g)
 for c=1,p.cats do g.picks[c]=p.solution[c][p.murder] end
 local good=g.picks[2];g.picks[2]=good%p.n+1
 assert(not M.accuse(g) and g.phase==0 and g.attempts==1)
 g.picks[2]=good;assert(M.accuse(g) and g.phase==1 and g.attempts==2)
 for _,mark in ipairs(g.marks) do assert(mark==0,'accusation rewrote notes') end
 local saved=M.encode(g);assert(M.encode(assert(M.decode(saved)))==saved)
 g=nil;p=nil;collectgarbage('collect')
end
print('independent Murdle accusation checks passed')
