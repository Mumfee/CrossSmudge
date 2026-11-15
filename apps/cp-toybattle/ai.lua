return function(M,U,g)
 local best,value=nil,-999
 M.moves(g,function(m)local v=0
  if #m==0 then v=U.sum(g.rack[g.turn])<3 and 30 or -5
  else local st=m[1];local t=M.board(g);local holder=M.top(g,st[1]);v=holder==3-g.turn and 12 or holder==0 and 8 or -2
   if st[1]>t.n then v=1000 end
   if (st[2]==2 or st[2]==6 or st[2]==7) and not (g.special==1 and t.special[st[1]]==7) then st[3]=1;v=v+3 end
  end
  if v>value then best,value=m,v end
 end);return best
end
