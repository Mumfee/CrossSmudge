dofile('main.lua');on_init();on_draw()
for i=1,80 do on_button('down',true);on_draw();on_button('confirm',true);on_update();on_draw() end
on_touch(-1,-1,'tap');on_touch(50,250,'release');on_exit()
on_init=nil;on_draw=nil;on_update=nil;on_exit=nil;on_touch=nil;on_button=nil
collectgarbage('collect')
dofile('main.lua');on_init();on_draw();on_button('back',true);on_exit()
