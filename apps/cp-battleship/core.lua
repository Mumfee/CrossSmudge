local U=dofile('support.lua');local M={title='Battleship',cols=10,rows=10}
local lengths={5,4,3,3,2}
local function at(s,i)return tonumber(s:sub(i,i)) end
local function place(board,ship,bow,horizontal)
 local x,y=(bow-1)%10,(bow-1)//10;local n=lengths[ship]
 if bow<1 or bow>100 or (horizontal and x+n>10) or (not horizontal and y+n>10) then return nil end
 for k=0,n-1 do local i=bow+k*(horizontal and 1 or 10);if at(board,i)~=0 then return nil end end
 for k=0,n-1 do board=U.mark(board,bow+k*(horizontal and 1 or 10),tostring(ship)) end
 return board
end
function M.new(seed)
 local g={rng=seed,turn=1,status=0,phase=0,ship=1,rotate=0,show=0,a=string.rep('0',100),b=string.rep('0',100),sa=string.rep('0',100),sb=string.rep('0',100),last=0}
 for ship=1,5 do
  local start=U.random(g,200)
  for k=0,199 do local n=(start+k-1)%200;local b=place(g.b,ship,n%100+1,n<100)
   if b then g.b=b;break end
  end
 end
 return g
end
function M.hits(board,shots)
 local n=0;for i=1,100 do if at(board,i)>0 and at(shots,i)==1 then n=n+1 end end;return n
end
function M.fire(g,i)
 if g.phase~=1 or g.status~=0 or i<1 or i>100 then return false end
 local key=g.turn==1 and 'sb' or 'sa';if at(g[key],i)==1 then return false end
 g[key]=U.mark(g[key],i,'1');g.last=i
 local target=g.turn==1 and g.b or g.a
 if M.hits(target,g[key])==17 then g.status=g.turn else g.turn=3-g.turn end
 return true
end
function M.action(g,x,y)
 if x<1 or x>10 or y<1 or y>10 or g.status~=0 then return false end
 local i=(y-1)*10+x
 if g.phase==0 then
  local board=place(g.a,g.ship,i,g.rotate==0);if not board then return false end
  g.a=board;g.ship=g.ship+1;if g.ship==6 then g.phase=1;g.show=0 end;return true
 end
 if g.show==1 and g.turn==1 then return false end
 return M.fire(g,i)
end
function M.choose(g)
 -- Build PUBLIC shot observations only. The decision receives no unshot cells.
 local observations={};local sunk={}
 for ship=1,5 do local hit=0;for i=1,100 do if at(g.sa,i)==1 and at(g.a,i)==ship then hit=hit+1 end end;sunk[ship]=hit==lengths[ship] end
 for i=1,100 do observations[i]=at(g.sa,i)==0 and -1 or (at(g.a,i)>0 and not sunk[at(g.a,i)] and 1 or 0) end
 local best,score=nil,-1
 for i=1,100 do if observations[i]==-1 then
  local x,y=(i-1)%10,(i-1)//10;local v=(x+y)%2==0 and 1 or 0
  for _,d in ipairs({-10,10,-1,1}) do local j=i+d
   if j>=1 and j<=100 and (math.abs(d)==10 or (j-1)//10==y) and observations[j]==1 then
    v=v+10;local k=j+d
    if k>=1 and k<=100 and (math.abs(d)==10 or (k-1)//10==y) and observations[k]==1 then v=v+20 end
   end
  end
  if v>score then score=v;best=i end
 end end
 return best
end
function M.valid(g)
 if #g.a~=100 or #g.b~=100 or #g.sa~=100 or #g.sb~=100 then return false end
 for side=1,2 do local b=side==1 and g.a or g.b
  for ship=1,5 do
   local cells={};for i=1,100 do if at(b,i)==ship then cells[#cells+1]=i end end
   local expected=side==1 and ship>=g.ship and 0 or lengths[ship]
   if #cells~=expected then return false end
   if #cells>0 then local step=cells[2]-cells[1];if step~=1 and step~=10 then return false end
    for i=2,#cells do if cells[i]~=cells[1]+(i-1)*step or (step==1 and (cells[i]-1)//10~=(cells[1]-1)//10) then return false end end
   end
  end
 end
 if (g.phase==1)~=(g.ship==6) then return false end
 local _,a=g.sa:gsub('1','');local _,b=g.sb:gsub('1','')
 if g.phase==0 and (a~=0 or b~=0 or g.turn~=1) then return false end
 if b<a or b>a+1 then return false end
 if g.phase==1 then
  local expected=g.status==0 and (a==b and 1 or 2) or g.status
  if g.turn~=expected or (g.status==1 and b~=a+1) or (g.status==2 and a~=b) then return false end
 end
 local ha,hb=M.hits(g.a,g.sa),M.hits(g.b,g.sb)
 if ha==17 and hb==17 then return false end
 return g.status==(hb==17 and 1 or ha==17 and 2 or 0)
end
U.codec(M,{{'rng',-2147483648,2147483647},{'turn',1,2},{'status',0,2},{'phase',0,1},{'ship',1,6},{'rotate',0,1},{'show',0,1},{'a','^[0-5]+$',100},{'b','^[0-5]+$',100},{'sa','^[01]+$',100},{'sb','^[01]+$',100},{'last',0,100}})
function M.label(g,i)
 local own=g.phase==0 or g.show==1;local b=own and g.a or g.b;local shots=own and g.sa or g.sb
 if at(shots,i)==1 then return at(b,i)>0 and 'X' or '-' end
 return own and at(b,i)>0 and tostring(at(b,i)) or '.'
end
function M.message(g)
 if g.status>0 then return g.status==1 and 'You sank the fleet!' or 'Opponent wins' end
 if g.phase==0 then return 'Place ship '..g.ship..' length '..lengths[g.ship]..(g.rotate==0 and ' right' or ' down') end
 return (g.show==1 and 'YOUR fleet' or 'TARGET grid')..' - hits '..M.hits(g.b,g.sb)..'/17'
end
M.help={'Place five ships: lengths 5,4,3,3,2. Choose the bow cell.','Use Menu to rotate before placing. Ships may touch, never overlap.','After placement, select target cells to fire. Hits do not grant extra turns.','X = hit, - = miss. Own fleet is visible from Menu; switch back to fire.','Computer hunts on alternating cells and follows unsunk hits.','Local solo game; Nearby is unavailable.'}
return M
