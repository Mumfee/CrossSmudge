return function(M,U,kinds,g)
 local best,value=nil,-999
 M.moves(g,function(m) local k,x=m[1],m[2];local v=0;local c=0
  if k==9 then v=100 elseif k==5 then v=30 elseif k==1 then v=5 elseif k==2 then c=M.top(g,x) elseif k==3 then c=g.drawn[x] elseif k==7 then c=x elseif k==6 then v=1 end
  if c>0 then local t=kinds[c];v=3+M.count(g,g.turn,t)*2+(t==9 and 5 or 0) end
  if v>value then best,value=m,v end
 end);return best
end
