return function(M,U,g,steps)
local draw,pop,push,settle,win,blind,adjacent=M._draw,M._pop,M._push,M._settle,M._win,M._blind,M._adjacent
local function base(g,s,st)
 local t=M.board(g);local p,from,to,k=st[1],st[6],st[7],st[8];local kind=t.special[p];local foe=3-s
 if kind==2 then if U.sum(g.rack[s])==8 or g.taken[s]==20 then return false end;draw(g,s,1)
 elseif kind==4 then if U.sum(g.rack[s])==8 or k<1 or k>8 or g.discard[s][k]==0 then return false end;g.discard[s][k]=g.discard[s][k]-1;g.rack[s][k]=g.rack[s][k]+1
 elseif kind==1 then if U.sum(g.rack[s])==8 or from==p or from<1 or from>t.n or M.top(g,from)~=s then return false end;local _,troop=pop(g,from);g.rack[s][troop]=g.rack[s][troop]+1;settle(g)
 elseif kind==3 then if from<1 or from>t.n or to<1 or to>t.n or not adjacent(t,p,from) or not adjacent(t,from,to) or M.top(g,from)~=foe then return false end;local who,troop=pop(g,from);push(g,to,who,troop);settle(g)
 elseif kind==5 then local picked=blind(g,foe,0xb1f1e1d0);if picked==0 then return false end;g.frozen[foe]=picked
 else return false end;return true
end
local function step(g,s,st,chain)
 local t=M.board(g);local p,k,e,target=st[1],st[2],st[3]==1,st[4]
 if not U.array(st,8,0,40) or not M.canplace(g,s,p,k,e) then return false end
 local special=p<=t.n and t.special[p] or 0
 if g.special==1 and special==7 and e then return false end
 if st[5]==1 and not (g.special==1 and special>=1 and special<=5) then return false end
 if chain~=(k==3 and e) or (e and (k==1 or k==8)) then return false end
 g.rack[s][k]=g.rack[s][k]-1;push(g,p,s,k)
 if p>t.n then win(g,s,1);return true end;settle(g);if g.phase==1 then return true end
 if e then
  if k==2 then draw(g,s,2) elseif k==7 then draw(g,s,1)
  elseif k==4 then if target<1 or target>t.n or not adjacent(t,p,target) or M.top(g,target)~=3-s then return false end;local who,troop=pop(g,target);g.discard[who][troop]=g.discard[who][troop]+1;settle(g)
  elseif k==6 then local foe=3-s;local picked=blind(g,foe,0xb5000000);if picked>0 then g.rack[foe][picked]=g.rack[foe][picked]-1;g.discard[foe][picked]=g.discard[foe][picked]+1;if g.frozen[foe]==picked and g.rack[foe][picked]==0 then g.frozen[foe]=0 end end end
 end;return true
end
local function apply(g,steps)
 if g.phase~=0 or #steps>4 then return false end
 local trial=U.copy(g);local s=g.turn
 if #steps==0 then if trial.taken[s]==20 or U.sum(trial.rack[s])==8 then return false end;draw(trial,s,2)
 else
  for i,st in ipairs(steps) do if not step(trial,s,st,i<#steps) then return false end;if trial.phase==1 then break end end
  if trial.phase==0 then for _,st in ipairs(steps) do if st[5]==1 and not base(trial,s,st) then return false end;if trial.phase==1 then break end end end
 end
 if trial.phase==0 then trial.frozen[s]=0;trial.turn=3-s;local any=false;M.moves(trial,function()any=true end);if not any then win(trial,trial.medals[3-s]>trial.medals[s] and 3-s or s,3) end end
 for k,v in pairs(trial) do g[k]=v end;return true
end
return apply(g,steps)
end
