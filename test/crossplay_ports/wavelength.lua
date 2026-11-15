local M=dofile('core.lua')
for guess=1,20 do for target=1,20 do for mode=1,2 do for call=1,2 do
 local d=math.abs(guess-target);local expected=({[0]=5,[1]=3,[2]=1})[d] or 0
 if mode==2 and ((target>guess and call==1) or (target<guess and call==2)) then expected=expected+1 end
 assert(M.score(guess,target,call,mode)==expected)
end end end end
local g=M.new(42)
for i=1,M.count do assert(M.word(i):sub(1,1)~='[') end
for i=1,M.count+2 do
 M.deal(g);assert(g.a~=g.b);assert(M.decode(M.encode(g)));M.act(g,1);assert(g.phase==2);M.act(g,1);assert(g.phase==3)
 g.guess=g.target;M.record(g,1);assert(g.points==(i==1 and 0 or 5));assert(M.decode(M.encode(g)))
end
assert(g.total==(M.count+1)*5)
print('Wavelength: exhaustive 1600 scoring cases, practice and 100 rounds')
