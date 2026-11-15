-- Worst duo-choice fanout in a conservation-valid late-round position.
-- Setup in a separate VM/process when measuring only fresh lifecycle overhead.
local M=dofile('core.lua');local S=dofile('storage.lua');local g=M.new(123)
for c=1,58 do g.place[c]=2;g.seq[c]=0 end
for c=1,24 do g.place[c]=1 end
for c=51,53 do g.place[c]=1 end
-- Three mermaids with human, one with opponent: no premature four-mermaid win.
g.at=59;g.step=3
assert(M.valid(g));assert(S.save(M.encode(g)))
M=nil;S=nil;g=nil;collectgarbage('collect')
dofile('main.lua');on_init();on_draw();on_button('confirm',true);on_draw();on_exit()
print('late-round Sea Salt UI/journal lifecycle passed')
