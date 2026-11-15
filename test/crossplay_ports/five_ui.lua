dofile('main.lua');on_init();on_draw()
for i=1,15 do on_button('down',true);on_draw() end
on_button('confirm',true);on_draw()
on_touch(40,270,'tap');on_draw();on_button('back',true);on_exit()
