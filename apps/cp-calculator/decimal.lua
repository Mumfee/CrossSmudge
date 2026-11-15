-- Bounded decimal arithmetic for stock Lua's int32/float32 ABI.
-- Decimal coefficients are strings; arithmetic never converts them to floats.
local D={}
local function trim(s)s=s:gsub('^0+','');return s=='' and '0' or s end
local function add(a,b)
 local out={};local carry=0;local n=math.max(#a,#b)
 for k=0,n-1 do
  local x=k<#a and a:byte(#a-k)-48 or 0;local y=k<#b and b:byte(#b-k)-48 or 0
  local v=x+y+carry;out[n-k]=string.char(48+v%10);carry=v//10
 end
 return (carry>0 and '1' or '')..table.concat(out)
end
local function cmp(a,b)
 if #a~=#b then return #a>#b and 1 or -1 end
 return a==b and 0 or (a>b and 1 or -1)
end
local function sub(a,b)
 local out={};local borrow=0
 for k=0,#a-1 do
  local y=k<#b and b:byte(#b-k)-48 or 0
  local v=a:byte(#a-k)-48-y-borrow;borrow=v<0 and 1 or 0
  out[#a-k]=string.char(48+(v+10)%10)
 end
 return trim(table.concat(out))
end
local function split(t)
 local sign,d,e=t:match('^(-?)(%d+)@(-?%d+)$');return sign,d,tonumber(e)
end
local function rounded(d,e,precision)
 local inexact=false
 if #d>precision then
  local cut=#d-precision;local tail=d:sub(precision+1);inexact=tail:find('[1-9]')~=nil
  local up=tail:byte(1)>=53;d=d:sub(1,precision);e=e+cut
  if up then d=add(d,'1') end
  if #d>precision then d=d:sub(1,precision);e=e+1 end
 end
 return d,e,inexact
end
local function canon(sign,d,e,precision)
 d=trim(d);if d=='0' then return '0@0' end
 local inexact;d,e,inexact=rounded(d,e,precision or 16)
 while #d>1 and d:sub(-1)=='0' do d=d:sub(1,-2);e=e+1 end
 local adjusted=e+#d-1
 if adjusted>99 or e< -114 or (adjusted< -99 and inexact) then return nil,'OVERFLOW' end
 return sign..d..'@'..e
end
function D.parse(s)
 local sign,whole,frac=s:match('^(-?)(%d+)%.?(%d*)$')
 if not sign or #whole+#frac>32 then return nil,'BAD INPUT' end
 return canon(sign,whole..frac,-#frac)
end
function D.valid(t)
 if type(t)~='string' or #t>24 then return false end
 local sign,d,e=split(t)
 return e and #d<=16 and e>=-114 and e<=99 and canon(sign,d,e)==t or false
end
function D.neg(t)if t=='0@0' then return t end;return t:sub(1,1)=='-' and t:sub(2) or '-'..t end
function D.op(a,b,op)
 local sa,da,ea=split(a);local sb,db,eb=split(b)
 if op=='-' then sb=sb=='' and '-' or '';op='+' end
 if op=='+' then
  local e=math.min(ea,eb);da=da..string.rep('0',ea-e);db=db..string.rep('0',eb-e)
  da=trim(da);db=trim(db)
  if sa==sb then return canon(sa,add(da,db),e) end
  local c=cmp(da,db);if c>=0 then return canon(sa,sub(da,db),e) end
  return canon(sb,sub(db,da),e)
 elseif op=='*' then
  if da=='0' or db=='0' then return '0@0' end
  local out={};for i=1,#da+#db do out[i]=0 end
  for i=#da,1,-1 do for j=#db,1,-1 do out[i+j]=out[i+j]+(da:byte(i)-48)*(db:byte(j)-48) end end
  for i=#out,2,-1 do out[i-1]=out[i-1]+out[i]//10;out[i]=out[i]%10 end
  for i=1,#out do out[i]=string.char(48+out[i]) end
  return canon(sa==sb and '' or '-',table.concat(out),ea+eb)
 elseif op=='/' then
  if db=='0' then return nil,da=='0' and 'BAD INPUT' or 'DIVIDE BY 0' end
  if da=='0' then return '0@0' end
  local k=18+#db-#da;local numerator=da..string.rep('0',k);local q={};local rem='0'
  for i=1,#numerator do
   rem=trim(rem..numerator:sub(i,i));local digit=0
   while cmp(rem,db)>=0 do rem=sub(rem,db);digit=digit+1 end
   q[i]=string.char(48+digit)
  end
  return canon(sa==sb and '' or '-',trim(table.concat(q)),ea-eb-k)
 end
 return nil,'BAD INPUT'
end
function D.format(t)
 local sign,d,e=split(t);d,e=rounded(d,e,10)
 -- The original display context has precision 10 and Emin -99 (Etiny -108).
 if e< -108 then
  local cut=-108-e
  if cut>=#d then
   d=(cut==#d and d:byte(1)>=53) and '1' or '0';e=-108
  else d,e=rounded(d,e,#d-cut) end
 end
 if d=='0' then return '0' end
 while #d>1 and d:sub(-1)=='0' do d=d:sub(1,-2);e=e+1 end
 local fixed
 if e>=0 and #d+e<=16 then fixed=d..string.rep('0',e)
 elseif e<0 and #d+e>0 then fixed=d:sub(1,#d+e)..'.'..d:sub(#d+e+1)
 elseif e<0 and -e+2<=16 then fixed='0.'..string.rep('0',-e-#d)..d end
 if fixed and #sign+#fixed<=16 then return sign..fixed end
 local exponent=e+#d-1
 return sign..d:sub(1,1)..(#d>1 and '.'..d:sub(2) or '')..'e'..(exponent>=0 and '+' or '')..exponent
end
return D
