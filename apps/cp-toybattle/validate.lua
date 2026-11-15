return function(M,U,g)
 if g.map<0 or g.map>9 or g.turn<1 or g.turn>2 or g.phase<0 or g.phase>1 or g.winner<0 or g.winner>2 or g.ending<0 or g.ending>3 then return false end
 local t=M.board(g)
 if #g.slots>48 or not U.array(g.slots,#g.tiles,1,t.n+#t.hq) or not U.array(g.tiles,#g.slots,0,15) or not U.array(g.taken,2,0,20) or not U.array(g.frozen,2,0,8) then return false end
 if not U.array(g.medals,2,0,40) or g.regions<0 or g.regions>=(1<<#t.regions) then return false end
 local bank=0;for i,r in ipairs(t.regions) do if (g.regions&(1<<(i-1)))~=0 then bank=bank+r[2] end end;if U.sum(g.medals)~=bank then return false end
 for s=1,2 do
  if not U.array(g.rack[s],8,0,3) or not U.array(g.discard[s],8,0,3) or U.sum(g.rack[s])>8 then return false end
  local count=U.zeros(8);local d=M.deck(g.seed,s);for i=1,g.taken[s] do count[d[i+4]]=count[d[i+4]]+1 end
  for k=1,8 do count[k]=count[k]-g.rack[s][k]-g.discard[s][k] end
  for _,tile in ipairs(g.tiles) do if tile//8+1==s then count[tile%8+1]=count[tile%8+1]-1 end end
  for _,v in ipairs(count) do if v~=0 then return false end end
 end
 return #g.steps<=3 and U.array(g.edit,8,0,40)
end
