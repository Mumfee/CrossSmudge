local M=dofile('core.lua')
for seed=1,12 do
 local g=M.new(seed)
 while g.phase~=4 do
  if g.phase==0 then assert(M.commit(g,{M.passpick(g,1),M.passpick(g,2),M.passpick(g,3),M.passpick(g,4)}))
  elseif g.phase==1 then local c=M.choose(g);assert(c and M.play(g,c))
  elseif g.phase==2 then assert(M.sweep(g))
  elseif g.phase==3 then g.handno=g.handno+1;M.deal(g) end
  assert(M.decode(M.encode(g)))
 end
 assert(g.t1>=100 or g.t2>=100 or g.t3>=100 or g.t4>=100)
end
local g=M.new(1);g.board=string.rep('5',52);g.number=13;M.score(g);assert(g.t1==0 and g.t2==26 and g.t3==26 and g.t4==26)
assert(M.penalty(37)==13 and M.penalty(40)==1 and M.penalty(1)==0)
assert(not M.decode('garbage'))
print('Hearts: 12 complete matches; pass rotation, legal play, 100-point end, moon, saves')
-- Focused rules with controlled hands; 2 is low, Ace is high.
local function pos(cards,trick,number,broken)
 local p=M.new(1);p.board=string.rep('8',52);p.phase=1;p.turn=1;p.leader=1;p.trick=trick or '';p.number=number or 1;p.broken=broken or 0
 for _,c in ipairs(cards) do p.board=p.board:sub(1,c-1)..'1'..p.board:sub(c+1) end;return p
end
g=pos({1,2,40},'',0);assert(M.legal(g,1) and not M.legal(g,2))
g=pos({2,40},string.char(1));assert(M.legal(g,2) and not M.legal(g,40))
g=pos({14,40},string.char(1),0);assert(M.legal(g,14) and not M.legal(g,40))
g=pos({37,40},string.char(1),0);assert(M.legal(g,37) and M.legal(g,40))
g=pos({14,40});assert(not M.legal(g,40));g=pos({40,41});assert(M.legal(g,40))
g=pos({},string.char(12,13,1,2));assert(M.winner(g)==2)
for handno=0,2 do
 g=M.new(42);g.handno=handno;local passes={}
 for seat=1,4 do passes[seat]=M.passpick(g,seat) end
 assert(M.commit(g,passes));local offset=({1,3,2})[handno+1]
 for seat=1,4 do for i=1,3 do local c=passes[seat]:byte(i);assert(tonumber(g.board:sub(c,c))==(seat-1+offset)%4+1) end end
end
