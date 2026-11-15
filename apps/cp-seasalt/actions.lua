return function(M,U,names,kinds,g,i)
local function duoCards(g,first)
 local a={};for c=1,58 do if g.place[c]==1 and kinds[c]<=4 and c~=first then
  local ok=not first
  if first then local x,y=kinds[first],kinds[c];ok=(x==y and x<=2) or (x==3 and y==4) or (x==4 and y==3) end
  if ok then a[#a+1]=c end
 end end;return a
end

 if g.ui==3 or g.ui==4 then local a=duoCards(g,g.ui==4 and g.first or nil);if i>#a then g.ui=0 elseif g.ui==3 then g.first=a[i];g.ui=4 else M.apply(g,{5,math.min(g.first,a[i]),math.max(g.first,a[i])});g.ui=0 end;return end
 if g.ui==9 then if i==2 then local n=M.new(U.next(g.seed));for k,v in pairs(n) do g[k]=v end else g.ui=0 end;return end
 if g.ui==1 then g.ui=0;return end
 if g.phase>=2 then if i==1 and g.phase==2 then g.round=g.round+1;M.deal(g,U.next(g.seed),g.ender~=0 and 3-g.ender or 3-g.starter) elseif i==2 then g.ui=1 elseif i==3 then g.ui=9 elseif i==4 then return 'exit' end;return end
 local a=g.turn==2 and {M.ai(g)} or M.moves(g,nil,true)
 if g.turn==1 and g.step==3 and i==#a+1 then g.ui=3;return end
 if g.turn==1 and g.step==3 and i>#a+1 then i=i-1 end
 if i<=#a then M.apply(g,a[i]) elseif i==#a+1 then g.ui=1 elseif i==#a+2 then g.ui=9 else return 'exit' end
end
