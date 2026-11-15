local M=dofile('core.lua');local S=dofile('storage.lua');local g=M.new(99)
if M.title=='Forehead' then
 for c=1,17 do g.category=c;M.begin(g);g.phase=1;for i=1,128 do M.answer(g,1) end end
elseif M.title=='Insider' then
 for i=1,560 do M.deal(g);g.phase=5;M.vote(g,g.roles:find('I') or 0) end
elseif M.title=='Wavelength' then
 for i=1,M.count do M.deal(g);M.act(g,1);M.record(g,1) end
end
assert(S.save(M.encode(g)));g.rng=g.rng~1;assert(S.save(M.encode(g)))
local encoded=M.encode(g);assert(M.decode(encoded))
S=nil;M=nil;g=nil;encoded=nil;collectgarbage('collect')
dofile('main.lua');on_init();on_draw();on_exit()
