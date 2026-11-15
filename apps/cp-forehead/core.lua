-- CrossPlay Forehead rules and complete deck; see license and PARITY.md.
local U=dofile('support.lua');local cats=dofile('categories.lua');local M={title='Forehead'}
function M.new(seed)return {rng=seed,phase=0,category=1,length=60,left=60000,seen=string.rep('0',619),history='',word=0,got=0,rounds=0,total=0,best=0,review=1,recent='',bestIn=string.rep('000',17)} end
function M.finish(g)
 if g.phase==3 then return end
 g.phase=3;g.rounds=math.min(65535,g.rounds+1);g.total=math.min(65535,g.total+g.got);g.best=math.max(g.best,g.got)
 g.recent=(g.recent..string.format('%03d',g.got)):sub(-48)
 local off=(g.category-1)*3;local old=tonumber(g.bestIn:sub(off+1,off+3))
 g.bestIn=g.bestIn:sub(1,off)..string.format('%03d',math.max(old,g.got))..g.bestIn:sub(off+4)
end
function M.next(g)
 if #g.history>=640 then g.word=0;M.finish(g);return end
 local c=cats[g.category];g.word=U.draw(g,c[3],c[4],g.history)
 if g.word==0 then M.finish(g) end
end
function M.begin(g)
 g.history='';g.got=0;g.review=1;g.left=g.length*1000;g.phase=2;M.next(g)
end
function M.answer(g,mark)
 if g.phase~=1 or g.word==0 then return false end
 g.history=g.history..string.format('%04x%d',g.word,mark)
 if mark==1 then g.got=g.got+1 end
 M.next(g);return true
end
function M.tick(g,ms)
 if g.phase~=1 then return end
 g.left=math.max(0,g.left-ms)
 if g.left==0 then
  if g.word>0 and #g.history<640 then g.history=g.history..string.format('%04x3',g.word) end
  g.word=0;M.finish(g)
 end
end
function M.valid(g)
 if #g.seen~=619 or #g.history%5~=0 or #g.bestIn~=51 or #g.recent%3~=0 or g.left>g.length*1000 then return false end
 if g.length%30~=0 then return false end
 local used={};local got=0;local c=cats[g.category]
 for i=1,#g.history,5 do
  local k=tonumber(g.history:sub(i,i+3),16);local m=g.history:sub(i+4,i+4)
  if not k or k<c[3] or k>=c[3]+c[4] or used[k] or not m:match('[123]') or not U.seen(g,k) then return false end
  used[k]=true;if m=='1' then got=got+1 end
 end
 if got~=g.got then return false end
 if g.phase==1 or g.phase==2 then return g.word>=c[3] and g.word<c[3]+c[4] and not used[g.word] and U.seen(g,g.word) end
 return true
end
U.codec(M,{{'rng',-2147483648,2147483647},{'phase',0,3},{'category',1,17},{'length',30,120},{'left',0,120000},{'seen','^[0-9a-f]+$',619},{'history','^[0-9a-f]*$',640},{'word',0,2474},{'got',0,128},{'rounds',0,65535},{'total',0,65535},{'best',0,128},{'review',1,128},{'recent','^%d*$',48},{'bestIn','^%d+$',51}})
function M.view(g)
 local c=cats[g.category]
 if g.phase==0 then return {c[1],c[2],g.length..' seconds; best '..g.best,'Hold screen outward. Others give clues.'},{'Next category','Previous category','Round length +30','Start round','Save and exit'} end
 if g.phase==2 then return {'Ready / Paused - screen outward',c[1],'Got: Confirm; Miss: choose Missed'},{'Start / resume clock','Save and exit'} end
 if g.phase==1 then return {U.word(g.word),c[2],'Got '..g.got..' / '..((g.left+999)//1000)..' seconds'},{'Got it','Missed','Pause','Save and exit'} end
 local n=#g.history//5;local i=math.min(g.review,math.max(1,n));local off=(i-1)*5
 local word=tonumber(g.history:sub(off+1,off+4),16);local mark=tonumber(g.history:sub(off+5,off+5))
 return {'Got '..g.got..' of '..n..'; best '..g.best,word and U.word(word) or 'No cards',i..' / '..n..': '..(mark and ({'Got','Missed','Unanswered'})[mark] or '')},{'Next result','Previous result','Choose next round','Save and exit'}
end
function M.act(g,i)
 local _,o=M.view(g);if o[i]=='Save and exit' then return 'exit' end
 if g.phase==0 then
  if i==1 then g.category=g.category%17+1 elseif i==2 then g.category=(g.category-2)%17+1 elseif i==3 then g.length=g.length%120+30 else M.begin(g) end
 elseif g.phase==2 then g.phase=1
 elseif g.phase==1 then if i<3 then M.answer(g,i) else g.phase=2 end
 elseif g.phase==3 then if i==3 then g.phase=0;g.history='';g.got=0;g.word=0 else local n=math.max(1,#g.history//5);g.review=(g.review+(i==1 and 0 or -2))%n+1 end end
end
function M.pause(g)if g.phase==1 then g.phase=2 end end
M.word=U.word;M.categories=cats
function M.awake(g)return g.phase==1 end
return M
