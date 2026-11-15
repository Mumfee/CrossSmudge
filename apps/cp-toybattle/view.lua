return function(M,U,names,g)
 local t=M.board(g);local lines={t.name..' | Turn '..g.turn,'Medals '..g.medals[1]..' / '..g.medals[2]..' | goal '..t.goal,'Reserve '..(20-g.taken[1])..' / '..(20-g.taken[2])}
 if g.notice==1 then lines[#lines+1]='Illegal plan: edit or cancel.' end
 if g.phase==1 then lines[#lines+1]='Seat '..g.winner..' wins: '..({'HQ','Medals','No moves'})[g.ending] end
 if g.ui==9 then local o={'Cancel'};for i=0,9 do if i~=1 then o[#o+1]=({'Castle Field','Proving Ground','City of Clouds','Volcanic Jungle','Cursed Cemetery','Battlefield','Caribbean Sea','Tropical Pool','Station Metal-X','La Croisette'})[i+1] end end;return lines,o end
 if g.ui==1 then
  local o={};for p=1,t.n+#t.hq do local s,k=M.top(g,p);local links={};for q=1,t.n+#t.hq do if (t.adj[p]&(1<<(q-1)))~=0 then links[#links+1]=q end end;local adj=table.concat(links,',');o[#o+1]=p..': '..(p>t.n and ('HQ '..t.hq[p-t.n]) or s>0 and (s..' '..names[k]) or 'empty')..' links '..adj;if p<=t.n then o[#o+1]='Effect: '..({'None','Recall','Draw','Shove','Exhume','Suppress','Gate','Nullify'})[t.special[p]+1] end;if t.gate[p]~=0 then for k=1,8 do if (t.gate[p]&(1<<(k-1)))~=0 then o[#o+1]='Allowed: '..names[k] end end end end
  for i,r in ipairs(t.regions) do local a={};for p=1,t.n do if (r[1]&(1<<(p-1)))~=0 then a[#a+1]=p end end;o[#o+1]='Region '..i..': '..table.concat(a,',')..' = '..r[2] end;for k=1,8 do o[#o+1]=k..' '..names[k]..': '..g.rack[1][k]..(g.frozen[1]==k and ' FROZEN' or '') end;o[#o+1]='Back';return lines,o
 end
 if g.ui==2 then
  local st=g.edit;lines[#lines+1]='Plan: '..#g.steps..' queued; edit next step'
  return lines,{'Slot: '..st[1],'Troop: '..names[st[2]],'Troop effect: '..st[3],'Jumbo target: '..st[4],'Base effect: '..st[5],'Base from: '..st[6],'Base to: '..st[7],'Exhume troop: '..names[st[8]],'Queue step (Capn chain)','Apply queued + current','Clear plan','Cancel'}
 end
 local rack='';for k=1,8 do rack=rack..k..':'..g.rack[1][k]..' ' end;lines[#lines+1]='Your rack '..rack
 if g.phase==1 then return lines,{'Inspect map','New match','Exit'} end
 return lines,g.turn==2 and {'Opponent move','Inspect map','New match','Exit'} or {'Draw two','Build placement / chain','Inspect map','New match','Exit'}
end
