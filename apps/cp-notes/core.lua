-- NotesCore.cpp rules translated from CrossPlay. Offsets are Lua 1-based.
local N={}
-- Seven array slots per row avoid seven hash entries per row on constrained Lua.
-- One shared metatable keeps the readable field interface, without per-row closures.
local slots={first=1,last=2,finish=3,text=4,mark=5,checked=6,offset=7}
local row_mt={__index=function(t,k)return rawget(t,slots[k])end,
 __newindex=function(t,k,v)rawset(t,slots[k],v)end}
function N.validate(s)
 if type(s)~='string' or #s>3072 or s:find('%z') then return nil end
 local n=0;for _ in s:gmatch('\n') do n=n+1 end
 if n>96 then return nil end
 return s
end
function N.pack(line)
 return string.pack('<I2I2I2I2I2B',line.first,line.last,line.finish,line.text,line.mark or 0,line.checked and 1 or 0)
end
function N.row(raw)
 if type(raw)=='table' then return raw end
 local first,last,finish,text,mark,checked=string.unpack('<I2I2I2I2I2B',raw)
 if mark==0 then mark=nil end
 return setmetatable({first,last,finish,text,mark,checked==1,0},row_mt)
end
function N.parse(s,compact)
 local lines={};local start=1
 while start<=#s+1 do
  local nl=s:find('\n',start,true);local stop=(nl or (#s+1))-1
  local last=stop;if s:sub(last,last)=='\r' then last=last-1 end
  local raw=s:sub(start,last);local indent,mark,rest=raw:match('^([ \t]*)[-*+] %[(.)%](.*)$')
  local line=setmetatable({start,last,nl or #s,start,nil,false,0},row_mt)
  if mark and (mark==' ' or mark=='x' or mark=='X') and (rest=='' or rest:sub(1,1)==' ') then
   line.mark=start+#indent+3;line.checked=mark~=' ';line.text=line.mark+2+(rest~='' and 1 or 0)
  end
  lines[#lines+1]=compact and N.pack(line) or line
  if not nl then break end;start=nl+1
 end
 return lines
end
function N.toggle(s,index,parsed)
 local raw=(parsed or N.parse(s,true))[index];if not raw then return s end
 local line=N.row(raw);if not line.mark then return s end
 return s:sub(1,line.mark-1)..(line.checked and ' ' or 'x')..s:sub(line.mark+1)
end
-- Bounded cold-path conversions build one <=3072-byte string rather than
-- retaining one newly allocated string for every distinct line simultaneously.
function N.clear(s,parsed)
 local out='';for _,raw in ipairs(parsed or N.parse(s,true)) do
  local line=N.row(raw)
  if not line.checked then out=out..s:sub(line.first,line.finish) end
 end;return out
end
function N.strip(s,parsed)
 local out='';for _,raw in ipairs(parsed or N.parse(s,true)) do
  local l=N.row(raw)
  out=out..s:sub(l.mark and l.text or l.first,l.finish)
 end;return out
end
function N.tolist(s,parsed)
 local out='';for _,raw in ipairs(parsed or N.parse(s,true)) do
  local l=N.row(raw);local prefix=(l.last>=l.first and not l.mark) and '- [ ] ' or ''
  if #out+#prefix+l.finish-l.first+1>3072 then return s end
  out=out..prefix..s:sub(l.first,l.finish)
 end
 return N.validate(out) and out or s
end
-- Avoid cutting multibyte UTF-8 at either display boundary; bytes remain untouched.
function N.slice(s,first,last)
 while first>1 and (s:byte(first) or 0)>=128 and (s:byte(first) or 0)<192 do first=first-1 end
 local next=last+1
 while next>first and (s:byte(next) or 0)>=128 and (s:byte(next) or 0)<192 do next=next-1 end
 return s:sub(first,next-1)
end
return N
