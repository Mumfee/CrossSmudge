local Y=dofile('core.lua')
assert(Y.raw({2,2,3,3,3},9)==25)
assert(Y.raw({3,3,3,3,3},9)==25)
assert(Y.raw({1,2,3,4,4},10)==30)
local g=Y.new(1);local card=g.cards[1];card[12]=50;g.d={3,3,3,3,3};g.rolls=1
assert(Y.allowed(card,g.d,3));assert(not Y.allowed(card,g.d,9));assert(Y.take(g,3));assert(card[14]==1)
card[12]=0;card[3]=15;assert(Y.allowed(card,{3,3,3,3,3},10));assert(Y.score_for(card,{3,3,3,3,3},10)==30)
assert(not Y.allowed(card,{3,3,3,3,3},1))
for seed=1,10 do
 local h=Y.new(seed)
 for i=1,26 do
  assert(Y.roll(h));assert(Y.hold(h,1));local d=h.d[1];assert(Y.roll(h));assert(h.d[1]==d)
  assert(Y.roll(h));assert(not Y.roll(h));assert(not Y.hold(h,2))
  local c=Y.choose_box(h.cards[h.turn],h.d);assert(c);assert(Y.take(h,c));assert(Y.decode(Y.encode(h)))
 end
 assert(Y.over(h));assert(not Y.roll(h));assert(not Y.take(h,1))
end
local h=Y.new(123);Y.roll(h);local before=Y.encode(h);local s=Y.begin(h)
for i=1,32 do local done,mask=Y.step(h,s);assert(done==(i==32));assert(mask>=0 and mask<32)end
assert(before==Y.encode(h));assert(not Y.decode('garbage'))
print('Yahtzee Joker/bonuses/holds/card completion/AI immutability OK')
