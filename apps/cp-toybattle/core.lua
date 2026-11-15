local U=dofile('util.lua');local M={title='Toy Battle - local'}
local names={'Kwak','Skully',"Cap'n",'Jumbo','Hook','XB42','Star','Roxy'}
local cache,terrain
function M.board(g)
 if cache~=g.map then terrain=dofile('maps/'..g.map..'.lua');cache=g.map;terrain.adj={};for i=1,terrain.n+#terrain.hq do terrain.adj[i]=0 end;for _,e in ipairs(terrain.edges) do terrain.adj[e[1]]=terrain.adj[e[1]]|(1<<(e[2]-1));terrain.adj[e[2]]=terrain.adj[e[2]]|(1<<(e[1]-1)) end;terrain.edges=nil end;return terrain
end
local function mix(x)x=x+0x9e3779b9;x=(x~(x>>16))*0x85ebca6b;x=(x~(x>>13))*0xc2b2ae35;return x~(x>>16) end
function M.deck(seed,s)
 local d={};for i=1,24 do d[i]=(i-1)//3+1 end
 for i=24,2,-1 do local x=mix(seed~((s-1)*0x9e3779b9)~((i-1)*0x85ebca6b));local j=U.mod(x,i)+1;d[i],d[j]=d[j],d[i] end;return d
end
local function draw(g,s,n)
 local d=M.deck(g.seed,s);n=math.min(n,8-U.sum(g.rack[s]),20-g.taken[s])
 for _=1,n do g.taken[s]=g.taken[s]+1;local k=d[4+g.taken[s]];g.rack[s][k]=g.rack[s][k]+1 end
end
function M.new(seed,map)
 local g={seed=seed,map=map or 0,turn=1,phase=0,winner=0,ending=0,taken={0,0},rack={U.zeros(8),U.zeros(8)},discard={U.zeros(8),U.zeros(8)},medals={0,0},frozen={0,0},tick=0,slots={},tiles={},regions=0,special=1,ui=0,steps={},edit={1,1,0,0,0,0,0,1},notice=0}
 draw(g,1,3);draw(g,2,4);return g
end
function M.top(g,slot)for i=#g.slots,1,-1 do if g.slots[i]==slot then return g.tiles[i]//8+1,g.tiles[i]%8+1,i end end;return 0,0,0 end
function M.reach(g,s)
 local t=M.board(g);local seen={};for i,owner in ipairs(t.hq) do if owner==s then for q=1,t.n+#t.hq do if (t.adj[t.n+i]&(1<<(q-1)))~=0 then seen[q]=true end end end end
 local changed=true;while changed do changed=false;for p=1,t.n do if seen[p] and M.top(g,p)==s then for q=1,t.n+#t.hq do if (t.adj[p]&(1<<(q-1)))~=0 and not seen[q] then seen[q]=true;changed=true end end end end end;return seen
end
function M.canplace(g,s,p,k,effect,reach)
 local t=M.board(g);if p<1 or p>t.n+#t.hq or k<1 or k>8 or g.rack[s][k]==0 or g.frozen[s]==k then return false end
 if g.special==1 and t.gate[p]~=0 and (t.gate[p]&(1<<(k-1)))==0 then return false end
 reach=reach or M.reach(g,s)
 if p>t.n then return t.hq[p-t.n]~=s and not not reach[p] end
 local holder,def=M.top(g,p);if holder~=0 and holder~=s and not (k==1 or def==1 or k>def) then return false end
 return not not (reach[p] or (effect and k==5 and not (g.special==1 and t.special[p]==7)))
end
local function win(g,s,why)g.phase=1;g.winner=s;g.ending=why end
local function settle(g)
 local t=M.board(g);for s=1,2 do local mask=0;for p=1,t.n do if M.top(g,p)==s then mask=mask|(1<<(p-1)) end end
 for i,r in ipairs(t.regions) do local bit=1<<(i-1);if (g.regions&bit)==0 and (mask&r[1])==r[1] then g.regions=g.regions|bit;g.medals[s]=g.medals[s]+r[2];if g.medals[s]>=t.goal and g.phase==0 then win(g,s,2) end end end end
end
local function pop(g,p)local s,k,i=M.top(g,p);if s==0 then return end;table.remove(g.slots,i);table.remove(g.tiles,i);return s,k end
local function push(g,p,s,k)g.slots[#g.slots+1]=p;g.tiles[#g.tiles+1]=(s-1)*8+k-1 end
local function adjacent(t,p,q)return t.adj[p] and q>0 and (t.adj[p]&(1<<(q-1)))~=0 end
local function blind(g,s,salt)
 local n=U.sum(g.rack[s]);if n==0 then return 0 end
 local pick=U.mod(mix(g.seed~salt~(g.tick<<16)),n);g.tick=(g.tick+1)%256
 for k=1,8 do if pick<g.rack[s][k] then return k end;pick=pick-g.rack[s][k] end;return 0
end
function M.moves(g,visit)
 local a={};local function add(m)if visit then visit(m) else a[#a+1]=m end end;if g.phase~=0 then return a end
 local s=g.turn;if g.taken[s]<20 and U.sum(g.rack[s])<8 then add({}) end
 local t=M.board(g);local reach=M.reach(g,s)
 for k=1,8 do if g.rack[s][k]>0 then for p=1,t.n+#t.hq do
  if M.canplace(g,s,p,k,true,reach) then local e=M.canplace(g,s,p,k,false,reach) and 0 or 1;add({{p,k,e,0,0,0,0,1}}) end
 end end end;return a
end
M._draw=draw;M._pop=pop;M._push=push;M._settle=settle;M._win=win;M._blind=blind;M._adjacent=adjacent
function M.apply(g,steps)return dofile('rules.lua')(M,U,g,steps) end
function M.ai(g)return dofile('ai.lua')(M,U,g) end
function M.valid(g)return dofile('validate.lua')(M,U,g) end
U.codec(M,{'seed','map','turn','phase','winner','ending','taken','rack','discard','medals','frozen','tick','slots','tiles','regions','special','ui','steps','edit','notice'})
function M.pause(g)g.ui=0 end
function M.view(g)return dofile('view.lua')(M,U,names,g) end
function M.act(g,i)return dofile('actions.lua')(M,U,names,g,i) end
return M
