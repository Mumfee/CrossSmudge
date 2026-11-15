return function(M,U)
function M.pause(g)g.ui=0 end
function M.view(g)
 local lines={(g.turn==1 and 'Black' or 'White')..' to move | '..g.n..'x'..g.n..' | move '..g.moves,'B captures '..g.captured[1]..' W '..g.captured[2]..' | Komi '..(g.komi/2)}
 if g.ui==9 then return lines,{'Cancel','New 9x9 shared','New 13x13 shared','New 19x19 shared','New 9x9 vs heuristic','New 9x9 handicap 2','New 9x9 handicap 3','New 9x9 handicap 4','New 9x9 handicap 5'} end
 if g.ui==1 then
  lines[#lines+1]='Row '..g.row..' (. empty B/W stone x dead)';local s='';for c=1,g.n do local p=(g.row-1)*g.n+c;s=s..(M.at(g.dead,p)==1 and 'x' or ({'.','B','W'})[M.at(g.b,p)+1]) end;lines[#lines+1]=s
  local o={};for c=1,g.n do local p=(g.row-1)*g.n+c;o[#o+1]=(g.phase==1 and 'Toggle group at ' or 'Play at ')..g.row..','..c..' '..({'.','B','W'})[M.at(g.b,p)+1] end;o[#o+1]='Back';return lines,o
 end
 if g.phase>0 then local a,b=M.score(g);lines[#lines+1]='Area: Black '..(a/2)..' White '..(b/2);lines[#lines+1]=g.phase==2 and (a>b and 'Black wins' or 'White wins') or 'Agree dead groups before accepting' end
 local o={};for r=1,g.n do o[#o+1]='Inspect row '..r end
 if g.phase==0 then o[#o+1]=(g.solo==1 and g.turn==2) and 'Opponent move' or 'Pass'
 elseif g.phase==1 then o[#o+1]='Black accepts';o[#o+1]='White accepts';o[#o+1]='Resume play (disagreement)' end
 o[#o+1]='New game';o[#o+1]='Exit';return lines,o
end
function M.act(g,i)
 if g.ui==9 then
  if i==1 then g.ui=0 else local n=M.new(U.next(g.seed),i==3 and 13 or i==4 and 19 or 9,i>=6 and i-4 or 0);n.solo=i==5 and 1 or 0;for k,v in pairs(n) do g[k]=v end end;return
 end
 if g.ui==1 then if i<=g.n then local p=(g.row-1)*g.n+i;if g.phase==1 then M.mark(g,p) elseif g.phase==0 and not (g.solo==1 and g.turn==2) then M.play(g,p) end else g.ui=0 end;return end
 if i<=g.n then g.row=i;g.ui=1;return end
 i=i-g.n
 if g.phase==0 then if i==1 then M.play(g,g.solo==1 and g.turn==2 and M.ai(g) or 0);return end;i=i-1
 elseif g.phase==1 then if i<=2 then M.accept(g,i);return elseif i==3 then g.phase=0;g.passes=0;g.dead=string.rep('0',#g.b);g.accepted=0;return end;i=i-3 end
 if i==1 then g.ui=9 else return 'exit' end
end
end
