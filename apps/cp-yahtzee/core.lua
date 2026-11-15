-- CrossPlay YahtzeeCore/Brain rules, including Joker and exact one-roll AI.
local Y={names={'Ones','Twos','Threes','Fours','Fives','Sixes','Three alike','Four alike','Full house','Small straight','Large straight','Yahtzee','Chance'}}
local worth={2,4,6,8,10,12,22,13,22,29,24,17,22}
local maxima={5,10,15,20,25,30,30,30,25,30,40,50,30}
local counts={0,0,0,0,0,0}
local function facts(d)
 for i=1,6 do counts[i]=0 end
 local sum=0;for i=1,5 do counts[d[i]]=counts[d[i]]+1;sum=sum+d[i] end
 local largest,run,longest=0,0,0;local pair=false;local triple=false
 for i=1,6 do
  largest=math.max(largest,counts[i]);run=counts[i]>0 and run+1 or 0;longest=math.max(longest,run)
  if counts[i]==2 then pair=true elseif counts[i]==3 then triple=true end
 end
 return sum,largest,longest,(pair and triple) or largest==5
end
function Y.raw(d,c)
 local sum,largest,run,house=facts(d)
 if c<=6 then return counts[c]*c elseif c==7 then return largest>=3 and sum or 0
 elseif c==8 then return largest>=4 and sum or 0 elseif c==9 then return house and 25 or 0
 elseif c==10 then return run>=4 and 30 or 0 elseif c==11 then return run==5 and 40 or 0
 elseif c==12 then return largest==5 and 50 or 0 else return sum end
end
local function five(d)return d[1]==d[2] and d[1]==d[3] and d[1]==d[4] and d[1]==d[5]end
function Y.allowed(card,d,c)
 if c<1 or c>13 or card[c]~=-1 then return false end
 if not five(d) or card[12]==-1 then return true end
 if card[d[1]]==-1 then return c==d[1] end
 for i=7,13 do if card[i]==-1 then return c>=7 end end
 return true
end
function Y.score_for(card,d,c)
 if five(d) and card[12]~=-1 and card[d[1]]~=-1 and c>=9 and c<=11 then return maxima[c] end
 return Y.raw(d,c)
end
function Y.upper(card)local n=0;for i=1,6 do n=n+math.max(0,card[i])end;return n end
function Y.total(card)
 local n=Y.upper(card)>=63 and 35 or 0;n=n+card[14]*100
 for i=1,13 do n=n+math.max(0,card[i]) end;return n
end
function Y.over(g)
 for p=1,2 do for i=1,13 do if g.cards[p][i]==-1 then return false end end end;return true
end
function Y.new(seed)
 local g={cards={{},{}},d={1,1,1,1,1},held=0,rolls=0,turn=1,rng=seed}
 for p=1,2 do for i=1,13 do g.cards[p][i]=-1 end;g.cards[p][14]=0 end
 return g
end
function Y.roll(g)
 if Y.over(g) or g.rolls==3 then return false end
 for i=1,5 do if g.rolls==0 or g.held&(1<<(i-1))==0 then
  local s=g.rng;if s==0 then s=0x9e3779b9 end;s=s~(s<<13);s=s~(s>>17);s=s~(s<<5);g.rng=s
  g.d[i]=((s&0x7fffffff)%6+(s<0 and 2 or 0))%6+1
 end end
 if g.rolls==0 then g.held=0 end;g.rolls=g.rolls+1;return true
end
function Y.hold(g,i)
 if Y.over(g) or g.rolls==0 or g.rolls==3 or i<1 or i>5 then return false end
 g.held=g.held~(1<<(i-1));return true
end
function Y.take(g,c)
 local card=g.cards[g.turn]
 if Y.over(g) or g.rolls==0 or not Y.allowed(card,g.d,c) then return false end
 if five(g.d) and card[12]==50 then card[14]=card[14]+1 end
 card[c]=Y.score_for(card,g.d,c);g.held=0;g.rolls=0;g.turn=3-g.turn;return true
