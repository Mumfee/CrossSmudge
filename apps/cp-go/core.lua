local U=dofile('util.lua');local M={title='Go - local'}
function M.at(b,p)return b:byte(p)-48 end
function M.put(b,p,v)return b:sub(1,p-1)..v..b:sub(p+1) end
function M.neighbours(n,p)
 local a={};local r,c=(p-1)//n,(p-1)%n
 if r>0 then a[#a+1]=p-n end;if r<n-1 then a[#a+1]=p+n end;if c>0 then a[#a+1]=p-1 end;if c<n-1 then a[#a+1]=p+1 end;return a
end
function M.group(b,n,p)
 local stones,seen,libs={p},{[p]=true},{};local at,lc=1,0
 while at<=#stones do for _,q in ipairs(M.neighbours(n,stones[at])) do
  if M.at(b,q)==0 then if not libs[q] then libs[q]=true;lc=lc+1 end
  elseif M.at(b,q)==M.at(b,p) and not seen[q] then seen[q]=true;stones[#stones+1]=q end
 end;at=at+1 end;return stones,lc,libs
end
function M.key(b,s)local h=0x811c9dc5;for i=1,#b do h=(h~M.at(b,i))*16777619 end;return (h~s)*16777619 end
function M.new(seed,n,handicap)
 n=n or 9;handicap=handicap or 0
 local g={seed=seed,n=n,b=string.rep('0',n*n),dead=string.rep('0',n*n),turn=1,phase=0,moves=0,passes=0,ko=0,last=0,history={},captured={0,0},komi=handicap>0 and 1 or 15,handicap=handicap,accepted=0,ui=0,row=1,col=1,solo=0}
 if handicap>=2 then local near=n==9 and 2 or 3;local far=n-1-near;local mid=n//2;local p={far*n+near+1,near*n+far+1,near*n+near+1,far*n+far+1,mid*n+mid+1};for i=1,handicap do g.b=M.put(g.b,p[i],1) end;g.turn=2 end;return g
end
function M.trial(g,p)
 if p<1 or p>g.n*g.n or M.at(g.b,p)~=0 or p==g.ko then return nil end
 local b=M.put(g.b,p,g.turn);local taken=0
 for _,q in ipairs(M.neighbours(g.n,p)) do if M.at(b,q)==3-g.turn then local stones,libs=M.group(b,g.n,q);if libs==0 then for _,x in ipairs(stones) do b=M.put(b,x,0);taken=taken+1 end end end end
 local stones,libs,ls=M.group(b,g.n,p);if libs==0 then return nil end
 local key=M.key(b,3-g.turn);for _,k in ipairs(g.history) do if k==key then return nil end end
 local ko=0;if taken==1 and #stones==1 and libs==1 then for x in pairs(ls) do ko=x end end
 return b,taken,ko,libs
end
function M.play(g,p)
 if g.phase~=0 then return false end
 local b,taken,ko
 if p~=0 then b,taken,ko=M.trial(g,p);if not b then return false end end
 g.history[#g.history+1]=M.key(g.b,g.turn);if #g.history>8 then table.remove(g.history,1) end
 if p==0 then g.passes=g.passes+1;g.ko=0 else g.b=b;g.captured[g.turn]=g.captured[g.turn]+taken;g.ko=ko;g.passes=0 end
 g.moves=g.moves+1;g.last=p;g.turn=3-g.turn;if g.passes>=2 or g.moves>=g.n*g.n*5 then g.phase=1 end;return true
end
function M.score(g)
 local b=g.b;for i=1,#b do if M.at(g.dead,i)==1 then b=M.put(b,i,0) end end
 local seen={};local score={0,g.komi}
 for i=1,#b do
  if M.at(b,i)>0 then score[M.at(b,i)]=score[M.at(b,i)]+2
  elseif not seen[i] then
   local region={i};seen[i]=true;local at,mask=1,0
   while at<=#region do for _,q in ipairs(M.neighbours(g.n,region[at])) do
    if M.at(b,q)>0 then mask=mask|(1<<M.at(b,q)) elseif not seen[q] then seen[q]=true;region[#region+1]=q end
   end;at=at+1 end
   if mask==2 then score[1]=score[1]+2*#region elseif mask==4 then score[2]=score[2]+2*#region end
  end
 end;return score[1],score[2]
end
function M.mark(g,p)
 if g.phase~=1 or p<1 or p>#g.b or M.at(g.b,p)==0 then return false end
 local stones=M.group(g.b,g.n,p);local v=1-M.at(g.dead,p);for _,q in ipairs(stones) do g.dead=M.put(g.dead,q,v) end;g.accepted=0;return true
end
function M.accept(g,s)
 if g.phase~=1 then return false end;g.accepted=g.accepted|(1<<s);if g.accepted==6 then g.phase=2 end;return true
end
function M.eye(g,p)
 for _,q in ipairs(M.neighbours(g.n,p)) do if M.at(g.b,q)~=g.turn then return false end end
 local r,c=(p-1)//g.n,(p-1)%g.n;local count,bad=0,0
 for dr=-1,1,2 do for dc=-1,1,2 do local y,x=r+dr,c+dc;if y>=0 and y<g.n and x>=0 and x<g.n then count=count+1;if M.at(g.b,y*g.n+x+1)==3-g.turn then bad=bad+1 end end end end
 return bad<=(count<4 and 0 or 1)
end
-- One-ply capture/liberty heuristic. Does not claim Michi strength or dead-stone judgement.
function M.ai(g)
 local best,value=0,-999
 for p=1,#g.b do if M.at(g.b,p)==0 and not M.eye(g,p) then
  local b,taken,_,libs=M.trial(g,p)
  if b then local v=taken*20+math.min(libs,4)*2;local r,c=(p-1)//g.n,(p-1)%g.n;v=v+math.min(r,c,g.n-1-r,g.n-1-c)
   if libs==1 then v=v-30 end
   if v>value then best,value=p,v end
  end
 end end
 return best
end
function M.valid(g)
 if g.n~=9 and g.n~=13 and g.n~=19 then return false end
 if type(g.b)~='string' or #g.b~=g.n*g.n or not g.b:match('^[012]+$') or type(g.dead)~='string' or #g.dead~=#g.b or not g.dead:match('^[01]+$') or not U.array(g.captured,2,0,10000) then return false end
 if g.turn<1 or g.turn>2 or g.phase<0 or g.phase>2 or g.moves<0 or g.moves>g.n*g.n*5 or g.ko<0 or g.ko>#g.b or g.passes<0 or g.passes>2 or #g.history>8 then return false end
 if g.komi~=1 and g.komi~=15 then return false end
 if g.handicap~=0 and (g.handicap<2 or g.handicap>5) then return false end
 if g.last<0 or g.last>#g.b or not U.array(g.history,#g.history,-2147483648,2147483647) then return false end
 if g.ui~=0 and g.ui~=1 and g.ui~=9 then return false end
 if g.phase==2 and g.accepted~=6 then return false end
 for i=1,#g.b do if M.at(g.dead,i)==1 and (M.at(g.b,i)==0 or g.phase==0) then return false end end
 return g.accepted>=0 and g.accepted<=6 and g.row>=1 and g.row<=g.n and g.solo>=0 and g.solo<=1
end
U.codec(M,{'seed','n','b','dead','turn','phase','moves','passes','ko','last','history','captured','komi','handicap','accepted','ui','row','col','solo'})
function M.pause(g)g.ui=0 end
local function ui()dofile('ui.lua')(M,U) end
function M.view(g)ui();return M.view(g) end
function M.act(g,i)ui();return M.act(g,i) end

return M
