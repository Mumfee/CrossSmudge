dofile('main.lua');on_init();on_draw()
for i=1,100 do
 on_button('right',true);on_button('confirm',true);on_draw()
end
on_touch(-1,-1,'tap');on_touch(9999,9999,'tap')
on_button('back',true);on_draw();on_exit()
assert(smudge.read_file('state-a.dat',4097))
for i=1,5 do
 on_init=nil;on_draw=nil;on_button=nil;on_touch=nil;on_update=nil;on_swipe=nil;on_exit=nil;collectgarbage('collect')
 dofile('main.lua');on_init();on_draw();on_exit()
end
print('calculator keypad/menu/save/reload OK')
