dofile('main.lua');on_init();on_draw();on_button('confirm',true)
on_button('confirm',true);on_draw() -- Roll
on_button('down',true);on_button('confirm',true);on_draw() -- Score proposal
on_button('back',true);on_draw() -- Cancel proposal
on_button('confirm',true);on_button('down',true);on_button('confirm',true) -- Score
for i=1,100 do on_update()end;on_draw()
on_touch(-1,-1,'tap');on_touch(9999,9999,'tap')
on_button('back',true);on_exit();assert(smudge.read_file('state-a.dat',4097))
print('Yahtzee UI roll/score cancel/AI updates/save OK')
