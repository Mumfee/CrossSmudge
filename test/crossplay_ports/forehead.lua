local M=dofile('core.lua');local g=M.new(42)
for c=1,17 do
 g.category=c;M.begin(g);g.phase=1;local seen={}
 for n=1,128 do
  assert(not seen[g.word]);seen[g.word]=true;assert(M.word(g.word):sub(1,1)~='[')
  assert(M.answer(g,n%2+1));assert(M.decode(M.encode(g)))
 end
 assert(g.phase==3 and #g.history==640 and g.got==64)
end
assert(g.rounds==17 and g.total==1088 and #g.recent==48)
M.begin(g);g.phase=1;M.tick(g,g.left);assert(g.phase==3 and g.history:sub(-1)=='3');local rounds=g.rounds;M.tick(g,100);assert(rounds==g.rounds)
M.begin(g);M.pause(g);local left=g.left;M.tick(g,100);assert(g.left==left)
assert(not M.decode('garbage'))
-- Read all genuine content through bounded block loader.
for i=1,2474 do assert(M.word(i):sub(1,1)~='[') end
print('Forehead: full deck; 17 maximum rounds, history, score, timeout, pause, saves')
