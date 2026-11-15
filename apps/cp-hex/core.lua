-- 11x11 Hex. Stock-sized shortest-path opponent, not the upstream RAVE engine.
local M={title='Hex',cols=11,rows=11}
local dx={-1,1,0,0,-1,1};local dy={0,0,-1,1,1,-1}
local function neighbors(i,fn)
 local x=(i-1)%11;local y=(i-1)//11
 for d=1,6 do local xx,yy=x+dx[d],y+dy[d];if xx>=0 and xx<11 and yy>=0 and yy<11 then fn(yy*11+xx+1) end end
end
function M.distance(g,seat)
 local dist,used={},{}
 for i=1,121 do
  local edge=seat==1 and i<=11 or seat==2 and (i-1)%11==0
  dist[i]=edge and (g.b[i]==seat and 0 or g.b[i]==0 and 1 or 999) or 999
 end
 for _=1,121 do
  local at,best=0,999
  for i=1,121 do if not used[i] and dist[i]<best then at=i;best=dist[i] end end
  if at==0 then return 999 end
  if (seat==1 and at>110) or (seat==2 and at%11==0) then return best end
  used[at]=true
  neighbors(at,function(j)if not used[j] and g.b[j]~=3-seat then dist[j]=math.min(dist[j],best+(g.b[j]==0 and 1 or 0)) end end)
 end
 return 999
end
function M.new()
 local g={b={},turn=1,status=0};for i=1,121 do g.b[i]=0 end;return g
end
function M.action(g,x,y)
 if g.status~=0 or x<1 or x>11 or y<1 or y>11 then return false end
 local i=(y-1)*11+x;if g.b[i]~=0 then return false end
 g.b[i]=g.turn;if M.distance(g,g.turn)==0 then g.status=g.turn else g.turn=3-g.turn end;return true
end
function M.begin(g)return {g=g,i=1,best=nil,value=-1000000,seat=g.turn} end
function M.step(s)
 local g=s.g
 while s.i<=121 and g.b[s.i]~=0 do s.i=s.i+1 end
 if s.i>121 then return true,s.best end
 local i=s.i;g.b[i]=s.seat;local mine=M.distance(g,s.seat);local theirs=M.distance(g,3-s.seat)
 g.b[i]=3-s.seat;local threat=M.distance(g,3-s.seat)==0;g.b[i]=0
 local x,y=(i-1)%11,(i-1)//11
 local v=mine==0 and 900000 or threat and 800000 or (theirs-mine)*100-mine*10-math.abs(x-5)-math.abs(y-5)
 if v>s.value then s.value=v;s.best=i end
 s.i=i+1;return false
end
function M.encode(g)local t={};for i=1,121 do t[i]=g.b[i] end;return g.turn..table.concat(t) end
function M.decode(s)
 if type(s)~='string' or #s~=122 or not s:match('^[12][012]+$') then return nil end
 local g=M.new();g.turn=tonumber(s:sub(1,1));local counts={0,0}
 for i=1,121 do g.b[i]=tonumber(s:sub(i+1,i+1));if g.b[i]>0 then counts[g.b[i]]=counts[g.b[i]]+1 end end
 if counts[1]<counts[2] or counts[1]>counts[2]+1 then return nil end
 local a,b=M.distance(g,1)==0,M.distance(g,2)==0
 if a and b then return nil end;g.status=a and 1 or b and 2 or 0
 if g.status==0 and g.turn~=(counts[1]==counts[2] and 1 or 2) then return nil end
 if g.status>0 and (g.turn~=g.status or (g.status==1 and counts[1]~=counts[2]+1) or (g.status==2 and counts[1]~=counts[2])) then return nil end
 return g
end
function M.label(g,i)return ({'.','o','x'})[g.b[i]+1] end
function M.message(g)return g.status>0 and 'P'..g.status..' connected - wins' or 'P'..g.turn..': '..(g.turn==1 and 'top to bottom' or 'left to right') end
M.help={'11x11 Hex; P1 connects top to bottom, P2 left to right.','Neighbors: left/right, up/down, upper-right/lower-left.','No swap rule. No draws. An occupied cell cannot change.','Solo evaluates shortest connection paths plus immediate wins/blocks.','This smaller opponent is NOT the original RAVE Monte Carlo engine.','Two players share this device; Nearby is unavailable.'}
return M
