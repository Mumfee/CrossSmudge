local M=dofile('core.lua')
local S=dofile('storage.lua')
local g,cursor,notice= nil,1,''
local savedAt=0
local top,rows=200,8
local function save()
 local ok=S.save(M.encode(g));notice=ok and '' or 'SAVE FAILED';return ok
end
function on_init()g=S.load(M.decode) or M.new(smudge.millis());M.pause(g);smudge.request_update()end
function on_draw()
 local lines,options=M.view(g);local w,h=smudge.get_bounds()
 top=100+#lines*28;rows=math.max(1,(h-top-65)//36)
 smudge.clear();smudge.header(M.title,notice)
 for i,line in ipairs(lines) do smudge.text(20,80+i*28,line,'small') end
 cursor=math.min(cursor,#options);local start=((cursor-1)//rows)*rows+1
 for i=start,math.min(#options,start+rows-1) do smudge.text(20,top+(i-start)*36,(i==cursor and '> ' or '  ')..options[i],'small') end
 smudge.button_hints('Save/Exit','Select','Up','Down')
end
function on_button(b,p)
 if not p then return end
 if b=='back' then M.pause(g);if save() then smudge.exit() end
 elseif b=='confirm' then local result=M.act(g,cursor);cursor=1;if result=='exit' then M.pause(g);if save() then smudge.exit() end elseif smudge.millis()-savedAt>=5000 then save();savedAt=smudge.millis() end
 else local _,o=M.view(g);if b=='up' or b=='left' or b=='page_back' then cursor=(cursor-2)%#o+1 elseif b=='down' or b=='right' or b=='page_forward' then cursor=cursor%#o+1 end end
 smudge.request_update()
end
function on_touch(x,y,e)
 if e~='tap' or x<20 or y<top or y>=top+rows*36 then return end
 local _,o=M.view(g);local i=((cursor-1)//rows)*rows+1+(y-top)//36
 if i<=#o then o=nil;cursor=i;on_button('confirm',true) end
end
function on_exit()M.pause(g);save()end
