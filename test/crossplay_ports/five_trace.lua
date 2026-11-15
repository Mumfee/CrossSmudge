local M=dofile('core.lua');local U=dofile('util.lua');local cfg=dofile('trace-config.lua');local g=M.new(cfg.seed,cfg.arg);local input,expected={},{}
local function out(a)local s='';for i,x in ipairs(a) do s=s..(i>1 and ' ' or '')..x end;return s end
local function append(a,t)for _,v in ipairs(t) do a[#a+1]=v end end
for i=1,2500 do
 local m,row,line
 if cfg.app=='jaipur' then
  if g.phase==2 then break end
  if g.phase==1 then local seed=U.next(g.seed);line='9 '..tostring(seed);local w=M.winner(g);g.round=g.round+1;M.deal(g,seed,w>0 and 3-w or g.starter);row={g.phase,g.turn,g.at,M.score(g,1),M.score(g,2),g.seals[1],g.seals[2]}
  else m=M.ai(g);if i%7==0 then for mask=1,31 do local e={3,mask,{0,0,0,0,0,0,2}};if M.legal(g,e) then m=e;break end end end;assert(m);if m[1]==3 then local a={3,m[2],0};append(a,m[3]);line=out(a) else line=out({m[1],m[2] or 0,m[3] or 0}) end;assert(M.apply(g,m));row={1,g.phase,g.turn,g.at,M.score(g,1),M.score(g,2),g.seals[1],g.seals[2]} end
  append(row,g.market);append(row,g.hands[1]);append(row,g.hands[2]);append(row,g.herd)
 elseif cfg.app=='seasalt' then
  if g.phase==3 then break end
  if g.phase==2 then local seed=U.next(g.seed);line='10 '..seed..' 0';g.round=g.round+1;M.deal(g,seed,g.ender~=0 and 3-g.ender or 3-g.starter)
  else m=M.ai(g);assert(m);line=out({m[1],m[2] or 0,m[3] or 0});assert(M.apply(g,m)) end
  row={1,g.phase,g.step,g.turn,g.at,M.points(g,1),M.points(g,2),g.score[1],g.score[2]};append(row,g.place)
 elseif cfg.app=='go' then
  if g.phase~=0 then break end
  m=i>g.n*g.n*2 and 0 or M.ai(g);line=tostring(m);assert(M.play(g,m));local a,b=M.score(g);row={1,g.phase,g.turn,g.moves,a,b};for p=1,#g.b do row[#row+1]=M.at(g.b,p) end
 else
  if g.phase==1 then break end;m=M.ai(g);assert(m);local a={#m};for _,st in ipairs(m) do append(a,st) end;line=out(a);assert(M.apply(g,m));row={1,g.phase,g.turn,g.winner,g.ending,g.medals[1],g.medals[2]};append(row,g.rack[1]);append(row,g.rack[2]);row[#row+1]=#g.slots;for j=1,#g.slots do row[#row+1]=g.slots[j];row[#row+1]=g.tiles[j] end
 end
 assert(M.valid(g),'semantic validity at '..i);assert(M.decode(M.encode(g)),'round trip at '..i)
 -- Stream files instead of retaining whole-game traces under the hardware budget.
 assert(smudge.write_file('input.txt',line..'\n',true));assert(smudge.write_file('expected.txt',out(row)..'\n',true))
 if i==2500 then error('game failed to finish') end
end
assert(cfg.app=='go' and g.phase==1 or cfg.app=='jaipur' and g.phase==2 or cfg.app=='seasalt' and g.phase==3 or cfg.app=='toybattle' and g.phase==1,'complete game required')
