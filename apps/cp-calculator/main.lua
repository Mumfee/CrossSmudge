local C=dofile('core.lua');local S=dofile('storage.lua')
local g=C.new();local selected=1;local menu=false;local notice=''
local keys={'AC','CE','BS','/', '7','8','9','*', '4','5','6','-', '1','2','3','+', '+/-','0','.','=', '%'}
local w,h,left,top,cw,ch=480,800,20,160,110,70
local function geometry()
 w,h=smudge.get_bounds();left=w>h and 64 or 20;top=150
 cw=math.floor((w-left-20)/4);ch=math.floor((h-top-60)/6)
end
local function save()
 if not S.save(C.encode(g)) then notice='Save failed - retry before exit';return false end;return true
end
function on_init()g=S.load(C.decode) or C.new();geometry();smudge.request_update()end
function on_draw()
 geometry();smudge.clear();smudge.header('CP Calculator','Decimal - 16 working digits')
 smudge.rect(w-105,65,85,30,false);smudge.text(w-98,70,'Menu','small')
 smudge.text(left,105,C.display(g),'ui12','bold')
 if g.op~='' then smudge.text(w-65,108,g.op,'small') end
 if menu then
  smudge.text(left,165,'Confirm: save and exit','ui12')
  smudge.text(left,210,'Back or tap Menu: resume','small')
 else
  for i,k in ipairs(keys)do
   local x=left+(i-1)%4*cw;local y=top+(i-1)//4*ch
   smudge.rect(x,y,cw,ch,false)
   if i==selected then smudge.rect(x+2,y+2,cw-4,ch-4,false,2) end
   smudge.text(x+cw//2-10,y+ch//2-8,k,'small')
  end
 end
 if notice~='' then smudge.text(left,h-85,notice,'small')end
 smudge.button_hints('Menu','Press','Up','Down')
end
function on_button(btn,pressed)
 if not pressed then return end
 if btn=='page_back' then btn='up' elseif btn=='page_forward' then btn='down' end
 if btn=='back' then menu=not menu
 elseif menu then if btn=='confirm' and save()then smudge.exit()end
 elseif btn=='confirm' then C.press(g,keys[selected]);notice=''
 elseif btn=='left' then selected=(selected-2)%#keys+1
 elseif btn=='right' then selected=selected%#keys+1
 elseif btn=='up' then selected=(selected-5)%#keys+1
 elseif btn=='down' then selected=(selected+3)%#keys+1 end
 smudge.request_update()
end
function on_touch(x,y,event)
 if event~='tap' or x<0 or y<0 or x>=w or y>=h then return end
 if x>=w-105 and x<w-20 and y>=65 and y<95 then on_button('back',true);return end
 if menu then if y>=150 and y<200 then on_button('confirm',true)end;return end
 if x>=left and x<left+cw*4 and y>=top and y<top+ch*6 then
  local i=(y-top)//ch*4+(x-left)//cw+1
  if i<=#keys then selected=i;on_button('confirm',true)end
 end
end
function on_swipe(dir)on_button(dir,true)end
function on_update()end
function on_exit()save()end
