local now=123456;local awake=false;local texts={}
smudge.millis=function()return now end
smudge.prevent_sleep=function(v)awake=v end
smudge.text=function(x,y,t)texts[#texts+1]=t end
local M=dofile('core.lua');local S=dofile('storage.lua');local g=M.new(123)
if M.title=='Insider' then M.deal(g);g.phase=2 else M.begin(g);g.phase=1 end
assert(S.save(M.encode(g)));local secret=M.word(g.word)
M=nil;S=nil;g=nil;collectgarbage('collect')
dofile('main.lua');on_init();on_draw()
for _,t in ipairs(texts) do assert(not t:find(secret,1,true)) end
assert(not awake)
on_button('confirm',true) -- Insider reveal, or Forehead resume
on_update()
if secret and awake then now=now+120001;on_update();assert(not awake) end
on_exit();assert(not awake)
print('private resume, timer expiry and sleep flag checked')
