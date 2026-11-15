-- CrossPlay rules and one-ply opponent translated; see LICENSE-CrossPlay.txt.
local M={title='Knucklebones',cols=3,rows=6}
local function roll(g)
 local s=g.rng;if s==0 then s=0x9e3779b9 end
 s=(s~(s<<13))&0xffffffff;s=s~(s>>17);s=(s~(s<<5))&0xffffffff
 g.rng=s;g.die=((s&0x7fffffff)%6+(s<0 and 2 or 0))%6+1
end
function M.new(seed)
 local g={b={},rng=seed&0xffffffff,turn=1,status=0,die=0}
 for i=1,18 do g.b[i]=0 end;roll(g);return g
end
local function base(seat,col)return (seat-1)*9+(col-1)*3 end
local function count(g,seat,col)
 local b=base(seat,col);local n=0
 for r=1,3 do if g.b[b+r]~=0 then n=n+1 end end;return n
end
function M.column_score(g,seat,col)
 local b=base(seat,col);local score=0
 for r=1,3 do
  local v=g.b[b+r]
  for j=1,3 do if v==g.b[b+j] then score=score+v end end
 end;return score
end
function M.score(g,seat)
 local n=0;for c=1,3 do n=n+M.column_score(g,seat,c) end;return n
end
local function full(g,seat)
 for c=1,3 do if count(g,seat,c)<3 then return false end end;return true
end
function M.action(g,c)
 if g.status~=0 or c<1 or c>3 or g.die<1 or g.die>6 then return false end
 local n=count(g,g.turn,c);if n==3 then return false end
 g.b[base(g.turn,c)+n+1]=g.die
 local b=base(3-g.turn,c);local k=1
 for r=1,3 do local v=g.b[b+r];if v~=0 and v~=g.die then g.b[b+k]=v;k=k+1 end end
 for r=k,3 do g.b[b+r]=0 end
 if full(g,1) or full(g,2) then
  local a,z=M.score(g,1),M.score(g,2);g.status=a==z and 3 or (a>z and 1 or 2);g.die=0
 else g.turn=3-g.turn;roll(g) end
 return true
end
function M.choose(g)
 if g.status~=0 then return nil end
 local hash=0x811c9dc5
 for i=1,18 do hash=((hash~g.b[i])*16777619)&0xffffffff end
 hash=((hash~(g.turn-1))*16777619)&0xffffffff;hash=((hash~g.die)*16777619)&0xffffffff
 local best,value,room=nil,-999,-1
 for step=0,2 do
  local c=((hash&0x7fffffff)%3+(hash<0 and 2 or 0)+step)%3+1;local n=count(g,g.turn,c)
  if n<3 then
   -- Only this column changes. Score it without reading future RNG.
   local b=base(g.turn,c);local mine=M.column_score(g,g.turn,c)
   local copies=0;for r=1,3 do if g.b[b+r]==g.die then copies=copies+1 end end
   mine=mine+g.die*(2*copies+1)
   local ob=base(3-g.turn,c);copies=0
   for r=1,3 do if g.b[ob+r]==g.die then copies=copies+1 end end
   local gain=mine-M.column_score(g,g.turn,c)+g.die*copies*copies
   if gain>value or (gain==value and 3-n>room) then best,value,room=c,gain,3-n end
  end
 end
 return best
end
function M.encode(g)
 local s={tostring(g.rng),'|',tostring(g.turn),tostring(g.die),tostring(g.status),'|'}
 for i=1,18 do s[#s+1]=tostring(g.b[i]) end;return table.concat(s)
end
function M.decode(s)
 if type(s)~='string' or #s>35 then return nil end
 local rng,t,d,st,b=s:match('^(-?%d+)|([12])([0-6])([0-3])|([0-6]+)$')
 rng=tonumber(rng);if not b or #b~=18 or not rng or math.type(rng)~='integer' then return nil end
 local g=M.new(rng);g.rng=rng;g.turn=tonumber(t);g.die=tonumber(d);g.status=tonumber(st)
 for i=1,18 do g.b[i]=tonumber(b:sub(i,i)) end
 for seat=1,2 do for c=1,3 do
  local gap=false;local off=base(seat,c)
  for r=1,3 do if g.b[off+r]==0 then gap=true elseif gap then return nil end end
 end end
 local over=full(g,1) or full(g,2)
 if over then
  local a,z=M.score(g,1),M.score(g,2);local expected=a==z and 3 or (a>z and 1 or 2)
  if g.status~=expected or g.die~=0 then return nil end
 elseif g.status~=0 or g.die==0 then return nil end
 return g
end
function M.label(g,i)
 -- UI rows show player 1 above player 2, columns preserve matching lanes.
 local row=(i-1)//3;local c=(i-1)%3+1
 local index=(row//3)*9+(c-1)*3+row%3+1
 return g.b[index]==0 and '.' or tostring(g.b[index])
end
function M.message(g)
 local scores='P1 '..M.score(g,1)..' / P2 '..M.score(g,2)
 if g.status~=0 then return scores..(g.status==3 and ' - Draw' or ' - P'..g.status..' wins') end
 return scores..'  P'..g.turn..' roll: '..g.die
end
M.help={'Choose a column for the shown die.', 'Top grid is P1; bottom grid is P2.',
 'Matching values in the enemy column are destroyed.', 'Each value scores value x copies x copies per column.',
 'The game ends as soon as either grid fills.', 'Solo uses the original deterministic one-ply strategy.',
 'Two players share this device. Nearby is unavailable.'}
return M
