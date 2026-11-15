local function valid(s)if s:match('^valid%d+$') then return s end end
local S=dofile('storage.lua');assert(not S.load(valid));assert(S.save('valid1'));assert(S.save('valid2'))
local original=smudge.write_file
smudge.write_file=function()return false end
assert(not S.save('valid3'));smudge.write_file=original
local T=dofile('storage.lua');assert(T.load(valid)=='valid2')
-- Torn latest slot must fall back to the previous valid one.
assert(smudge.write_file('state-b.dat','CP1:2:1\ntruncated'))
local T=dofile('storage.lua');assert(T.load(valid)=='valid1')
-- Well-checksummed but semantically invalid payload also rolls back.
assert(T.save('invalid'))
local T=dofile('storage.lua');assert(T.load(valid)=='valid1')
assert(not T.save(string.rep('x',4001)))
assert(not pcall(smudge.write_file,'../outside','bad'))
assert(not pcall(smudge.read_file,'/books/other.txt'))
assert(T.save('valid4'))
local T=dofile('storage.lua');assert(T.load(valid)=='valid4')
print('journal: corruption/semantic rejection/write failure/rollback OK')
