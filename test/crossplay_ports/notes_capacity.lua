local doc=string.rep('- [ ] '..string.rep('a',25)..'\n',96)
assert(#doc==3072);assert(smudge.write_file('note1.md',doc))
dofile('main.lua');on_init();on_button('confirm',true);on_draw()
for i=1,96 do on_button('confirm',true);on_button('down',true)end
on_draw();on_exit()
assert(smudge.read_file('note-1-a.dat',4097))
print('Notes maximum-size import/toggle/serialize OK')
-- Keep both full-size journal slots, then restart callbacks and read them back.
on_button('up',true);on_button('confirm',true);on_exit()
on_init=nil;on_draw=nil;on_update=nil;on_exit=nil;on_touch=nil;on_button=nil;on_swipe=nil;doc=nil
collectgarbage('collect');dofile('main.lua');on_init();on_button('confirm',true);on_draw()
on_button('back',true)
for i=1,3 do on_button('down',true)end
on_button('confirm',true);on_draw() -- Convert full card to page.
on_button('back',true);on_button('down',true);on_button('down',true)
on_button('confirm',true);on_draw();on_exit() -- Convert back to full checklist.
print('Notes full-size two-slot resume/conversions OK')
