-- Independent rule review fixtures; use synthetic positions to isolate each rule.
local M=dofile('core.lua')
local function fixture()
 local g=M.new(2468,1)
 g.rack={{0,0,0,0,0,0,0,0},{1,0,0,0,0,0,0,0}}
 g.taken={20,19};g.discard={{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}}
 g.slots={};g.tiles={};return g
end
local function put(g,p,s,k)g.slots[#g.slots+1]=p;g.tiles[#g.tiles+1]=(s-1)*8+k-1 end
local function st(p,k,e,target,base,from,to,bkind)return {p,k,e or 0,target or 0,base or 0,from or 0,to or 0,bkind or 1} end
local function unchanged(g,steps)local before=M.encode(g);assert(not M.apply(g,steps));assert(M.encode(g)==before,'illegal move mutated state') end
-- Both joker branches: Kwak covers Roxy; weakest troop covers Kwak.
local g=fixture();g.rack[1][1]=1;put(g,1,2,8);assert(M.apply(g,{st(1,1)}));assert(M.top(g,1)==1)
g=fixture();g.rack[1][2]=1;put(g,1,2,1);assert(M.apply(g,{st(1,2)}))
-- Hook waives connectivity, but only when its effect is selected.
g=fixture();g.rack[1][5]=1;unchanged(g,{st(15,5)});assert(M.apply(g,{st(15,5,1)}))
-- A Cap'n chain commits together and allows a newly connected second placement.
g=fixture();g.rack[1][3]=1;g.rack[1][8]=1;unchanged(g,{st(1,3,1)});assert(M.apply(g,{st(1,3,1),st(2,8)}));assert(M.top(g,2)==1)
g=fixture();g.rack[1][3]=1;g.rack[1][8]=1;unchanged(g,{st(1,3,1),st(15,8)})
-- Jumbo removes only the visible enemy, revealing the buried owner.
g=fixture();g.rack[1][4]=1;put(g,2,1,2);put(g,2,2,8);assert(M.apply(g,{st(1,4,1,2)}));assert(M.top(g,2)==1 and g.discard[2][8]==1)
-- Nullify disables Hook's waiver, but permits connected Hook with effect declined.
g=fixture();g.rack[1][5]=1;unchanged(g,{st(5,5,1)});for p=1,4 do put(g,p,1,8) end;unchanged(g,{st(5,5,1)});assert(M.apply(g,{st(5,5)}))
-- Gate permits only Star/Roxy on proving-ground slot 10, including Hook.
g=fixture();g.rack[1][5]=1;unchanged(g,{st(10,5,1)});g.rack[1][8]=1;put(g,6,1,8);put(g,7,1,8);put(g,8,1,8);put(g,9,1,8);assert(M.apply(g,{st(10,8)}))
-- Recall cannot recall the just-placed troop; valid recall returns another own top.
g=fixture();g.rack[1][5]=1;put(g,1,1,8);unchanged(g,{st(3,5,1,0,1,3)});assert(M.apply(g,{st(3,5,1,0,1,1)}));assert(g.rack[1][8]==1 and M.top(g,1)==0)
-- Exhume returns precisely the selected discarded troop.
g=fixture();g.rack[1][5]=1;g.discard[1][8]=1;assert(M.apply(g,{st(4,5,1,0,1,0,0,8)}));assert(g.rack[1][8]==1 and g.discard[1][8]==0)
-- Draw base takes one reserve troop after the troop effect.
g=fixture();g.rack[1][5]=1;g.taken[1]=19;assert(M.apply(g,{st(8,5,1,0,1)}));assert(g.taken[1]==20)
-- Suppress lasts through exactly the victim's next turn.
g=fixture();g.rack[1][5]=1;assert(M.apply(g,{st(9,5,1,0,1)}));assert(g.frozen[2]==1);assert(M.apply(g,{}));assert(g.frozen[2]==0)
-- Shove moves an adjacent enemy to its neighbour, ignores normal placement rules.
g=fixture();g.rack[1][5]=1;put(g,12,2,8);put(g,11,1,8);assert(M.apply(g,{st(13,5,1,0,1,12,11)}));assert(M.top(g,12)==0 and M.top(g,11)==2)
print('independent Toy Battle effect/atomicity checks passed')
-- Tropical Pool also gates HQs, so those public restrictions must be visible.
g=nil;collectgarbage('collect');g=M.new(123,7);g.ui=1
local _,options=M.view(g);local slot,allowed=0,0
for _,label in ipairs(options) do
 local p=label:match('^(%d+):');if p then slot=tonumber(p) end
 if (slot==15 or slot==16) and label:match('^Allowed:') then allowed=allowed+1 end
end
assert(allowed==6,'gated HQ restrictions missing from inspector')
print('gated HQ inspector passed')
