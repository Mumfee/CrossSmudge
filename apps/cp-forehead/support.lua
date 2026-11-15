-- Bounded data and state helpers. No executable save data.
local U={}
function U.random(g,n)
 local x=g.rng;if x==0 then x=0x9e3779b9 end
 local excess=((0x7fffffff%n+1)%n*2)%n
 repeat x=x~(x<<13);x=x~(x>>17);x=x~(x<<5) until excess==0 or x>=0 or x < -excess
 g.rng=x
 return ((x&0x7fffffff)%n+(x<0 and ((0x7fffffff%n+1)%n) or 0))%n+1
end
function U.mark(s,i,v)return s:sub(1,i-1)..v..s:sub(i+1) end
function U.seen(g,i)
 local p=(i-1)//4+1;local bit=1<<((i-1)%4)
 return (tonumber(g.seen:sub(p,p),16)&bit)~=0
end
function U.setseen(g,i,on)
 local p=(i-1)//4+1;local bit=1<<((i-1)%4);local v=tonumber(g.seen:sub(p,p),16)
 g.seen=U.mark(g.seen,p,string.format('%x',on and (v|bit) or (v&~bit)))
end
function U.draw(g,first,count,keep)
 local left=0
 for i=first,first+count-1 do if not U.seen(g,i) then left=left+1 end end
 if left==0 then
  for i=first,first+count-1 do U.setseen(g,i,false) end
  if keep then for i=1,#keep,5 do U.setseen(g,tonumber(keep:sub(i,i+3),16),true) end end
  for i=first,first+count-1 do if not U.seen(g,i) then left=left+1 end end
 end
 if left==0 then return 0 end
 local n=U.random(g,left)
 for i=first,first+count-1 do if not U.seen(g,i) then
  n=n-1;if n==0 then U.setseen(g,i,true);return i end
 end end
end
local block,content
function U.word(i)
 local b=(i-1)//32
 if block~=b then content=smudge.read_file('data/'..b..'.txt',2048);block=b end
 if not content then return '[Missing deck file]' end
 local n=0
 for line in content:gmatch('([^\n]+)') do n=n+1;if n==(i-1)%32+1 then return line end end
 return '[Invalid deck entry]'
end
function U.codec(M,fields)
 function M.encode(g)
  local out={}
  for _,f in ipairs(fields) do local v=g[f[1]];out[#out+1]=tostring(v) end
  return table.concat(out,'|')
 end
 function M.decode(s)
  if type(s)~='string' or #s>3900 then return nil end
  local g={};local i=0
  for v in (s..'|'):gmatch('(.-)|') do
   i=i+1;local f=fields[i];if not f then return nil end
   if type(f[2])=='number' then v=tonumber(v);if not v or math.type(v)~='integer' or v<f[2] or v>f[3] then return nil end
   elseif #v>f[3] or not v:match(f[2]) then return nil end
   g[f[1]]=v
  end
  if i~=#fields or (M.valid and not M.valid(g)) then return nil end
  return g
 end
end
return U
