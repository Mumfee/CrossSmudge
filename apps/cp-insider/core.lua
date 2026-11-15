-- Translated from InsiderCore.cpp; MIT notices in this package.
local U=dofile('support.lua')
local M={title='Insider'}
function M.new(seed)
 return {rng=seed,players=4,phase=0,seat=1,roles='',word=0,left=300000,seen=string.rep('0',140),rounds=0,won=0,lost=0,timeout=0,result=0,recent=''}
end
function M.deal(g)
 local bag=string.rep('C',g.players-1)..'I';local drop=U.random(g,g.players)
 bag=bag:sub(1,drop-1)..bag:sub(drop+1)..'M'
 for i=g.players,2,-1 do local j=U.random(g,i);local a,b=bag:sub(i,i),bag:sub(j,j);bag=U.mark(U.mark(bag,i,b),j,a) end
 g.roles=bag;g.word=U.draw(g,1,560);g.seat=1;g.phase=1;g.left=300000;g.result=0
end
function M.finish(g,result)
 if g.phase==6 then return end
 g.phase=6;g.result=result;g.rounds=math.min(65535,g.rounds+1)
 local key=({'won','lost','timeout'})[result];g[key]=math.min(65535,g[key]+1)
 g.recent=(g.recent..result):sub(-16)
end
function M.vote(g,seat)
 if g.phase~=5 then return false end
 local insider=g.roles:find('I',1,true) or 0
 M.finish(g,seat==insider and 1 or 2);return true
end
function M.tick(g,ms)
 if g.phase~=3 then return end
 g.left=math.max(0,g.left-ms);if g.left==0 then M.finish(g,3) end
end
function M.valid(g)
 if #g.seen~=140 or #g.recent>g.rounds then return false end
 if g.phase==0 then return g.word==0 and g.roles=='' end
 if #g.roles~=g.players or g.seat>g.players or g.word<1 or not U.seen(g,g.word) then return false end
 local _,masters=g.roles:gsub('M','');local _,insiders=g.roles:gsub('I','')
 return masters==1 and insiders<=1 and (g.phase==6)==(g.result>0)
end
U.codec(M,{{'rng',-2147483648,2147483647},{'players',4,8},{'phase',0,6},{'seat',1,8},{'roles','^[CIM]*$',8},{'word',0,560},{'left',0,300000},{'seen','^[0-9a-f]+$',140},{'rounds',0,65535},{'won',0,65535},{'lost',0,65535},{'timeout',0,65535},{'result',0,3},{'recent','^[123]*$',16}})
function M.view(g)
 if g.phase==0 then return {'Players: '..g.players,'Win '..g.won..' / Lost '..g.lost..' / Time '..g.timeout,'Ask yes/no questions to find the word.','Then vote for the hidden Insider.'},{'Players +','Deal roles','Save and exit'} end
 if g.phase==1 then return {'Pass privately to player '..g.seat,'Only that player may reveal.'},{'Reveal role','Save and exit'} end
 if g.phase==2 then
  local r=g.roles:sub(g.seat,g.seat);return {'Player '..g.seat..': '..({C='Citizen',I='Insider',M='Master'})[r],r=='C' and 'You do not know the secret.' or U.word(g.word),'Hide before passing the device.'},{'Hide and pass','Save and exit'}
 end
 if g.phase==3 then return {'Ask the Master yes/no questions.', 'Seconds left: '..((g.left+999)//1000)},{'Word found - vote','Pause','Save and exit'} end
 if g.phase==4 then return {'Paused. Word and roles are hidden.'},{'Resume questions','Save and exit'} end
 if g.phase==5 then local options={'Nobody'};for i=1,g.players do options[#options+1]='Player '..i end;return {'Discuss, then accuse the Insider.'},options end
 return {({'Table wins','Wrong accusation','Out of time'})[g.result],'Word: '..U.word(g.word),'Insider: '..(g.roles:find('I',1,true) or 'Nobody')},{'Next round','Save and exit'}
end
function M.act(g,i)
 local _,o=M.view(g);if o[i]=='Save and exit' then return 'exit' end
 if g.phase==0 then if i==1 then g.players=g.players%8+1;if g.players<4 then g.players=4 end else M.deal(g) end
 elseif g.phase==1 then g.phase=2
 elseif g.phase==2 then g.seat=g.seat+1;if g.seat>g.players then g.seat=g.players;g.phase=4 else g.phase=1 end
 elseif g.phase==3 then g.phase=i==1 and 5 or 4
 elseif g.phase==4 then g.phase=3
 elseif g.phase==5 then M.vote(g,i-1)
 elseif g.phase==6 then M.deal(g) end
end
function M.pause(g)if g.phase==3 then g.phase=4 elseif g.phase==2 then g.phase=1 end end
M.word=U.word
function M.awake(g)return g.phase==3 end
return M
