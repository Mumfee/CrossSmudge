assert(math.maxinteger==2147483647 and math.type(0xffffffff)=='integer')
dofile('main.lua');on_init();on_draw()
on_button('confirm',false);on_button('confirm',true);on_draw()
-- Touch edges must not index outside the board. Real signature: x,y,event.
on_touch(-1,-1,'tap');on_touch(5000,5000,'tap');on_touch(40,130,'hold')
for i=1,18 do
 on_button('right',true);on_button('confirm',true)
 for j=1,5 do on_update() end
 on_draw()
end
on_button('back',true);on_draw()
-- New game dialog is cancelled, never implicitly resets the save.
on_button('down',true);on_button('down',true);on_button('confirm',true);on_draw()
on_button('back',true);on_draw()
on_exit()
assert(smudge.read_file('state-a.dat',4097))
-- Reload in same process, just as another lifecycle; callbacks replaced.
for i=1,10 do
 on_init=nil;on_draw=nil;on_update=nil;on_exit=nil;on_touch=nil;on_button=nil;on_swipe=nil
 collectgarbage('collect')
 dofile('main.lua');on_init();on_button('confirm',true);on_draw()
 on_button('back',true);on_exit();collectgarbage('collect')
end
print('callbacks/input/draw/save/reload OK (native adapter, not HAL)')
