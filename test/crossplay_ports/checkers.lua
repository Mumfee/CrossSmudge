local M=dofile('core.lua');local g=M.new();assert(#M.moves(g)==7);assert(M.decode(M.encode(g)))
-- Mandatory two-jump chain: light 42 -> 28 -> 14, capturing 35,21.
for i=1,64 do g.b[i]=0 end
g.b[42]=1;g.b[35]=2;g.b[21]=2;g.b[64]=2
local moves=M.moves(g);assert(#moves==1 and #moves[1]==4 and moves[1]:byte(2)==14)
assert(not M.play(g,string.char(42,28,35)));assert(M.play(g,moves[1]));assert(g.b[35]==0 and g.b[21]==0)
-- Promotion stops the man, even when a backward capture could follow.
g=M.new();for i=1,64 do g.b[i]=0 end;g.b[19]=1;g.b[12]=2;g.b[14]=2
moves=M.moves(g);assert(#moves==1 and #moves[1]==3);assert(M.play(g,moves[1]));assert(g.b[5]==3 and g.b[14]==2)
for game=1,6 do
 g=M.new();local n=0
 while g.status==0 and n<500 do
  local m=M.choose(g);assert(m and M.play(g,m));assert(M.decode(M.encode(g)));n=n+1
 end
 assert(g.status~=0)
end
assert(not M.decode('1,0,'..string.rep('1',64)))
print('Checkers: mandatory full jumps, crowning, six AI games and saves')
-- Original regression fixtures: nine captures and a king returning to origin.
g=M.new();for i=1,64 do g.b[i]=0 end;g.b[2]=3
for _,y in ipairs({1,3,5}) do for _,x in ipairs({2,4,6}) do g.b[y*8+x+1]=2 end end
local longest=0
for _,m in ipairs(M.moves(g)) do longest=math.max(longest,#m-2) end
assert(longest==9)
g=M.new();for i=1,64 do g.b[i]=0 end;g.b[27]=3
for _,i in ipairs({20,36,22,38}) do g.b[i]=2 end
local cycle
for _,m in ipairs(M.moves(g)) do if m:byte(1)==m:byte(2) then cycle=m;break end end
assert(cycle and M.play(g,cycle) and g.b[27]==3)
-- A returning king must also be playable through the two-click UI action.
g=M.new();for i=1,64 do g.b[i]=0 end;g.b[27]=3
for _,i in ipairs({20,36,22,38}) do g.b[i]=2 end
assert(M.action(g,3,4));assert(M.action(g,3,4));assert(g.status==1 and g.b[27]==3)
