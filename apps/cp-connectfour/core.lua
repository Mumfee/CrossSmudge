-- CrossPlay 7-ply alpha-beta, rules and weights; see LICENSE-CrossPlay.txt.
local M={title='Connect Four',cols=7,rows=6}
local order={4,3,5,2,6,1,7};local weights={3,4,5,7,5,4,3}
local dx={1,0,1,1};local dy={0,1,1,-1}
function M.new()
 local g={b={},turn=1,status=0,history=''}
 for i=1,42 do g.b[i]=0 end;return g
end
local function landing(g,c)
 for r=1,6 do local i=(r-1)*7+c;if g.b[i]==0 then return i end end
end
local function won(g,i,side)
 local x=(i-1)%7+1;local y=(i-1)//7+1
 for d=1,4 do
  local n=1
  for sign=-1,1,2 do
   local xx=x+dx[d]*sign;local yy=y+dy[d]*sign
   while xx>=1 and xx<=7 and yy>=1 and yy<=6 and g.b[(yy-1)*7+xx]==side do
    n=n+1;xx=xx+dx[d]*sign;yy=yy+dy[d]*sign
   end
  end
  if n>=4 then return true end
 end;return false
end
function M.action(g,c)
 if g.status~=0 or c<1 or c>7 then return false end
 local i=landing(g,c);if not i then return false end
 g.b[i]=g.turn;g.history=g.history..c
 if won(g,i,g.turn) then g.status=g.turn
 elseif #g.history==42 then g.status=3
 else g.turn=3-g.turn end
 return true
end
local function evaluate(g,side)
 local score=0;local other=3-side
 for y=1,6 do for x=1,7 do
  local v=g.b[(y-1)*7+x]
  if v==side then score=score+weights[x] elseif v==other then score=score-weights[x] end
  for d=1,4 do
   local ex=x+dx[d]*3;local ey=y+dy[d]*3
   if ex>=1 and ex<=7 and ey>=1 and ey<=6 then
    local mine,theirs=0,0
    for k=0,3 do
     v=g.b[(y+dy[d]*k-1)*7+x+dx[d]*k]
     if v==side then mine=mine+1 elseif v==other then theirs=theirs+1 end
    end
    if theirs==0 then if mine==3 then score=score+60 elseif mine==2 then score=score+6 end
    elseif mine==0 then if theirs==3 then score=score-60 elseif theirs==2 then score=score-6 end end
   end
  end
 end end
 return score
end
-- Explicit reusable search frames keep computation resumable between UI ticks.
-- Seven frames plus a board copy: bounded independently of explored node count.
function M.begin(g)
 if g.status~=0 then return nil end
 local s={g=M.new(),stack={},level=1,best=nil,nodes=0}
 for i=1,42 do s.g.b[i]=g.b[i] end
 s.g.turn=g.turn;s.moves=#g.history
 for i=1,8 do s.stack[i]={} end
 local f=s.stack[1];f.side=g.turn;f.depth=7;f.alpha=-2000000;f.beta=2000000;f.best=-2000000;f.next=1
 return s
end
local function accept(s,score,c)
 local f=s.stack[s.level]
 if s.level==1 then
  if not s.best or score>f.best or (score==f.best and (weights[c]>weights[s.best] or
      (weights[c]==weights[s.best] and c<s.best))) then s.best=c;f.best=score end
 else
  if score>f.best then f.best=score end
  if score>f.alpha then f.alpha=score end
  if f.alpha>=f.beta then f.next=8 end
 end
end
function M.step(s,budget)
 for tick=1,budget do
  local f=s.stack[s.level]
  if f.next>7 then
   if s.level==1 then return true,s.best end
   local value=f.best;local index=f.index;local col=f.col
   s.g.b[index]=0;s.level=s.level-1;accept(s,-value,col)
  else
   local col=order[f.next];f.next=f.next+1
   local i=landing(s.g,col)
   if i then
    s.nodes=s.nodes+1;s.g.b[i]=f.side
    local score
    if won(s.g,i,f.side) then score=1000000+f.depth-1
    elseif s.moves+s.level==42 then score=0
    elseif f.depth==1 then score=-evaluate(s.g,3-f.side) end
    if score then s.g.b[i]=0;accept(s,score,col)
    else
     local child=s.stack[s.level+1]
     child.side=3-f.side;child.depth=f.depth-1
     child.alpha=s.level==1 and -2000000 or -f.beta
     child.beta=s.level==1 and 2000000 or -f.alpha
     child.best=-2000000;child.next=1;child.index=i;child.col=col
     s.level=s.level+1
    end
   end
  end
 end
 return false
end
function M.choose(g)
 local s=M.begin(g);if not s then return nil end
 while true do local done,col=M.step(s,100);if done then return col end end
end
function M.encode(g)return g.history end
function M.decode(s)
 if type(s)~='string' or #s>42 or s:find('[^1-7]') then return nil end
 local g=M.new()
 for i=1,#s do if not M.action(g,tonumber(s:sub(i,i))) then return nil end end
 return g
end
function M.label(g,i)
 local row=(i-1)//7;local col=(i-1)%7+1;local v=g.b[(5-row)*7+col]
 return v==0 and '.' or (v==1 and 'O' or 'X')
end
function M.message(g)
 if g.status==3 then return 'Draw' elseif g.status>0 then return 'P'..g.status..' wins' end
 return 'P'..g.turn..' to drop ('..(g.turn==1 and 'O' or 'X')..')'
end
M.help={'Drop discs into one of seven columns.', 'Connect four horizontally, vertically or diagonally.',
 'P1 is O and always opens; P2 is X.', 'Solo uses the original seven-ply search and weights.',
 'Thinking runs in small steps; Back cancels to menu.', 'Two players share this device. Nearby is unavailable.'}
return M
