return function(M,U,names,kinds,g)
local function duoCards(g,first)
 local a={};for c=1,58 do if g.place[c]==1 and kinds[c]<=4 and c~=first then
  local ok=not first
  if first then local x,y=kinds[first],kinds[c];ok=(x==y and x<=2) or (x==3 and y==4) or (x==4 and y==3) end
  if ok then a[#a+1]=c end
 end end;return a
end

 local lines={'Round '..g.round..' | Bank '..g.score[1]..' / '..g.score[2], 'Your cards: '..M.points(g,1)..' points | Deck '..(59-g.at)}
 for p=1,2 do local c=M.top(g,p);lines[#lines+1]='Pile '..p..': '..(c>0 and M.name(c) or 'empty') end
 if g.ui==3 or g.ui==4 then local o={};for _,c in ipairs(duoCards(g,g.ui==4 and g.first or nil)) do o[#o+1]=M.name(c) end;o[#o+1]='Cancel';return lines,o end
 if g.ui==9 then return lines,{'Cancel','Confirm new match'} end
 if g.ui==1 then local o={};for c=1,58 do if g.place[c]==1 or g.place[c]==3 or g.place[c]==4 or (g.place[c]==2 and (g.phase>=2 or (g.revealed&2)~=0)) then o[#o+1]=(g.place[c]==1 and 'Hand ' or g.place[c]==3 and 'Laid ' or 'Foe ')..M.name(c) end end;o[#o+1]='Back';return lines,o end
 if g.phase>=2 then return lines,{g.phase==2 and 'Next round' or ('Match over: '..(M.count(g,1,9,true)==4 and 'You' or M.count(g,2,9,true)==4 and 'Opponent' or g.score[1]>g.score[2] and 'You' or g.score[2]>g.score[1] and 'Opponent' or g.starter==1 and 'Opponent' or 'You')),'Inspect cards','New match','Exit'} end
 if g.turn==2 then return lines,{'Opponent decision','Inspect cards','New match','Exit'} end
 local o={};for _,m in ipairs(M.moves(g,nil,true)) do
  local k,x,y=m[1],m[2],m[3]
  o[#o+1]=k==1 and 'Draw two' or k==2 and ('Take pile '..x) or k==3 and ('Keep '..M.name(g.drawn[x])) or k==4 and ('Discard to '..x) or k==5 and ('Play '..names[kinds[x]+1]..' #'..x..' + #'..y) or k==6 and ('Dig pile '..x) or k==7 and ('Take '..M.name(x)) or k==8 and 'End turn' or x==0 and 'STOP' or 'LAST CHANCE'
 end;if g.step==3 then o[#o+1]='Play duo (choose two cards)' end;o[#o+1]='Inspect cards';o[#o+1]='New match';o[#o+1]='Exit';return lines,o
end