end
local function value(card,d,c)
 local score=Y.score_for(card,d,c);local v=score*10
 if c<=6 then
  local best=Y.upper(card)+score
  for i=1,6 do if card[i]==-1 and i~=c then best=best+i*5 end end
  if best>=63 then v=v+score*50//9 end
 end
 return v-worth[c]*10
end
function Y.choose_box(card,d)
 local best,v=nil,-99999
 for c=1,13 do if Y.allowed(card,d,c) then local n=value(card,d,c);if n>v then best,v=c,n end end end
 return best,v
end
local factorial={1,1,2,6,24,120}
-- One hold mask per step. Enumeration depth <=5; exact multinomial weights.
function Y.mask_value(card,d,mask)
 local hand={};local kept=0
 for i=1,5 do if mask&(1<<(i-1))~=0 then kept=kept+1;hand[kept]=d[i] end end
 local free=5-kept;local rolled={};local total,weight=0,0
 local function visit(pos,low)
  if pos>free then
   for i=1,free do hand[kept+i]=rolled[i] end
   local denom=1;local run=1
   for i=2,free+1 do
    if i<=free and rolled[i]==rolled[i-1] then run=run+1
    else denom=denom*factorial[run+1];run=1 end
   end
   local ways=factorial[free+1]//denom
   local box,v=Y.choose_box(card,hand);total=total+(box and v or 0)*ways;weight=weight+ways
   return
  end
  for f=low,6 do rolled[pos]=f;visit(pos+1,f)end
 end
 visit(1,1)
 -- weight is exactly 6^free and divides 7776. Divide first to avoid int32 overflow.
 return total*(7776//weight),kept
end
function Y.begin(g)return {mask=0,best=0,value=-2147483647,kept=6}end
function Y.step(g,s)
 local v,k=Y.mask_value(g.cards[g.turn],g.d,s.mask)
 if v>s.value or (v==s.value and k<s.kept) then s.best=s.mask;s.value=v;s.kept=k end
 s.mask=s.mask+1;return s.mask==32,s.best
end
function Y.encode(g)
 local a={g.rng,g.turn,g.rolls,g.held}
 for i=1,5 do a[#a+1]=g.d[i]end
 for p=1,2 do for i=1,14 do a[#a+1]=g.cards[p][i]end end
 return table.concat(a,',')
end
function Y.decode(s)
 if type(s)~='string' or #s>180 or s:find('[^%d,-]') then return nil end
 local a={};for token in (s..','):gmatch('(.-),')do
  if not token:match('^-?%d+$') then return nil end
  local n=tonumber(token);if math.type(n)~='integer' then return nil end;a[#a+1]=n
 end
 if #a~=37 or a[2]<1 or a[2]>2 or a[3]<0 or a[3]>3 or a[4]<0 or a[4]>31 or (a[3]==0 and a[4]~=0)then return nil end
 local g=Y.new(a[1]);g.turn=a[2];g.rolls=a[3];g.held=a[4]
 for i=1,5 do if a[4+i]<1 or a[4+i]>6 then return nil end;g.d[i]=a[4+i]end
 local filled={0,0}
 for p=1,2 do
  local card=g.cards[p]
  for i=1,14 do
   local n=a[9+(p-1)*14+i];card[i]=n
   if i==14 then if n<0 or n>12 or (n>0 and card[12]~=50)then return nil end
   elseif n~=-1 then
    filled[p]=filled[p]+1
    if n<0 or n>maxima[i] or (i<=6 and n%i~=0) or (i>=9 and i<=12 and n~=0 and n~=maxima[i]) or ((i==7 or i==8 or i==13) and n~=0 and n<5)then return nil end
   end
  end
 end
 if filled[1]~=filled[2] and filled[1]~=filled[2]+1 then return nil end
 if not Y.over(g) and g.turn~=(filled[1]==filled[2] and 1 or 2)then return nil end
 if Y.over(g) and (g.rolls~=0 or g.held~=0)then return nil end
 return g
end
return Y
