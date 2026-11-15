local U=dofile('util.lua');local cards=dofile('cards.lua');local kinds,colours=cards[1],cards[2]
local M={title='Sea Salt - local'}
local names={'Crab','Boat','Fish','Swimmer','Shark','Shell','Octopus','Penguin','Sailor','Mermaid','Lighthouse','Shoal','Colony','Captain'}
local cn={'dark blue','light blue','black','yellow','green','white','purple','grey','light orange','pink','orange'}
function M.name(c)return names[kinds[c]+1]..' '..cn[colours[c]+1]..' #'..c end
function M.deck(seed)local t={};for i=1,58 do t[i]=i end;return U.shuffle(t,seed,0) end
function M.top(g,p)local best,seq=0,-1;for c=1,58 do if g.place[c]==p+4 and g.seq[c]>seq then best,seq=c,g.seq[c] end end;return best end
function M.count(g,s,k,hand)local n=0;for c=1,58 do if kinds[c]==k and (g.place[c]==s or (not hand and g.place[c]==s+2)) then n=n+1 end end;return n end
function M.points(g,s)
 local n=U.zeros(14);local co=U.zeros(11)
 for c=1,58 do if g.place[c]==s or g.place[c]==s+2 then n[kinds[c]+1]=n[kinds[c]+1]+1;co[colours[c]+1]=co[colours[c]+1]+1 end end
 local p=n[1]//2+n[2]//2+n[3]//2+math.min(n[4],n[5])
 p=p+({0,0,2,4,6,8,10})[n[6]+1]+({0,0,3,6,9,12})[n[7]+1]+({0,1,3,5})[n[8]+1]+({0,0,5})[n[9]+1]
 if n[11]>0 then p=p+n[2] end;if n[12]>0 then p=p+n[3] end;if n[13]>0 then p=p+2*n[8] end;if n[14]>0 then p=p+3*n[9] end
 table.sort(co,function(a,b)return a>b end);for i=1,n[10] do p=p+co[i] end
 return p,co[1]
end
local function hand(g,c)g.place[c]=g.turn;g.seq[c]=0;if M.count(g,g.turn,9,true)==4 then g.phase=3 end end
function M.deal(g,seed,starter)
 g.seed=seed;g.turn=starter;g.starter=starter;g.phase=0;g.step=0;g.place=U.zeros(58);g.seq=U.zeros(58);g.seqnext=3;g.at=3;g.drawn={0,0};g.pending=0;g.crab=0;g.ender=0;g.bet=0;g.extra=0;g.revealed=0;g.ui=0;g.first=0
 local d=M.deck(seed);for p=1,2 do g.place[d[p]]=p+4;g.seq[d[p]]=p end
end
function M.new(seed)local g={score={0,0},round=1};M.deal(g,seed,1);return g end
function M.bank(g)
 if g.ender~=0 then
  local a,b=M.points(g,1),M.points(g,2);local won=(g.ender==1 and a>=b) or (g.ender==2 and b>=a)
  for s=1,2 do local p,c=M.points(g,s);if g.bet==1 then p=s==g.ender and (won and p+c or c) or (won and c or p) end;g.score[s]=g.score[s]+p end
 end
 g.phase=(g.score[1]>=40 or g.score[2]>=40) and 3 or 2
