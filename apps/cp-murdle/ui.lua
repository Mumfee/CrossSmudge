return function(M,U)
function M.pause(g)g.ui=0 end
local function wrap(o,s)while #s>42 do local cut=42;for i=42,20,-1 do if s:sub(i,i)==' ' then cut=i;break end end;o[#o+1]=s:sub(1,cut);s=s:sub(cut+1) end;o[#o+1]=s end
function M.view(g)
 local p=M.case(g);local lines={'Case '..g.id..' | '..({'Elementary','Nosy','Hard-boiled','Impossible'})[(g.id-1)//16+1],g.phase==1 and 'Solved! Attempts '..g.attempts or (g.attempts>0 and ('Incorrect accusation; attempts '..g.attempts) or 'Use clues and notes; submit accusation.')}
 if g.id>48 then lines[#lines+1]='Only the murderer lies when speaking.' end
 if g.ui==9 then local o={'Cancel'};for tier=1,4 do o[#o+1]='Next '..({'Elementary','Nosy','Hard-boiled','Impossible'})[tier]..' case' end;return lines,o end
 if g.ui==1 or g.ui==2 then local o={};for i,s in ipairs(g.ui==1 and p.text or p.dossier) do wrap(o,(g.ui==1 and (i..'. ') or '')..s) end;o[#o+1]='Back';return lines,o end
 if g.ui==3 then
  local pair=M.blocks(p)[g.block];local o={}
  for r=1,p.n do for c=1,p.n do o[#o+1]=p.labels[pair[1]][r]..' / '..p.labels[pair[2]][c]..': '..({'?','NO','YES'})[M.shown(g,g.block,r,c)+1] end end;o[#o+1]='Back';return lines,o
 end
 local o={'Read clues','Dossier'};for _,pair in ipairs(M.blocks(p)) do o[#o+1]=({'Suspect','Weapon','Location','Motive'})[pair[1]]..' / '..({'Suspect','Weapon','Location','Motive'})[pair[2]] end
 for c=1,p.cats do o[#o+1]='Accuse '..p.labels[c][g.picks[c]] end;o[#o+1]='Submit accusation';o[#o+1]='Choose next case';o[#o+1]='Exit';return lines,o
end
function M.act(g,i)
 local p=M.case(g)
 if g.ui==9 then if i==1 then g.ui=0 else local tier=i-2;local id=tier*16+((g.id-1)%16+1)%16+1;local n=M.new(g.seed,id);for k,v in pairs(n) do g[k]=v end end;return end
 if g.ui==1 or g.ui==2 then g.ui=0;return end
 if g.ui==3 then if i<=p.n*p.n then M.tap(g,g.block,(i-1)//p.n+1,(i-1)%p.n+1) else g.ui=0 end;return end
 local b=p.cats*(p.cats-1)//2
 if i<=2 then g.ui=i elseif i<=b+2 then g.block=i-2;g.ui=3 elseif i<=b+2+p.cats then local c=i-b-2;g.picks[c]=g.picks[c]%p.n+1 elseif i==b+3+p.cats then M.accuse(g) elseif i==b+4+p.cats then g.ui=9 else return 'exit' end
end
end
