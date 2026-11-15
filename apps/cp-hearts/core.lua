-- Hearts rules translated from CrossPlay; opponent is an explicitly simpler
-- public-information risk heuristic, not a claim of original Brain parity.
local U=dofile('support.lua');local M={title='Hearts'}
local function suit(c)return (c-1)//13 end -- Clubs, Diamonds, Spades, Hearts
local function rank(c)return (c-1)%13+2 end -- 2..Ace
local function penalty(c)return c==37 and 13 or (suit(c)==3 and 1 or 0) end
local function label(c)local r=rank(c);return (({[11]='J',[12]='Q',[13]='K',[14]='A'})[r] or tostring(r))..({'C','D','S','H'})[suit(c)+1] end
local function own(g,c)return tonumber(g.board:sub(c,c)) end
local function put(g,c,s)g.board=U.mark(g.board,c,tostring(s)) end
function M.hand(g,seat)local out={};for c=1,52 do if own(g,c)==seat then out[#out+1]=c end end;return out end
function M.new(seed)
 local g={rng=seed,phase=0,board=string.rep('0',52),turn=1,leader=1,trick='',broken=0,number=0,handno=0,selection='',t1=0,t2=0,t3=0,t4=0}
 M.deal(g);return g
end
function M.deal(g)
 local deck={};for c=1,52 do deck[c]=c end
 for i=52,2,-1 do local j=U.random(g,i);deck[i],deck[j]=deck[j],deck[i] end
 g.board=string.rep('0',52)
 for i,c in ipairs(deck) do put(g,c,(i-1)%4+1) end
 g.phase=0;g.turn=1;g.trick='';g.broken=0;g.number=0;g.selection=''
 if g.handno%4==3 then M.commit(g,{}) end
end
function M.legal(g,card)
 if g.phase~=1 or own(g,card)~=g.turn then return false end
 local hand=M.hand(g,g.turn)
 if #g.trick==0 then
  if g.number==0 then return card==1 end
  if suit(card)==3 and g.broken==0 then for _,c in ipairs(hand) do if suit(c)~=3 then return false end end end
 else
  local led=suit(g.trick:byte(1));local follows=false;local safe=false
  for _,c in ipairs(hand) do if suit(c)==led then follows=true end;if penalty(c)==0 then safe=true end end
  if follows then return suit(card)==led end
  if g.number==0 and penalty(card)>0 and safe then return false end
 end
 return true
end
function M.play(g,c)
 if not M.legal(g,c) then return false end
 put(g,c,0);g.trick=g.trick..string.char(c);if suit(c)==3 then g.broken=1 end
 if #g.trick==4 then g.phase=2 else g.turn=g.turn%4+1 end;return true
end
function M.winner(g)
 local high=g.trick:byte(1);local index=1
 for i=2,#g.trick do local c=g.trick:byte(i);if suit(c)==suit(high) and rank(c)>rank(high) then high=c;index=i end end
 return (g.leader+index-2)%4+1
end
function M.taken(g,seat)local n=0;for c=1,52 do if own(g,c)==seat+4 then n=n+penalty(c) end end;return n end
function M.score(g)
 local shooter=0;for s=1,4 do if M.taken(g,s)==26 then shooter=s end end
 local over=false
 for s=1,4 do local n=shooter>0 and (s==shooter and 0 or 26) or M.taken(g,s);local key='t'..s;g[key]=g[key]+n;if g[key]>=100 then over=true end end
 g.phase=over and 4 or 3
end
function M.sweep(g)
 if g.phase~=2 then return false end
 local winner=M.winner(g)
 for i=1,4 do put(g,g.trick:byte(i),winner+4) end
 g.trick='';g.number=g.number+1;g.turn=winner;g.leader=winner
 if g.number==13 then M.score(g) else g.phase=1 end;return true
end
function M.passpick(g,seat)
 local hand=M.hand(g,seat)
 table.sort(hand,function(a,b)local va=penalty(a)*4+rank(a);local vb=penalty(b)*4+rank(b);return va==vb and a>b or va>vb end)
 return string.char(hand[1],hand[2],hand[3])
end
function M.commit(g,passes)
 if g.phase~=0 then return false end
 if g.handno%4~=3 then
  for s=1,4 do local p=passes[s];if type(p)~='string' or #p~=3 then return false end
   for i=1,3 do local c=p:byte(i);if own(g,c)~=s then return false end;for j=1,i-1 do if c==p:byte(j) then return false end end end
  end
  local offset=({1,3,2})[g.handno%4+1]
  for s=1,4 do for i=1,3 do put(g,passes[s]:byte(i),(s-1+offset)%4+1) end end
 end
 g.turn=own(g,1);g.leader=g.turn;g.phase=1;g.selection='';return true
end
function M.choose(g)
 -- Only own hand and public trick enter the evaluation. Never sees other hands.
 local best,value=nil,-10000;local led=#g.trick>0 and suit(g.trick:byte(1)) or -1;local high=0;local blood=0
 for i=1,#g.trick do local c=g.trick:byte(i);blood=blood+penalty(c);if suit(c)==led then high=math.max(high,rank(c)) end end
 for _,c in ipairs(M.hand(g,g.turn)) do if M.legal(g,c) then
  local v
  if led<0 then v=-rank(c)-penalty(c)*5
  elseif suit(c)~=led then v=penalty(c)*30+rank(c)
  elseif rank(c)<high then v=100+rank(c)
  elseif #g.trick==3 and blood==0 then v=rank(c)
  else v=-rank(c)-blood*20-penalty(c)*30 end
  if v>value then best=c;value=v end
 end end
 return best
end
function M.valid(g)
 if #g.board~=52 or #g.selection>3 or #g.trick>4 then return false end
 local tablecards={};for i=1,#g.trick do local c=g.trick:byte(i);if c<1 or c>52 or tablecards[c] or own(g,c)~=0 then return false end;tablecards[c]=true end
 local spent=0;local counts={0,0,0,0}
 for c=1,52 do local s=own(g,c);if s==0 and not tablecards[c] then return false elseif s>4 then spent=spent+1 elseif s>0 then counts[s]=counts[s]+1 end end
 if spent~=g.number*4 then return false end
 for s=1,4 do
  local expected=13-g.number
  for i=1,#g.trick do if (g.leader+i-2)%4+1==s then expected=expected-1 end end
  if counts[s]~=expected then return false end
 end
 if g.phase==0 then
  if spent~=0 or #g.trick~=0 then return false end
  local seen={};for i=1,#g.selection do local c=g.selection:byte(i);if own(g,c)~=1 or seen[c] then return false end;seen[c]=true end
 end
 if (g.phase==2)~=(#g.trick==4) then return false end
 if g.phase==1 and g.turn~=(g.leader+#g.trick-1)%4+1 then return false end
 if g.phase>=3 and g.number~=13 then return false end
 return true
end
U.codec(M,{{'rng',-2147483648,2147483647},{'phase',0,4},{'board','^[0-8]+$',52},{'turn',1,4},{'leader',1,4},{'trick','^.*$',4},{'broken',0,1},{'number',0,13},{'handno',0,1000},{'selection','^.*$',3},{'t1',0,125},{'t2',0,125},{'t3',0,125},{'t4',0,125}})
function M.view(g)
 local scores='S '..g.t1..' W '..g.t2..' N '..g.t3..' E '..g.t4
 if g.phase==0 then
  local o={};for _,c in ipairs(M.hand(g,1)) do o[#o+1]=(g.selection:find(string.char(c),1,true) and '* ' or '')..label(c) end
  o[#o+1]='Commit pass';o[#o+1]='Save and exit'
  return {scores,'Pass 3 '..({'left','right','across','hold'})[g.handno%4+1],#g.selection..' selected (tap to toggle)'},o
 end
 if g.phase==1 then
  local trick='Table:';for i=1,#g.trick do trick=trick..' '..label(g.trick:byte(i)) end
  if g.turn~=1 then return {scores,trick,({'South','West','North','East'})[g.turn]..' to play'},{'Play opponent turn','Save and exit'} end
  local o={};for _,c in ipairs(M.hand(g,1)) do if M.legal(g,c) then o[#o+1]=label(c) end end;o[#o+1]='Save and exit'
  return {scores,trick,'Your legal cards (South)'},o
 end
 if g.phase==2 then local t='';for i=1,4 do t=t..label(g.trick:byte(i))..' ' end;return {scores,t,'Winner: '..({'South','West','North','East'})[M.winner(g)]},{'Collect trick','Save and exit'} end
 local winner=math.min(g.t1,g.t2,g.t3,g.t4);local names=''
 for i=1,4 do if g['t'..i]==winner then names=names..({'S','W','N','E'})[i]..' ' end end
 return {scores,g.phase==4 and ('Game over. Winner(s): '..names) or 'Hand complete','Low score wins; 100 ends game.'},{g.phase==4 and 'New match' or 'Next hand','Save and exit'}
end
function M.act(g,i)
 local _,o=M.view(g);if o[i]=='Save and exit' then return 'exit' end
 if g.phase==0 then
  local hand=M.hand(g,1)
  if i<=#hand then local ch=string.char(hand[i]);local at=g.selection:find(ch,1,true)
   if at then g.selection=g.selection:sub(1,at-1)..g.selection:sub(at+1) elseif #g.selection<3 then g.selection=g.selection..ch end
  elseif #g.selection==3 then M.commit(g,{g.selection,M.passpick(g,2),M.passpick(g,3),M.passpick(g,4)}) end
 elseif g.phase==1 then
  if g.turn~=1 then M.play(g,M.choose(g)) else local n=0;for _,c in ipairs(M.hand(g,1)) do if M.legal(g,c) then n=n+1;if n==i then M.play(g,c);break end end end end
 elseif g.phase==2 then M.sweep(g)
 elseif g.phase==3 then g.handno=g.handno+1;M.deal(g)
 elseif g.phase==4 then g.t1=0;g.t2=0;g.t3=0;g.t4=0;g.handno=0;M.deal(g) end
end
function M.pause()end
M.penalty=penalty;M.label=label
return M
