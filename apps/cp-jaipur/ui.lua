return function(M,U,names,tokens)
function M.pause(g)g.ui=0 end
function M.view(g)
 local lines={'Round '..g.round..' Seals '..g.seals[1]..' / '..g.seals[2], 'You '..M.score(g,1)..' | Opponent goods '..g.points[2], 'Camels '..g.herd[1]..' / '..g.herd[2]..' | Deck '..(53-g.at)}
 local hand='';for k=1,6 do hand=hand..names[k]:sub(1,2)..':'..g.hands[1][k]..' ' end;lines[#lines+1]=hand
 if g.ui==2 then local o={};for k=1,6 do local t={};for i=g.depth[k]+1,#tokens[k] do t[#t+1]=tokens[k][i] end;o[#o+1]=names[k]..': '..table.concat(t,',') end;o[#o+1]='Back';return lines,o end
 if g.ui==9 then return lines,{'Cancel','Confirm new match'} end
 if g.phase>0 then lines[#lines+1]='Round score '..M.score(g,1)..' / '..M.score(g,2);return lines,{g.phase==1 and 'Next round' or 'Match over - new match','Exit'} end
 if g.turn==2 then return lines,{'Opponent moves','New match','Exit'} end
 local o={}
 if g.ui==1 then
  for i=1,5 do o[#o+1]=((g.mask&(1<<(i-1)))~=0 and '* ' or '')..'Take '..i..' '..names[g.market[i]] end
  for k=1,7 do o[#o+1]='Give '..names[k]..': '..g.give[k] end
  o[#o+1]='Confirm exchange';o[#o+1]='Cancel';return lines,o
 end
 for _,m in ipairs(M.moves(g)) do o[#o+1]=m[1]==1 and ('Take '..names[g.market[m[2]]]) or m[1]==2 and 'Take all camels' or ('Sell '..m[3]..' '..names[m[2]]) end
 o[#o+1]='Exchange 2+ cards';o[#o+1]='Inspect token piles';o[#o+1]='New match';o[#o+1]='Exit';return lines,o
end
function M.act(g,i)
 if g.ui==2 then g.ui=0;return end
 if g.ui==9 then if i==2 then local n=M.new(U.next(g.seed));for k,v in pairs(n) do g[k]=v end else g.ui=0 end;return end
 if g.phase>0 then if i==2 then return 'exit' end;if g.phase==2 then g.ui=9 else local w=M.winner(g);g.round=g.round+1;M.deal(g,U.next(g.seed),w>0 and 3-w or g.starter) end;return end
 if g.turn==2 then if i==1 then M.apply(g,M.ai(g)) elseif i==2 then g.ui=9 else return 'exit' end;return end
 if g.ui==1 then
  if i<=5 then g.mask=g.mask~(1<<(i-1)) elseif i<=12 then local k=i-5;g.give[k]=(g.give[k]+1)%((k==7 and math.min(g.herd[1],5) or g.hands[1][k])+1)
  elseif i==13 then M.apply(g,{3,g.mask,g.give}) else g.ui=0 end;return
 end
 local a=M.moves(g);if i<=#a then M.apply(g,a[i]) elseif i==#a+1 then g.ui=1;g.mask=0;g.give=U.zeros(7) elseif i==#a+2 then g.ui=2 elseif i==#a+3 then g.ui=9 else return 'exit' end
end
end
