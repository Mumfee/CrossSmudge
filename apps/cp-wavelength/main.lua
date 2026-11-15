-- Button/tap list adapter; scrolling keeps every action reachable in both orientations.
local M=dofile('core.lua');local S=dofile('storage.lua')
local g,cursor,last,notice= nil,1,0,''
local lastSave=0
local w,h,top,rows=480,800,240,8
local function geometry(lines)
 w,h=smudge.get_bounds();top=100+#lines*28;rows=math.max(1,(h-top-65)//36)
end
local function save()local ok=S.save(M.encode(g));notice=ok and '' or 'SAVE FAILED';return ok end
local function tick()
 local phase=g.phase
 local now=smudge.millis();local delta=now-last;last=now
 if delta>=0 and delta<3600000 and M.tick then M.tick(g,delta) end
 if M.awake then smudge.prevent_sleep(M.awake(g)) end
 return phase~=g.phase
end
function on_init()
 g=S.load(M.decode) or M.new(smudge.millis());M.pause(g);last=smudge.millis();smudge.request_update()
end
function on_draw()
 local lines,options=M.view(g);geometry(lines);smudge.clear();smudge.header(M.title,notice)
 for i,line in ipairs(lines) do smudge.text(24,80+i*28,line,'small') end
 local start=((cursor-1)//rows)*rows+1
 for i=start,math.min(#options,start+rows-1) do smudge.text(24,top+(i-start)*36,(i==cursor and '> ' or '  ')..options[i],'small') end
 smudge.button_hints('Pause/Exit','Select','Up','Down')
end
function on_button(b,p)
 if not p then return end;if tick() then cursor=1;smudge.request_update();return end;local _,o=M.view(g)
 if b=='back' then M.pause(g);if save() then smudge.exit() end
 elseif b=='up' or b=='left' or b=='page_back' then cursor=(cursor-2)%#o+1
 elseif b=='down' or b=='right' or b=='page_forward' then cursor=cursor%#o+1
 elseif b=='confirm' then
  local result=M.act(g,cursor);cursor=1
  if result=='exit' then M.pause(g);if save() then smudge.exit() end elseif smudge.millis()-lastSave>=5000 then save();lastSave=smudge.millis() end
 end
 smudge.request_update()
end
function on_touch(x,y,e)
 if e~='tap' then return end
 local lines,o=M.view(g);geometry(lines)
 local start=((cursor-1)//rows)*rows+1;local i=start+(y-top)//36
 if x>=20 and x<w-20 and y>=top and y<top+rows*36 and i<=#o then cursor=i;on_button('confirm',true) end
end
function on_update()
 local before=g.left;local phase=g.phase;tick()
 if phase~=g.phase then cursor=1;save();smudge.request_update()
 elseif before and before//5000~=g.left//5000 then smudge.request_update() end
end
function on_exit()tick();M.pause(g);if M.awake then smudge.prevent_sleep(false) end;save()end
