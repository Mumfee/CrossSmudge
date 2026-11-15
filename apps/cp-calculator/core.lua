-- Immediate execution, contextual percent and repeated equals from CalcEngine.h.
local D=dofile('decimal.lua');local C={}
function C.new()return {acc='0@0',operand='',entry='0',live=false,negative=false,op='',repeatOp='',repeatValue='0@0',err=''}end
local function current(g)
 if g.live then return D.parse((g.negative and '-' or '')..g.entry) end
 return g.operand~='' and g.operand or g.acc
end
local function apply(g,a,b,op)
 local value,err=D.op(a,b,op);if not value then g.err=err end;return value
end
local function reset(g)
 local fresh=C.new();for k,v in pairs(fresh) do g[k]=v end
end
function C.display(g)
 if g.err~='' then return g.err end
 if g.live then return (g.negative and g.entry:find('[1-9]') and '-' or '')..g.entry end
 return D.format(current(g))
end
function C.press(g,k)
 if g.err~='' and k~='AC' and k~='CE' then return end
 if k=='AC' or (k=='CE' and g.err~='') then reset(g);return end
 if k=='CE' then g.entry='0';g.live=true;g.negative=false;g.operand='';return end
 if k:match('^%d$') or k=='.' then
  g.operand=''
  if not g.live then g.entry='0';g.live=true;g.negative=false end
  if k=='.' then if not g.entry:find('.',1,true) then g.entry=g.entry..'.' end;return end
  local digits=g.entry:gsub('%.','')
  local count=(g.entry:sub(1,2)=='0.') and #digits-1 or (g.entry=='0' and 0 or #digits)
  if count>=10 then return end
  if g.entry=='0' then g.entry=k else g.entry=g.entry..k end
 elseif k=='BS' then
  if g.live then g.entry=g.entry:sub(1,-2);if g.entry=='' then g.entry='0';g.negative=false end end
 elseif k=='+/-' then
  if g.live then g.negative=not g.negative elseif g.operand~='' then g.operand=D.neg(g.operand) else g.acc=D.neg(g.acc) end
 elseif k=='%' then
  local value=apply(g,current(g),'1@2','/');if not value then return end
  if g.op=='+' or g.op=='-' then value=apply(g,g.acc,value,'*');if not value then return end end
  g.operand=value;g.live=false;g.entry='0';g.negative=false
 elseif k=='+' or k=='-' or k=='*' or k=='/' then
  if not g.live and g.op~='' then g.op=k;return end
  local rhs=current(g)
  local value=g.op=='' and rhs or apply(g,g.acc,rhs,g.op);if not value then return end
  g.acc=value;g.op=k;g.live=false;g.operand='';g.repeatOp=''
 elseif k=='=' then
  local rhs,op
  if g.op~='' then rhs=current(g);op=g.op
  elseif g.repeatOp~='' then
   if g.live or g.operand~='' then g.acc=current(g) end
   rhs=g.repeatValue;op=g.repeatOp
  else g.acc=current(g);g.live=false;g.operand='';return end
  local result=apply(g,g.acc,rhs,op);if not result then return end
  g.acc=result;g.repeatValue=rhs;g.repeatOp=op;g.op='';g.live=false;g.operand=''
 end
end
function C.encode(g)
 return table.concat({g.acc,g.operand,g.entry,g.live and '1' or '0',g.negative and '1' or '0',g.op,g.repeatOp,g.repeatValue,g.err},'|')
end
function C.decode(s)
 if type(s)~='string' or #s>180 then return nil end
 local f={};for v in (s..'|'):gmatch('(.-)|')do f[#f+1]=v end
 if #f~=9 or not D.valid(f[1]) or (f[2]~='' and not D.valid(f[2])) or not D.valid(f[8]) then return nil end
 if not f[3]:match('^%d+%.?%d*$') or #f[3]>12 or not f[4]:match('^[01]$') or not f[5]:match('^[01]$') then return nil end
 if not f[6]:match('^[+*/-]?$') or not f[7]:match('^[+*/-]?$') then return nil end
 if f[9]~='' and f[9]~='DIVIDE BY 0' and f[9]~='BAD INPUT' and f[9]~='OVERFLOW' then return nil end
 return {acc=f[1],operand=f[2],entry=f[3],live=f[4]=='1',negative=f[5]=='1',op=f[6],repeatOp=f[7],repeatValue=f[8],err=f[9]}
end
return C
