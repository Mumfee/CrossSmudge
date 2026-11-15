local U=dofile('util.lua')
local M={title='Jaipur - local'}
local names={'Diamond','Gold','Silver','Cloth','Spice','Leather','Camel','Empty'}
local supply={6,6,6,8,8,10}
local tokens={{7,7,5,5,5},{6,6,5,5,5},{5,5,5,5,5},{5,3,3,2,2,1,1},{5,3,3,2,2,1,1},{4,3,2,1,1,1,1,1,1}}
local bonuses={{1,1,2,2,2,3,3},{4,4,5,5,6,6},{8,8,9,10,10}}
function M.deck(seed)local t={};for k,n in ipairs(supply) do for _=1,n do t[#t+1]=k end end;for _=1,8 do t[#t+1]=7 end;return U.shuffle(t,seed,0) end
function M.deal(g,seed,starter)
 g.seed=seed;g.turn=starter;g.starter=starter;g.phase=0;g.hands={U.zeros(6),U.zeros(6)};g.herd={0,0};g.sold=U.zeros(6);g.depth=U.zeros(6);g.bdepth=U.zeros(3)
 g.points={0,0};g.bpoints={0,0};g.nt={0,0};g.nb={0,0};g.ui=0;g.mask=0;g.give=U.zeros(7)
 local d=M.deck(seed);for s=1,2 do for i=(s-1)*5+1,s*5 do local c=d[i];if c==7 then g.herd[s]=g.herd[s]+1 else g.hands[s][c]=g.hands[s][c]+1 end end end
 g.market={7,7,7,d[11],d[12]};g.at=13
end
function M.new(seed)local g={seals={0,0},round=1};M.deal(g,seed,1);return g end
function M.score(g,s)return g.points[s]+g.bpoints[s]+(g.herd[s]>g.herd[3-s] and 5 or 0) end
function M.winner(g)
 for _,t in ipairs({{M.score(g,1),M.score(g,2)},g.nb,g.nt,g.herd}) do if t[1]~=t[2] then return t[1]>t[2] and 1 or 2 end end;return 0
end
function M.legal(g,m)
 if g.phase~=0 then return false end
 local h=g.hands[g.turn]
 if m[1]==1 then return m[2]>=1 and m[2]<=5 and g.market[m[2]]<7 and U.sum(h)<7
 elseif m[1]==2 then for _,v in ipairs(g.market) do if v==7 then return true end end;return false
 elseif m[1]==4 then return m[2]>=1 and m[2]<=6 and m[3]>=(m[2]<=3 and 2 or 1) and m[3]<=h[m[2]]
 elseif m[1]==3 then
  local taken=U.zeros(6);local n=0
  for i=1,5 do if (m[2]&(1<<(i-1)))~=0 then local c=g.market[i];if c>=7 then return false end;taken[c]=taken[c]+1;n=n+1 end end
  if n<2 or not U.array(m[3],7,0,7) or U.sum(m[3])~=n or m[3][7]>g.herd[g.turn] or U.sum(h)+m[3][7]>7 then return false end
  for k=1,6 do if m[3][k]>h[k] or (m[3][k]>0 and taken[k]>0) then return false end end;return true
 end;return false
end
function M.apply(g,m)
 if not M.legal(g,m) then return false end
 local s=g.turn;local h=g.hands[s];local failed=false
 local function refill(i)local d=M.deck(g.seed);g.market[i]=d[g.at] or 8;if g.at<=52 then g.at=g.at+1 else failed=true end end
 if m[1]==1 then local c=g.market[m[2]];h[c]=h[c]+1;refill(m[2])
 elseif m[1]==2 then for i=1,5 do if g.market[i]==7 then g.herd[s]=g.herd[s]+1;refill(i) end end
 elseif m[1]==3 then
  local slots={};for i=1,5 do if (m[2]&(1<<(i-1)))~=0 then h[g.market[i]]=h[g.market[i]]+1;slots[#slots+1]=i end end
  local at=1;for k=1,7 do for _=1,m[3][k] do g.market[slots[at]]=k;at=at+1 end;if k<7 then h[k]=h[k]-m[3][k] else g.herd[s]=g.herd[s]-m[3][k] end end
 else
  local k,n=m[2],m[3];h[k]=h[k]-n;g.sold[k]=g.sold[k]+n
  for _=1,n do if g.depth[k]<#tokens[k] then g.depth[k]=g.depth[k]+1;g.points[s]=g.points[s]+tokens[k][g.depth[k]];g.nt[s]=g.nt[s]+1 end end
  if n>=3 then local b=math.min(n,5)-2;if g.bdepth[b]<#bonuses[b] then local t=U.shuffle(U.copy(bonuses[b]),g.seed,b);g.bdepth[b]=g.bdepth[b]+1;g.bpoints[s]=g.bpoints[s]+t[g.bdepth[b]];g.nb[s]=g.nb[s]+1 end end
 end
 local empty=0;for k=1,6 do if g.depth[k]==#tokens[k] then empty=empty+1 end end
 if failed or empty>=3 then local w=M.winner(g);if w>0 then g.seals[w]=g.seals[w]+1 end;g.phase=(g.seals[1]>=2 or g.seals[2]>=2) and 2 or 1 else g.turn=3-s end
 g.ui=0;return true
end
function M.moves(g)
 local a={};for i=1,5 do local m={1,i};if M.legal(g,m) then a[#a+1]=m end end
 if M.legal(g,{2}) then a[#a+1]={2} end
 for k=1,6 do for n=1,g.hands[g.turn][k] do local m={4,k,n};if M.legal(g,m) then a[#a+1]=m end end end;return a
end
function M.ai(g)
 local a=M.moves(g);local best,value=nil,-999
 for _,m in ipairs(a) do
  local v=0
  if m[1]==4 then for i=1,m[3] do v=v+(tokens[m[2]][g.depth[m[2]]+i] or 0) end;v=v+(m[3]>=3 and (m[3]>=5 and 9 or m[3]==4 and 5 or 2) or 0)
  elseif m[1]==1 then local k=g.market[m[2]];v=3+g.hands[g.turn][k]*2+(7-k)/4
  else v=2;for _,c in ipairs(g.market) do if c==7 then v=v+1 end end end
  if v>value then best,value=m,v end
 end;return best
end
function M.valid(g)
 if g.turn<1 or g.turn>2 or g.phase<0 or g.phase>2 or g.at<13 or g.at>53 or g.round<1 then return false end
 if not U.array(g.market,5,1,8) or not U.array(g.herd,2,0,11) or not U.array(g.depth,6,0,9) or not U.array(g.sold,6,0,10) then return false end
 for _,key in ipairs({'points','bpoints','nt','nb','seals'}) do if not U.array(g[key],2,0,200) then return false end end
 if not U.array(g.bdepth,3,0,7) or not U.array(g.give,7,0,7) then return false end
 local d=M.deck(g.seed);local count=U.zeros(7)
 for s=1,2 do if not U.array(g.hands[s],6,0,7) or U.sum(g.hands[s])>7 then return false end;for k=1,6 do count[k]=count[k]+g.hands[s][k] end;count[7]=count[7]+g.herd[s] end
 for _,c in ipairs(g.market) do if c<8 then count[c]=count[c]+1 end end
 for i=g.at,52 do count[d[i]]=count[d[i]]+1 end
 for k=1,6 do if count[k]+g.sold[k]~=supply[k] or g.depth[k]~=math.min(g.sold[k],#tokens[k]) then return false end end
 local total,nt,bt,nb=0,0,0,0
 for k=1,6 do for i=1,g.depth[k] do total=total+tokens[k][i] end;nt=nt+g.depth[k] end
 for k=1,3 do if g.bdepth[k]>#bonuses[k] then return false end;local b=U.shuffle(U.copy(bonuses[k]),g.seed,k);for i=1,g.bdepth[k] do bt=bt+b[i] end;nb=nb+g.bdepth[k] end
 if U.sum(g.points)~=total or U.sum(g.nt)~=nt or U.sum(g.bpoints)~=bt or U.sum(g.nb)~=nb then return false end
 if g.seals[1]>2 or g.seals[2]>2 or ((g.seals[1]==2 or g.seals[2]==2)~=(g.phase==2)) then return false end
 return count[7]==11 and g.starter>=1 and g.starter<=2 and g.ui>=0 and g.ui<=9 and g.mask>=0 and g.mask<=31
end
U.codec(M,{'seed','turn','starter','phase','hands','herd','sold','depth','bdepth','points','bpoints','nt','nb','seals','round','market','at','ui','mask','give'})
function M.pause(g)g.ui=0 end
local function ui()dofile('ui.lua')(M,U,names,tokens) end
function M.view(g)ui();return M.view(g) end
function M.act(g,i)ui();return M.act(g,i) end

return M
