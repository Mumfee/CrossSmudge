local U=dofile('support.lua');local count=dofile('deck.lua');local M={title='Wavelength'}
function M.new(seed)return {rng=seed,phase=0,mode=1,seen=string.rep('0',(count+3)//4),a=0,b=0,pair=0,target=1,guess=10,round=0,total=0,points=0,call=0} end
function M.score(guess,target,call,mode)
 if guess<1 or guess>20 or target<1 or target>20 then return 0 end
 local d=math.abs(guess-target);local points=d==0 and 5 or (d==1 and 3 or (d==2 and 1 or 0))
 if mode==2 and ((target>guess and call==1) or (target<guess and call==2)) then points=points+1 end
 return points
end
function M.deal(g)
 -- Reserve two choices only until one is picked; unchosen pair remains unseen.
 local left=0;for i=1,count do if not U.seen(g,i) then left=left+1 end end
 if left==0 then g.seen=string.rep('0',(count+3)//4);left=count end
 g.a=U.draw(g,1,count);g.b=left>1 and U.draw(g,1,count) or 0
 U.setseen(g,g.a,false);if g.b>0 then U.setseen(g,g.b,false) end
 g.phase=1;g.pair=0;g.guess=10;g.target=U.random(g,20);g.call=0
end
function M.record(g,call)
 g.call=call;g.points=g.round==0 and 0 or M.score(g.guess,g.target,call,g.mode)
 g.total=math.min(65535,g.total+g.points);g.round=math.min(65535,g.round+1);g.phase=5
end
function M.valid(g)
 if #g.seen~=(count+3)//4 then return false end
 if g.phase>0 and (g.a<1 or g.a==g.b) then return false end
 if g.phase>1 and (g.pair<1 or not U.seen(g,g.pair)) then return false end
 return true
end
U.codec(M,{{'rng',-2147483648,2147483647},{'phase',0,6},{'mode',1,2},{'seen','^[0-9a-f]+$',(count+3)//4},{'a',0,count},{'b',0,count},{'pair',0,count},{'target',1,20},{'guess',1,20},{'round',0,65535},{'total',0,65535},{'points',0,6},{'call',0,2}})
local function ends(g)local a,b=U.word(g.pair):match('^(.-) / (.*)$');return '20: '..(a or ''),'1: '..(b or '') end
function M.view(g)
 local top,bottom='','';if g.pair>0 then top,bottom=ends(g) end
 if g.phase==0 then return {'One clue-giver sees a secret slot.','Others guess from 1 (bottom) to 20 (top).','First round is practice: zero points.'},{g.mode==1 and 'Mode: Co-op (toggle)' or 'Mode: Teams (toggle)','Start session','Save and exit'} end
 if g.phase==1 then
  local a,b=U.word(g.a):match('^(.-) / (.*)$');local lines={'Clue-giver: choose spectrum','1 top: '..a,'1 bottom: '..b};local opts={'Use spectrum 1'}
  if g.b>0 then a,b=U.word(g.b):match('^(.-) / (.*)$');lines[#lines+1]='2 top: '..a;lines[#lines+1]='2 bottom: '..b;opts[#opts+1]='Use spectrum 2' end
  opts[#opts+1]='Save and exit';return lines,opts
 end
 if g.phase==6 then return {'Pass privately to the clue-giver.'},{'Reveal target','Save and exit'} end
 if g.phase==2 then return {top,bottom,'SECRET TARGET: '..g.target,'Say a clue, then hide the target.'},{'Hide - pass to guessers','Save and exit'} end
 if g.phase==3 then return {top,bottom,'Guess: '..g.guess..' (20 top / 1 bottom)','Total '..g.total..(g.round==0 and ' - PRACTICE' or '')},{'Guess +1','Guess -1','Lock guess','Save and exit'} end
 if g.phase==4 then return {top,bottom,'Guess locked: '..g.guess,'Opposing team: which side is target?'},{'Toward top','Toward bottom','Save and exit'} end
 return {top,bottom,'Target '..g.target..' / Guess '..g.guess,'Scored '..g.points..'; Total '..g.total,'Round '..g.round..(g.round==1 and ' was practice' or '')},{'Next clue-giver','Save and exit'}
end
function M.act(g,i)
 local _,o=M.view(g);if o[i]=='Save and exit' then return 'exit' end
 if g.phase==0 then if i==1 then g.mode=3-g.mode else M.deal(g) end
 elseif g.phase==1 then g.pair=i==1 and g.a or g.b;U.setseen(g,g.pair,true);g.phase=2
 elseif g.phase==2 then g.phase=3
 elseif g.phase==3 then if i==1 then g.guess=math.min(20,g.guess+1) elseif i==2 then g.guess=math.max(1,g.guess-1) elseif g.mode==2 then g.phase=4 else M.record(g,0) end
 elseif g.phase==4 then M.record(g,i)
 elseif g.phase==5 then M.deal(g) elseif g.phase==6 then g.phase=2 end
end
function M.pause(g)if g.phase==2 then g.phase=6 end end
M.word=U.word;M.count=count
return M
