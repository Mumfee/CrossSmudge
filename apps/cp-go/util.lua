-- Numeric tree codec: bounded, non-executable saves; game validator checks semantics.
local U={}
function U.mod(x,n)return ((x&0x7fffffff)%n+(x<0 and ((0x7fffffff%n+1)%n) or 0))%n end
function U.next(x)x=x~(x<<13);x=x~(x>>17);return x~(x<<5) end
function U.shuffle(t,seed,stream)
 local x=seed~(0x9e3779b9*(stream+1));if x==0 then x=0x1234567 end
 for i=#t,2,-1 do x=U.next(x);local j=U.mod(x,i)+1;t[i],t[j]=t[j],t[i] end
 return t
end
function U.zeros(n)local t={};for i=1,n do t[i]=0 end;return t end
function U.sum(t)local n=0;for _,v in ipairs(t) do n=n+v end;return n end
function U.copy(t)local v={};for k,x in pairs(t) do v[k]=type(x)=='table' and U.copy(x) or x end;return v end
function U.codec(M,keys)
 function M.encode(g)
  local o=''
  local function emit(v)
   if type(v)=='table' then o=o..'[';for _,x in ipairs(v) do emit(x) end;o=o..']'
   elseif type(v)=='string' then o=o..'#'..#v..':'..v else o=o..tostring(v)..',' end
  end
  for _,k in ipairs(keys) do emit(g[k]) end
  return o
 end
 function M.decode(s)
  if type(s)~='string' or #s>3900 then return nil end
  local at,n=1,0
  local function read(depth)
   n=n+1;if depth>4 or n>1300 then error('size') end
   if s:sub(at,at)=='#' then local a,b,len=s:find('#(%d+):',at);if a~=at then error('string') end;len=tonumber(len);if len>1000 or b+len>#s then error('length') end;at=b+len+1;return s:sub(b+1,b+len) end
   if s:sub(at,at)=='[' then
    at=at+1;local t={};while s:sub(at,at)~=']' do t[#t+1]=read(depth+1) end;at=at+1;return t
   end
   local a,b,v=s:find('(-?%d+),',at);if a~=at then error('number') end;at=b+1
   v=tonumber(v);if math.type(v)~='integer' then error('integer') end;return v
  end
  local ok,g=pcall(function()local t={};local arrays=' history captured ';for _,k in ipairs(keys) do local v=read(0);local want=arrays:find(' '..k..' ',1,true) and 'table' or ((k=='b' or k=='dead') and 'string' or 'number');if type(v)~=want then return nil end;t[k]=v end;if at~=#s+1 or not M.valid(t) then return nil end;return t end)
  if ok then return g end
 end
end
function U.array(t,n,lo,hi)
 if type(t)~='table' or #t~=n then return false end
 for _,v in ipairs(t) do if type(v)~='number' or v<lo or v>hi then return false end end;return true
end
return U
