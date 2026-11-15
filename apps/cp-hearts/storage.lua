-- Fixed app-relative journal slots. No user-provided paths; never touches other apps.
local S={seq=0,slot=0,last=nil}
local paths={'state-a.dat','state-b.dat'}
local function checksum(s)
 local h=7;for i=1,#s do h=(h*31+s:byte(i))%65521 end;return h
end
local function parse(raw,validate)
 if type(raw)~='string' or #raw>4096 then return nil end
 local n,h,p=raw:match('^CP1:(%d+):(%d+)\n(.*)$')
 n=tonumber(n);h=tonumber(h)
 if not n or math.type(n)~='integer' or n<1 or n>1000000 or not p or checksum(p)~=h then return nil end
 local value=validate(p);if not value then return nil end
 return value,n,p
end
function S.load(validate)
 local best
 for i=1,2 do
  local value,n,p=parse(smudge.read_file(paths[i],4097),validate)
  if value and n>S.seq then best=value;S.seq=n;S.slot=i;S.last=p end
 end
 return best
end
function S.save(payload)
 if payload==S.last then return true end
 if #payload>4000 or S.seq>=1000000 then smudge.log('CrossPlay save limit reached');return false end
 local n=S.seq+1;local slot=S.slot==1 and 2 or 1
 local raw='CP1:'..n..':'..checksum(payload)..'\n'..payload
 if not smudge.write_file(paths[slot],raw) then smudge.log('CrossPlay save write failed');return false end
 if smudge.read_file(paths[slot],4097)~=raw then smudge.log('CrossPlay save readback failed');return false end
 S.seq=n;S.slot=slot;S.last=payload;return true
end
return S
