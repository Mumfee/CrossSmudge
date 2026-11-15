local Y=dofile('core.lua');local S=dofile('storage.lua')
local g=Y.new(1);local mode,screen,selection,menu=1,'menu',6,1
local search=nil;local notice='';local pending=1
local w,h,left,dieW,rowH,cardTop,rows=480,800,20,88,32,240,13
local menuItems={'Resume','New solo game','New two-player game','How to play','Save and exit'}
local function geometry()
 w,h=smudge.get_bounds();left=w>h and 64 or 20;dieW=(w-left-20)//5
 rows=w>h and 7 or 13;cardTop=210;rowH=math.max(16,math.min(32,(h-cardTop-55)//rows))
end
local function save()
 if not S.save(mode..'|'..Y.encode(g)) then notice='Save failed - retry before exit';return false end;return true
end
local function human()return mode==2 or g.turn==1 end
local function redraw()smudge.request_update()end
local function play()
 if not human() or Y.over(g) then return end
 if selection<=5 then Y.hold(g,selection)
 elseif selection==6 then Y.roll(g)
 elseif g.rolls>0 and Y.allowed(g.cards[g.turn],g.d,selection-6)then screen='take';menu=1 end
 redraw()
end
local function activate()
 if menu==1 then screen='board'
 elseif menu==4 then screen='help'
 elseif menu==5 then if save()then smudge.exit()end
 else pending=menu==2 and 1 or 2;screen='new';menu=1 end
 redraw()
end
function on_init()
 geometry()
 local old=S.load(function(s)
  local m,p=s:match('^([12])|(.*)$');local state=p and Y.decode(p)
  if state then return {mode=tonumber(m),g=state}end
 end)
 if old then mode=old.mode;g=old.g else g=Y.new(smudge.millis())end;redraw()
end
function on_draw()
 geometry();smudge.clear();smudge.header('CP Yahtzee','P1 '..Y.total(g.cards[1])..' / P2 '..Y.total(g.cards[2]))
 smudge.rect(w-105,65,85,30,false);smudge.text(w-98,70,'Menu','small')
 if screen=='menu' then
  for i,label in ipairs(menuItems)do smudge.text(left,105+(i-1)*42,(i==menu and '> ' or '  ')..label,'ui12')end
 elseif screen=='new' or screen=='take' then
  smudge.text(left,110,screen=='new' and 'Replace the current game?' or ('Take '..Y.names[selection-6]..' for '..Y.score_for(g.cards[g.turn],g.d,selection-6)..'?'),'small')
  smudge.text(left,170,(menu==1 and '> ' or '  ')..'Cancel','ui12')
  smudge.text(left,218,(menu==2 and '> ' or '  ')..'Confirm','ui12')
 elseif screen=='help' then
  local help={'Roll five dice, up to three rolls per turn.', 'Select dice to keep them; then select Roll.',
   'Select one empty category, then confirm its score.', 'Upper section: 63 or more earns 35 bonus points.',
   'Extra Yahtzee after a scored 50 earns 100 points.', 'Joker: matching upper box first, then a lower box.',
   'Solo uses exact one-reroll expected-value AI.', 'Nearby unavailable. Two players share this device.'}
  for i,line in ipairs(help)do smudge.text(left,105+(i-1)*30,line,'small')end
 else
  local status=Y.over(g) and (Y.total(g.cards[1])==Y.total(g.cards[2]) and 'Draw' or (Y.total(g.cards[1])>Y.total(g.cards[2]) and 'P1 wins' or 'P2 wins')) or ('P'..g.turn..' - roll '..g.rolls..'/3')
  smudge.text(left,72,status,'small')
  for i=1,5 do
   local x=left+(i-1)*dieW
   smudge.rect(x,108,dieW-4,48,false)
   if selection==i then smudge.rect(x+2,110,dieW-8,44,false,2)end
   smudge.text(x+12,120,g.rolls==0 and '-' or tostring(g.d[i]),'ui12')
   if g.held&(1<<(i-1))~=0 then smudge.text(x+dieW-38,125,'H','small')end
  end
  smudge.rect(left,165,125,34,false);smudge.text(left+8,172,(selection==6 and '> ' or '')..'Roll','small')
  smudge.text(left+145,172,search and 'Thinking (Back cancels)' or 'H = held','small')
  local colW=(w-left-20)//(rows==7 and 2 or 1)
  for c=1,13 do
   local column=(c-1)//rows;local r=(c-1)%rows;local x=left+column*colW;local y=cardTop+r*rowH
   local card=g.cards[g.turn];local value=card[c]~=-1 and tostring(card[c]) or
      (g.rolls>0 and Y.allowed(card,g.d,c) and ('['..Y.score_for(card,g.d,c)..']') or '-')
   smudge.text(x,y,(selection==c+6 and '> ' or '  ')..Y.names[c]..' '..value,'small')
  end
 end
 if notice~='' then smudge.text(left,h-75,notice,'small')end
 smudge.button_hints('Menu','Select','Up','Down')
end
function on_button(btn,pressed)
 if not pressed then return end
 if btn=='page_back' then btn='up' elseif btn=='page_forward' then btn='down' end
 if btn=='back' then
  search=nil
  if screen=='menu' then if save()then smudge.exit()end
  elseif screen=='board' then save();screen='menu';menu=1
  elseif screen=='take' then screen='board'
  else screen='menu';menu=1 end
 elseif screen=='menu' then
  if btn=='up' or btn=='left' then menu=(menu-2)%5+1 elseif btn=='down' or btn=='right' then menu=menu%5+1
  elseif btn=='confirm' then activate()end
 elseif screen=='new' or screen=='take' then
  if btn=='up' or btn=='down' or btn=='left' or btn=='right' then menu=3-menu
  elseif btn=='confirm' then
   if menu==2 then
    if screen=='new' then g=Y.new(smudge.millis());mode=pending;selection=6;search=nil
    else Y.take(g,selection-6);selection=6 end
    screen='board'
   else screen=screen=='new' and 'menu' or 'board' end;menu=1
  end
 elseif screen=='board' then
  if btn=='up' or btn=='left' then selection=(selection-2)%19+1
  elseif btn=='down' or btn=='right' then selection=selection%19+1
  elseif btn=='confirm' then play()end
 end
 redraw()
end
function on_touch(x,y,event)
 if event~='tap' or x<0 or y<0 or x>=w or y>=h then return end
 if x>=w-105 and x<w-20 and y>=65 and y<95 then on_button('back',true);return end
 if screen=='menu' then local i=(y-105)//42+1;if i>=1 and i<=5 then menu=i;activate()end
 elseif screen=='new' or screen=='take' then
  if y>=165 and y<255 then menu=y<213 and 1 or 2;on_button('confirm',true)end
 elseif screen=='help' then on_button('back',true)
 elseif screen=='board' then
  if y>=108 and y<156 and x>=left and x<left+dieW*5 then selection=(x-left)//dieW+1;play()
  elseif y>=165 and y<199 and x>=left and x<left+125 then selection=6;play()
  elseif y>=cardTop and y<cardTop+rows*rowH and x>=left and x<w-20 then
   local columns=rows==7 and 2 or 1;local colW=(w-left-20)//columns
   local c=(x-left)//colW*rows+(y-cardTop)//rowH+1
   if c<=13 then selection=c+6;play()end
  end
 end
end
function on_update()
 if screen~='board' or human() or Y.over(g)then return end
 if g.rolls==0 then Y.roll(g);redraw()
 elseif g.rolls<3 then
  if not search then search=Y.begin(g);redraw();return end
  local done,mask=Y.step(g,search)
  if done then search=nil;g.held=mask;Y.roll(g);redraw()end
 else local c=Y.choose_box(g.cards[g.turn],g.d);if c then Y.take(g,c)end;redraw()end
end
function on_swipe(dir)on_button(dir,true)end
function on_exit()save();search=nil end