end
function M.moves(g,visit,noduos)
 local a={};local emitted=0;local function add(k,x,y)emitted=emitted+1;local m={k,x,y};if visit then visit(m) else a[#a+1]=m end end
 if g.phase>=2 then return a end
 if g.step==0 then if g.at<=58 then add(1) end;for p=1,2 do if M.top(g,p)>0 then add(2,p) end end;if emitted==0 then add(8) end
 elseif g.step==1 then add(3,1);add(3,2)
 elseif g.step==2 then add(4,1);add(4,2)
 elseif g.step==4 then for p=1,2 do if M.top(g,p)>0 then add(6,p) end end
 elseif g.step==5 then for c=1,58 do if g.place[c]==g.crab+4 then add(7,c) end end
 else
  if not noduos then for a1=1,57 do if g.place[a1]==g.turn then for b=a1+1,58 do if g.place[b]==g.turn then
   local x,y=kinds[a1],kinds[b];if (x==y and x<=2) or (x==3 and y==4) or (x==4 and y==3) then add(5,a1,b) end
  end end end end end
  add(8);if g.phase==0 and M.points(g,g.turn)>=7 then add(9,0);add(9,1) end
 end;return a
end
function M.apply(g,m)
 local found=false;M.moves(g,function(v)if v[1]==m[1] and v[2]==m[2] and v[3]==m[3] then found=true end end);if not found then return false end
 local k,x,y=m[1],m[2],m[3]
 local function draw()local d=M.deck(g.seed);local c=d[g.at];g.at=g.at+1;return c end
 local function discard(p)g.place[g.pending]=p+4;g.seq[g.pending]=g.seqnext;g.seqnext=g.seqnext+1;g.pending=0;g.step=3 end
 if k==1 then local c=draw();if g.at<=58 then local b=draw();g.place[c]=7;g.place[b]=7;g.drawn={c,b};g.step=1 else hand(g,c);g.step=3 end
 elseif k==2 then hand(g,M.top(g,x));g.step=3
 elseif k==3 then hand(g,g.drawn[x]);g.pending=g.drawn[3-x];g.drawn={0,0};if g.phase~=3 then if M.top(g,1)==0 then discard(1) elseif M.top(g,2)==0 then discard(2) else g.step=2 end end
 elseif k==4 then discard(x)
 elseif k==5 then
  g.place[x]=g.turn+2;g.place[y]=g.turn+2;local t=kinds[x]
  if t==0 then if M.top(g,1)>0 or M.top(g,2)>0 then g.step=4 end
  elseif t==1 then g.extra=g.extra+1
  elseif t==2 then if g.at<=58 then hand(g,draw()) end
  else
   local foe=3-g.turn;if (g.revealed&(1<<(foe-1)))==0 then
    local held={};for c=1,58 do if g.place[c]==foe then held[#held+1]=c end end
    if #held>0 then local z=g.seed~(0x9e3779b9*(65+g.seqnext));if z==0 then z=0x1234567 end;z=z~(0x85ebca6b*g.at)~(0xc2b2ae35*g.turn);z=U.next(z);if z==0 then z=0x9e3779b9 end;hand(g,held[U.mod(U.next(z),#held)+1]) end
   end
  end
 elseif k==6 then g.crab=x;g.step=5
 elseif k==7 then hand(g,x);g.step=3
 elseif k==8 then
  if g.extra>0 then g.extra=g.extra-1;g.step=0
  elseif g.phase==1 then g.revealed=3;M.bank(g)
  elseif g.at==59 then g.phase=2
  else g.turn=3-g.turn;g.step=0 end
 elseif k==9 then
  g.ender=g.turn;g.bet=x;g.revealed=g.revealed|(1<<(g.turn-1))
  if x==0 then M.bank(g) else g.phase=1;g.turn=3-g.turn;g.step=0 end
 end;return true
end
-- This heuristic evaluates only visible choices and its own cards, never the deck or hidden hand.


function M.ai(g)return dofile('ai.lua')(M,U,kinds,g) end
function M.valid(g)return dofile('validate.lua')(M,U,kinds,g) end
U.codec(M,{'seed','turn','starter','phase','step','place','seq','seqnext','at','drawn','pending','crab','ender','bet','extra','revealed','score','round','ui','first'})
function M.pause(g)g.ui=0 end
function M.view(g)return dofile('view.lua')(M,U,names,kinds,g) end
function M.act(g,i)return dofile('actions.lua')(M,U,names,kinds,g,i) end
return M
