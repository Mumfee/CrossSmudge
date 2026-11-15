-- Observe actual UI callback text arguments; drawing remains the native adapter.
assert(smudge.write_file('note1.md',string.rep('long ',50)))
assert(smudge.write_file('note2.md','short card'))
dofile('main.lua');on_init();on_button('confirm',true);on_draw()
for i=1,5 do on_button('right',true)end
on_button('back',true);on_button('back',true) -- card menu then saved library
on_button('down',true);on_button('confirm',true) -- short Note2
local line='';local real=smudge.text
smudge.text=function(x,y,text,...)
 if y==95 then line=text end
 return real(x,y,text,...)
end
on_draw();assert(line:find('short card',1,true),'new note inherited prior horizontal scroll')
print('new card resets horizontal scrolling')
