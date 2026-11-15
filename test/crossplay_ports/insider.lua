assert(math.maxinteger==2147483647 and 16777216.0+1==16777216.0)
local M=dofile('core.lua');local g=M.new(123);local seen={};local absent=0
for i=1,560 do
 M.deal(g);assert(not seen[g.word]);seen[g.word]=true
 assert(M.word(g.word):sub(1,1)~='[');assert(M.decode(M.encode(g)))
 local master=g.roles:find('M');assert(master and not g.roles:find('M',master+1))
 local insider=g.roles:find('I') or 0;if insider==0 then absent=absent+1 end
 g.phase=5;assert(M.vote(g,insider));assert(g.result==1)
end
assert(absent>50 and absent<250);M.deal(g);assert(g.word>0)
g.phase=3;M.tick(g,299999);assert(g.left==1);M.tick(g,1);assert(g.result==3);local n=g.rounds;M.tick(g,10);assert(g.rounds==n)
assert(not M.decode(M.encode(g):gsub('MMMM','CCCC')) or g.roles~='MMMM')
assert(not M.decode('garbage'))
g=M.new(5);M.deal(g);g.phase=2;M.pause(g);assert(g.phase==1);g.phase=3;M.pause(g);M.tick(g,10000);assert(g.left==300000)
print('Insider: 560 distinct words, role bag, vote, timeout, pause, serialization')
