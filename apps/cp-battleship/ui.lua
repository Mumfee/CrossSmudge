-- Stock v1.6.5.3 UI adapter. Tap signature is (x,y,event), per LuaRunner.cpp.
local U={}
function U.install(M)
 local S=dofile('storage.lua')
 local g,mode,screen,cursor,menu,flag,search=nil,1,'menu',1,1,false,nil
 local w,h,left,top,cell,bw,bh=480,800,0,0,0,0,0
 local notice='';local pending=1;local margin=24
 local items={'Resume','Rotate placement','Toggle own / target grid','New game','How to play','Save and exit'}
 local function geometry()
  w,h=smudge.get_bounds();margin=w>h and 64 or 24
  local metrics=smudge.get_metrics()
  local pad=math.max(20,metrics.content_side_padding or 20)
  top=math.max(100,(metrics.top_padding or 0)+(metrics.header_height or 50)+40)
  cell=math.floor(math.min((w-2*pad)/M.cols,(h-top-100)/M.rows))
  bw=cell*M.cols;bh=cell*M.rows;left=math.floor((w-bw)/2)
 end
 local function save()
  local ok=S.save(tostring(mode)..'|'..M.encode(g))
  if not ok then notice='SAVE FAILED - try Save and exit again' end
  return ok
 end
 local function refresh()smudge.request_update()end
 local function act()
  if screen~='board' or (not M.solo and mode==1 and g.turn==2) then return end
  local x=(cursor-1)%M.cols+1;local y=(cursor-1)//M.cols+1
  if M.action(g,x,y,flag) then notice='';search=nil;refresh() end
 end
 local function select_menu()
  if menu==1 then screen='board'
  elseif menu==2 then g.rotate=1-g.rotate;screen='board'
  elseif menu==3 then g.show=1-g.show;screen='board'
  elseif menu==4 then pending=1;screen='confirm';menu=1
  elseif menu==5 then screen='help'
  else if save() then smudge.exit() end end
  refresh()
 end
 function on_init()
  geometry()
  local loaded=S.load(function(p)
   local m,data=p:match('^([12])|(.*)$');if not m then return nil end
   local value=M.decode(data);if not value then return nil end
   return {g=value,mode=tonumber(m)}
  end)
  if loaded then g=loaded.g;mode=loaded.mode else g=M.new(smudge.millis());mode=1 end
  refresh()
 end
 function on_draw()
  geometry();smudge.clear();smudge.header(M.title,screen=='board' and M.message(g) or 'CrossPlay local port')
  smudge.rect(w-105,65,85,30,false);smudge.text(w-98,70,'Menu','small')
  if screen=='menu' then
   for i,item in ipairs(items) do
    local y=top+(i-1)*48
    smudge.text(margin,y,(i==menu and '> ' or '  ')..item,'ui12')
   end
  elseif screen=='confirm' then
   smudge.text(margin,top,'Replace the current board?','ui12')
   smudge.text(margin,top+55,menu==1 and '> Cancel' or '  Cancel','ui12')
   smudge.text(margin,top+103,menu==2 and '> Start new game' or '  Start new game','ui12')
  elseif screen=='help' then
   local y=top
   for _,line in ipairs(M.help) do
    local current=''
    for word in line:gmatch('%S+') do
     local next_line=current=='' and word or current..' '..word
     if smudge.text_width(next_line,'small')>w-margin-20 and current~='' then
      smudge.text(margin,y,current,'small');y=y+25;current=word
     else current=next_line end
    end
    smudge.text(margin,y,current,'small');y=y+30
   end
  else
   for i=1,M.cols*M.rows do
    local x=left+((i-1)%M.cols)*cell;local y=top+((i-1)//M.cols)*cell
    smudge.rect(x,y,cell,cell,false)
    local label=M.label(g,i)
    smudge.text(x+math.floor(cell/2)-5,y+math.floor(cell/2)-9,label,'small')
    if i==cursor then smudge.rect(x+2,y+2,cell-4,cell-4,false,2) end
   end
   local y=top+bh+8
   smudge.text(margin,y,search and 'Thinking... Back cancels' or (M.solo and (flag and 'FLAG mode' or 'DIG mode') or 'Solo: you fire first'),'small')
   if M.solo then smudge.rect(w-160,h-80,140,36,false);smudge.text(w-152,h-72,flag and 'Switch to Dig' or 'Switch to Flag','small') end
  end
  if notice~='' then smudge.text(margin,h-112,notice,'small') end
  smudge.button_hints('Menu','Select','Up','Down')
 end
 function on_button(btn,pressed)
  if not pressed then return end
  if btn=='page_back' then btn='up' elseif btn=='page_forward' then btn='down' end
  if btn=='back' then
   search=nil
   if screen=='menu' then if save() then smudge.exit() end
   elseif screen=='board' then save();screen='menu';menu=1
   else screen='menu';menu=1 end
  elseif screen=='menu' then
   if btn=='up' or btn=='left' then menu=(menu-2)%#items+1
   elseif btn=='down' or btn=='right' then menu=menu%#items+1
   elseif btn=='confirm' then select_menu() end
  elseif screen=='confirm' then
   if btn=='up' or btn=='down' or btn=='left' or btn=='right' then menu=3-menu
   elseif btn=='confirm' then
    if menu==2 then g=M.new(smudge.millis());mode=pending;cursor=1;flag=false;screen='board';notice=''
    else screen='menu' end
    menu=1
   end
  elseif screen=='board' then
   if btn=='left' then cursor=(cursor-2)%(M.cols*M.rows)+1
   elseif btn=='right' then cursor=cursor%(M.cols*M.rows)+1
   elseif btn=='up' then cursor=(cursor-1-M.cols)%(M.cols*M.rows)+1
   elseif btn=='down' then cursor=(cursor-1+M.cols)%(M.cols*M.rows)+1
   elseif btn=='confirm' then act()
    end
  end
  refresh()
 end
 function on_touch(x,y,event)
  if event~='tap' then return end
  geometry()
  if x<0 or y<0 or x>=w or y>=h then return end
  if x>=w-105 and x<w-20 and y>=65 and y<95 then on_button('back',true);return end
  if screen=='menu' then
   local i=math.floor((y-top)/48)+1
   if x>=20 and x<w-20 and i>=1 and i<=#items then menu=i;select_menu() end
  elseif screen=='confirm' then
   if y>=top+48 and y<top+144 then menu=y<top+96 and 1 or 2;on_button('confirm',true) end
  elseif screen=='help' then on_button('back',true)
  elseif screen=='board' then
   if M.solo and x>=w-160 and x<w-20 and y>=h-80 and y<h-44 then flag=not flag;refresh();return end
   if x>=left and x<left+bw and y>=top and y<top+bh then
    cursor=math.floor((y-top)/cell)*M.cols+math.floor((x-left)/cell)+1;act()
   end
  end

 end
 function on_update()
  if screen~='board' or M.solo or mode~=1 or g.turn~=2 or g.status~=0 then return end
  if M.begin then
   if not search then search=M.begin(g);refresh();return end
   local done,col=M.step(search,24)
   if not done then return end
   search=nil;if col then M.action(g,(col-1)%10+1,(col-1)//10+1) end
  else local col=M.choose(g);if col then M.action(g,(col-1)%10+1,(col-1)//10+1) end end
  refresh()
 end
 function on_swipe(direction)on_button(direction,true)end
 function on_exit()save();search=nil end
end
return U
