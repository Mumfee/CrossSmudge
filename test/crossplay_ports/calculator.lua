local C=dofile('core.lua')
local function run(keys,expected)
 local g=C.new()
 for k in keys:gmatch('%S+')do C.press(g,k)end
 assert(C.display(g)==expected,keys..': '..C.display(g)..' expected '..expected)
end
run('0 . 1 + 0 . 2 - 0 . 3 =','0')
run('2 + 3 = = =','11')
run('2 + 3 = 7 =','10')
run('2 + * 3 =','6')
run('2 0 0 + 1 0 % =','220')
run('2 0 0 * 1 0 % =','20')
run('5 + 3 CE =','5')
run('1 / 3 = * 3 =','1')
run('5 / 0 = 9','DIVIDE BY 0')
run('5 / 0 = CE 3 =','3')
run('0 / 0 =','BAD INPUT')
run('1 2 . 3 BS BS 4','124')
run('9 9 9 9 9 9 9 9 9 9 8','9999999999')
run('5 +/- + 2 =','-3')
run('0 . 5 0','0.50')
local D=dofile('decimal.lua')
for a=-30,30 do for b=-10,10 do
 for _,op in ipairs({'+','-','*'}) do
  local token=assert(D.op(D.parse(tostring(a)),D.parse(tostring(b)),op))
  local expected=op=='+' and a+b or (op=='-' and a-b or a*b)
  assert(D.format(token)==tostring(expected))
 end
end end
assert(not D.op(D.parse('1'),D.parse('0'),'/'))
local g=C.new();C.press(g,'2');C.press(g,'+');C.press(g,'3')
local copy=assert(C.decode(C.encode(g)));C.press(copy,'=');assert(C.display(copy)=='5')
assert(not C.decode('garbage'))
print('decimal arithmetic/calculator key sequences OK')
