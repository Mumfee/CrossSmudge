-- Independent Go review: captures before suicide, ko, situational history, scoring.
local M=dofile('core.lua')
local g=M.new(123,9)
g.b=M.put(g.b,2,2);g.b=M.put(g.b,10,2)
assert(not M.play(g,1),'suicide allowed')
g.b=M.put(g.b,3,1);g.b=M.put(g.b,11,1);g.b=M.put(g.b,19,1)
assert(M.play(g,1),'capturing move incorrectly called suicide');assert(M.at(g.b,2)==0 and M.at(g.b,10)==0 and g.captured[1]==2)
-- Single capture by an isolated stone with one liberty is simple ko.
g=M.new(123,9);g.b=M.put(g.b,21,2);g.b=M.put(g.b,12,1);g.b=M.put(g.b,20,1);g.b=M.put(g.b,30,1);g.b=M.put(g.b,13,2);g.b=M.put(g.b,23,2);g.b=M.put(g.b,31,2)
assert(M.play(g,22));assert(g.ko==21);local encoded=M.encode(g);assert(not M.play(g,21));assert(M.encode(g)==encoded)
-- Repetition includes the next side to move, not just the board.
g=M.new(123,9);local b=M.put(string.rep('0',81),1,1)
g.history={M.key(b,2)};assert(not M.play(g,1));g.history={M.key(b,1)};assert(M.play(g,1))
-- Source's deliberately bounded recent-position ring.
for i=2,12 do assert(M.play(g,i)) end;assert(#g.history==8)
-- Two passes enter agreement; both players must accept, changing marks withdraws both.
g=M.new(123,9);assert(M.play(g,1));assert(M.play(g,0));assert(M.play(g,0));assert(g.phase==1)
local a,bscore=M.score(g);assert(a==162 and bscore==15)
assert(M.accept(g,1));assert(g.phase==1);assert(M.mark(g,1));assert(g.accepted==0)
a,bscore=M.score(g);assert(a==0 and bscore==15);assert(M.accept(g,1));assert(M.accept(g,2));assert(g.phase==2);assert(not M.play(g,2))
-- Empty regions bordering both colours are neutral.
g=M.new(123,9);g.b=M.put(g.b,1,1);g.b=M.put(g.b,81,2);a,bscore=M.score(g);assert(a==2 and bscore==17)
-- Five-board-area move-limit house rule enters scoring even without two passes.
g=M.new(123,9);g.moves=404;assert(M.play(g,1));assert(g.phase==1 and g.moves==405)
-- All exposed board sizes and handicaps, score/codec under caller's memory cap.
for _,n in ipairs({9,13,19}) do
 for _,handicap in ipairs({0,2,3,4,5}) do
  g=M.new(456,n,handicap);local stones=0;for i=1,#g.b do if M.at(g.b,i)==1 then stones=stones+1 end end
  assert(stones==handicap and g.turn==(handicap==0 and 1 or 2));assert(g.komi==(handicap==0 and 15 or 1))
  local s=M.encode(g);local restored=M.decode(s);assert(restored,'roundtrip failed on board '..n);assert(M.encode(restored)==s)
  g=nil;restored=nil;collectgarbage('collect')
 end
end
print('independent Go rule/roundtrip checks passed')
