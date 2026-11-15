local N=dofile('core.lua')
local s='Title\r\n  - [ ] Milk  \r\n+ [X] Eggs\n- [] prose\n- [x]tra\n'
local lines=N.parse(s);assert(#lines==6);assert(not lines[1].mark);assert(lines[2].mark and not lines[2].checked)
assert(lines[3].checked and not lines[4].mark and not lines[5].mark)
local t=N.toggle(s,2);local diff=0
for i=1,#s do if s:byte(i)~=t:byte(i) then diff=diff+1 end end
assert(diff==1 and #s==#t);assert(N.toggle(t,2)==s)
assert(N.clear(s)=='Title\r\n  - [ ] Milk  \r\n- [] prose\n- [x]tra\n')
assert(N.strip('  * [X] hi\r\n')=='hi\r\n')
assert(N.tolist('Milk\n\nEggs')=='- [ ] Milk\n\n- [ ] Eggs')
assert(N.tolist('- [x] Milk')=='- [x] Milk')
assert(N.validate(s)==s);assert(not N.validate(string.rep('x',3073)));assert(not N.validate('bad\0text'))
print('notes byte-preservation/parser/clear/convert bounds OK')
