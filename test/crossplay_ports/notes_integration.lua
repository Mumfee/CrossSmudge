assert(smudge.write_file('note1.md','Groceries\n- [ ] Milk\n- [X] Eggs\n'))
dofile('main.lua');on_init();on_draw();on_button('confirm',true);on_draw()
on_button('down',true);on_button('confirm',true);on_draw()
on_touch(-1,-1,'tap');on_touch(9999,9999,'tap');on_button('back',true);on_draw()
on_exit();assert(smudge.read_file('note-1-a.dat',4097))
assert(smudge.read_file('note1.md',3073)=='Groceries\n- [ ] Milk\n- [X] Eggs\n')
for i=1,5 do
 on_init=nil;on_draw=nil;on_button=nil;on_touch=nil;on_update=nil;on_swipe=nil;on_exit=nil;collectgarbage('collect')
 dofile('main.lua');on_init();on_button('confirm',true);on_draw();on_exit()
end
print('notes UI/tick/app-local persistence/source preservation OK')
