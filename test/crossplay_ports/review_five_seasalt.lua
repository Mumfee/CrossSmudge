local M=dofile('core.lua')
-- Human revealing their own hand must not reveal the opponent's final-turn hand.
local g=M.new(2468);g.phase=1;g.ender=1;g.revealed=1;g.place[50]=2;g.ui=1
local _,o=M.view(g);for _,v in ipairs(o) do assert(not v:find('#50',1,true),'private opponent card exposed') end
g.revealed=3;_,o=M.view(g);local found=false;for _,v in ipairs(o) do if v:find('#50',1,true) then found=true end end;assert(found,'revealed card missing')
-- After a scored round, the player AFTER the caller starts, not necessarily after starter.
for starter=1,2 do for ender=0,2 do
 g=M.new(2468);g.phase=2;g.starter=starter;g.ender=ender;M.act(g,1)
 assert(g.turn==(ender==0 and 3-starter or 3-ender),'wrong next-round starter')
end end
-- Last chance ties favour caller; successful bet pays caller cards+colour, foe colour only.
g=M.new(2468);for c=1,58 do g.place[c]=0 end;g.place[1]=1;g.place[2]=1;g.place[3]=2;g.place[4]=2;g.ender=1;g.bet=1
local p1,c1=M.points(g,1);local p2,c2=M.points(g,2);assert(p1==p2);M.bank(g);assert(g.score[1]==p1+c1 and g.score[2]==c2)
print('independent Sea Salt privacy/round checks passed')

local fresh=M.new(91);assert(not M.apply(fresh,{8}),'mandatory take cannot be skipped');local n=0;M.moves(fresh,function()n=n+1 end);assert(n==#M.moves(fresh),'callback/list legal moves agree')
-- List and callback enumeration must enumerate exactly the same moves.
local function checkMoves(g)
 local listed=M.moves(g);local at=0
 M.moves(g,function(m)
  at=at+1;local expected=listed[at];assert(expected,'callback emitted extra move')
  assert(m[1]==expected[1] and m[2]==expected[2] and m[3]==expected[3],'callback move mismatch')
 end)
 assert(at==#listed,'callback omitted move')
end
g=M.new(123);checkMoves(g);local before=M.encode(g);assert(not M.apply(g,{8}),'mandatory take skipped');assert(M.encode(g)==before)
for i=1,80 do
 checkMoves(g);if g.phase>=2 then break end
 local move=M.ai(g);assert(move and M.apply(g,move))
end
print('callback/list enumeration agrees; mandatory take enforced')
