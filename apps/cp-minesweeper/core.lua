-- CrossPlay rules translated to Lua; see LICENSE-CrossPlay.txt.
local M={title='Minesweeper',cols=8,rows=10,solo=true}
function M.new(seed)
 local g={b={},rng=seed&0xffffffff,status=0}
 for i=1,80 do g.b[i]=0 end
 return g
end
local function next_random(g)
 local s=g.rng;if s==0 then s=0x9e3779b9 end
 s=(s~(s<<13))&0xffffffff;s=s~(s>>17);s=(s~(s<<5))&0xffffffff
 g.rng=s;return s
end
function M.neighbors(i)
 local a={};local x=(i-1)%8;local y=(i-1)//8
 for dy=-1,1 do for dx=-1,1 do
  if (dx~=0 or dy~=0) and x+dx>=0 and x+dx<8 and y+dy>=0 and y+dy<10 then
   a[#a+1]=(y+dy)*8+x+dx+1
  end
 end end
 return a
end
function M.adjacent(g,i,mask)
 local n=0;local x=(i-1)%8;local y=(i-1)//8
 for dy=-1,1 do for dx=-1,1 do
  if (dx~=0 or dy~=0) and x+dx>=0 and x+dx<8 and y+dy>=0 and y+dy<10 then
   if g.b[(y+dy)*8+x+dx+1]&mask~=0 then n=n+1 end
  end
 end end
 return n
end
local function reveal(g,i)
 if g.b[i]&6~=0 or g.status>=2 then return false end
 g.b[i]=g.b[i]|2
 if g.b[i]&1~=0 then g.status=3;return true end
 -- Fixed board bounds cap this queue at 80 indices, marked when enqueued.
 local q={i};local head=1
 while head<=#q do
  local j=q[head];head=head+1
  if M.adjacent(g,j,1)==0 then
   local x=(j-1)%8;local y=(j-1)//8
   for dy=-1,1 do for dx=-1,1 do
    if x+dx>=0 and x+dx<8 and y+dy>=0 and y+dy<10 then
     local k=(y+dy)*8+x+dx+1
     if g.b[k]&7==0 then g.b[k]=g.b[k]|2;q[#q+1]=k end
    end
   end end
  end
 end
 for j=1,80 do if g.b[j]&3==0 then return true end end
 g.status=2;return true
end
function M.action(g,x,y,flag)
 if x<1 or x>8 or y<1 or y>10 or g.status>=2 then return false end
 local i=(y-1)*8+x
 if flag then
  if g.b[i]&2~=0 then return false end
  g.b[i]=g.b[i]~4;return true
 end
 if g.b[i]&4~=0 then return false end
 if g.status==0 then
  local count=0
  while count<10 do
   local j=((next_random(g)&0x7fffffff)%80+(g.rng<0 and 48 or 0))%80+1;local xx=(j-1)%8+1;local yy=(j-1)//8+1
   if g.b[j]&1==0 and (math.abs(xx-x)>1 or math.abs(yy-y)>1) then
    g.b[j]=g.b[j]|1;count=count+1
   end
  end
  g.status=1
 end
 if g.b[i]&2~=0 then
  if M.adjacent(g,i,4)~=M.adjacent(g,i,1) then return false end
  local changed=false
  for _,j in ipairs(M.neighbors(i)) do if reveal(g,j) then changed=true end end
  return changed
 end
 return reveal(g,i)
end
function M.encode(g)
 local s={tostring(g.rng),'|',tostring(g.status),'|'}
 for i=1,80 do s[#s+1]=tostring(g.b[i]) end
 return table.concat(s)
end
function M.decode(s)
 if type(s)~='string' or #s>100 then return nil end
 local rng,status,board=s:match('^(-?%d+)|([0-3])|([0-7]+)$')
 rng=tonumber(rng);status=tonumber(status)
 if not board or #board~=80 or not rng or math.type(rng)~='integer' then return nil end
 local g=M.new(rng);g.status=status;local mines,hidden,hit=0,0,0
 for i=1,80 do
  local v=tonumber(board:sub(i,i));g.b[i]=v
  if v&6==6 then return nil end
  if v&1~=0 then mines=mines+1;if v&2~=0 then hit=hit+1 end
  elseif v&2==0 then hidden=hidden+1 end
 end
 if status==0 then if mines~=0 or hidden~=80 then return nil end
 elseif mines~=10 then return nil
 elseif status==1 and (hit~=0 or hidden==0) then return nil
 elseif status==2 and (hidden~=0 or hit~=0) then return nil
 elseif status==3 and hit==0 then return nil end
 return g
end
function M.label(g,i)
 local v=g.b[i]
 if g.status==3 and v&1~=0 then return '*' end
 if v&4~=0 then return 'F' end
 if v&2==0 then return '?' end
 local n=M.adjacent(g,i,1);return n==0 and '' or tostring(n)
end
function M.message(g)
 if g.status==2 then return 'Cleared!' elseif g.status==3 then return 'Mine hit' end
 local n=10;for i=1,80 do if g.b[i]&4~=0 then n=n-1 end end
 return 'Mines minus flags: '..n
end
M.help={'Open every safe cell. Ten mines on an 8 x 10 board.',
 'The first dig and its neighbors are safe.', 'Flag mode marks suspected mines; Dig opens cells.',
 'Dig an open number to chord when its flags match.', 'Wrong flags can make a chord hit a mine.',
 'Stock Lua sends taps only: use the Flag/Dig button.', 'Back opens the menu. Exit saves your board.'}
return M
