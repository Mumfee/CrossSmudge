return function(M,U,names,g,i)
 local t=M.board(g);g.notice=0
 if g.ui==9 then if i==1 then g.ui=0 else local map=i==2 and 0 or i-1;local n=M.new(U.next(g.seed),map);for k,v in pairs(n) do g[k]=v end end;return end
 if g.ui==1 then g.ui=0;return end
 if g.ui==2 then
  local st=g.edit
  if i<=8 then local max=({t.n+#t.hq,8,1,t.n,1,t.n,t.n,8})[i];local min=(i==1 or i==2 or i==8) and 1 or 0;st[i]=st[i]+1;if st[i]>max then st[i]=min end
  elseif i==9 then if #g.steps<3 then g.steps[#g.steps+1]=U.copy(st);g.edit={1,1,0,0,0,0,0,1} end
  elseif i==10 then local steps=U.copy(g.steps);steps[#steps+1]=U.copy(st);if M.apply(g,steps) then g.steps={};g.ui=0 else g.notice=1 end
  elseif i==11 then g.steps={} else g.ui=0 end;return
 end
 if g.phase==1 then if i==1 then g.ui=1 elseif i==2 then g.ui=9 else return 'exit' end;return end
 if g.turn==2 then if i==1 then if not M.apply(g,M.ai(g)) then g.notice=1 end elseif i==2 then g.ui=1 elseif i==3 then g.ui=9 else return 'exit' end;return end
 if i==1 then if not M.apply(g,{}) then g.notice=1 end elseif i==2 then g.ui=2;g.steps={};g.edit={1,1,0,0,0,0,0,1} elseif i==3 then g.ui=1 elseif i==4 then g.ui=9 else return 'exit' end
end
