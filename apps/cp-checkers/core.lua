-- English draughts and deterministic two-ply material search, CrossPlay MIT.
local M={title='Checkers',cols=8,rows=8}
local function owner(p)return p%2==1 and 1 or 2 end
local function king(p)return p>=3 end
local function xy(i)return (i-1)%8,(i-1)//8 end
local function inside(x,y)return x>=0 and x<8 and y>=0 and y<8 end
function M.new()
 local g={b={},turn=1,idle=0,status=0,selected=0}
 for i=1,64 do local x,y=xy(i);g.b[i]=(x+y)%2==1 and (y<3 and 2 or (y>4 and 1 or 0)) or 0 end
 return g
end
function M.moves(g)
 local out={};local b=g.b
 local function jumps(i,from,taken)
  local x,y=xy(i);local p=b[i];local forward=owner(p)==1 and -1 or 1;local any=false
  for d=1,king(p) and 2 or 1 do local dy=d==1 and forward or -forward
   for dx=-1,1,2 do
    local xx,yy=x+2*dx,y+2*dy
    if inside(xx,yy) then
     local over=(y+dy)*8+x+dx+1;local land=yy*8+xx+1;local enemy=b[over]
     if enemy~=0 and owner(enemy)~=g.turn and b[land]==0 then
      any=true;b[i]=0;b[over]=0;b[land]=p
      jumps(land,from,taken..string.char(over))
      b[land]=0;b[over]=enemy;b[i]=p
     end
    end
   end
  end
  if not any and #taken>0 then out[#out+1]=string.char(from,i)..taken end
 end
 for i=1,64 do if b[i]~=0 and owner(b[i])==g.turn then jumps(i,i,'') end end
 if #out>0 then return out end
 for i=1,64 do local p=b[i]
  if p~=0 and owner(p)==g.turn then
   local x,y=xy(i);local forward=g.turn==1 and -1 or 1
   for d=1,king(p) and 2 or 1 do local dy=d==1 and forward or -forward
    for dx=-1,1,2 do if inside(x+dx,y+dy) then local to=(y+dy)*8+x+dx+1
     if b[to]==0 then out[#out+1]=string.char(i,to) end
    end end
   end
  end
 end
 return out
end
local function apply(g,m)
 local from,to=m:byte(1,2);local p=g.b[from];g.b[from]=0
 for i=3,#m do g.b[m:byte(i)]=0 end
 local _,y=xy(to);g.b[to]=not king(p) and ((owner(p)==1 and y==0) or (owner(p)==2 and y==7)) and p+2 or p
 g.idle=(#m>2 or not king(p)) and 0 or g.idle+1;g.turn=3-g.turn;g.selected=0
end
function M.outcome(g)
 if g.idle>=80 then return 3 end
 return #M.moves(g)==0 and 3-g.turn or 0
end
function M.play(g,m)
 if g.status~=0 then return false end
 for _,legal in ipairs(M.moves(g)) do if m==legal then apply(g,m);g.status=M.outcome(g);return true end end
 return false
end
local function copy(g)local n={b={},turn=g.turn,idle=g.idle,status=0};for i=1,64 do n.b[i]=g.b[i] end;return n end
local function material(g,seat)
 local s=0;for i=1,64 do local p=g.b[i];if p~=0 then s=s+(owner(p)==seat and 1 or -1)*(king(p) and 18 or 10) end end;return s
end
function M.choose(g)
 if g.status~=0 then return nil end
 local best,value=nil,-3000
 for _,m in ipairs(M.moves(g)) do
  local trial=copy(g);apply(trial,m);local replies=M.moves(trial);local worst=#replies==0 and 1000 or 2000
  for _,r in ipairs(replies) do local nextg=copy(trial);apply(nextg,r);worst=math.min(worst,material(nextg,g.turn)) end
  if worst>value then value=worst;best=m end
 end
 return best
end
function M.action(g,x,y)
 if type(x)=='string' then return M.play(g,x) end
 if g.status~=0 or x<1 or x>8 or y<1 or y>8 then return false end
 local i=(y-1)*8+x
 if g.selected>0 then for _,m in ipairs(M.moves(g)) do if m:byte(1)==g.selected and m:byte(2)==i then return M.play(g,m) end end end
 if g.b[i]~=0 and owner(g.b[i])==g.turn then g.selected=i;return true end
 return false
end
function M.encode(g)
 local s={};for i=1,64 do s[i]=tostring(g.b[i]) end
 return g.turn..','..g.idle..','..table.concat(s)
end
function M.decode(s)
 if type(s)~='string' then return nil end
 local t,idle,b=s:match('^([12]),(%d+),([0-4]+)$');idle=tonumber(idle)
 if not b or #b~=64 or not idle or idle>80 then return nil end
 local g=M.new();g.turn=tonumber(t);g.idle=idle;local counts={0,0}
 for i=1,64 do local p=tonumber(b:sub(i,i));local x,y=xy(i)
  if p~=0 then
   if (x+y)%2~=1 or (p==1 and y==0) or (p==2 and y==7) then return nil end
   counts[owner(p)]=counts[owner(p)]+1
  end
  g.b[i]=p
 end
 if counts[1]>12 or counts[2]>12 or counts[1]+counts[2]==0 then return nil end
 g.status=M.outcome(g);return g
end
function M.label(g,i)return ({'.','o','x','O','X'})[g.b[i]+1] end
function M.message(g)
 if g.status>0 then return g.status==3 and 'Draw: 80 idle plies' or 'P'..g.status..' wins' end
 return 'P'..g.turn..' to move'..(g.selected>0 and ' - select destination' or ' - select piece')
end
M.help={'P1 o/O moves up; P2 x/X moves down. Capitals are kings.','Select your piece, then its destination. Captures are mandatory.','A jump chain is one move: select its FINAL landing square.','Men jump forward; kings both ways. Crowning ends the move.','80 plies without a capture or man move is a draw.','Solo uses CrossPlay two-ply material search; no Nearby.'}
return M
