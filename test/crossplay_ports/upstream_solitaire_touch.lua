-- Regression demonstration: the shipped callback rejects stock argument order.
dofile('main.lua');on_init();new_game()
local before=#stock
on_touch(40,140,'tap')
assert(#stock==before,'unexpected: shipped callback now accepts stock touch order')
on_touch('tap',40,140)
assert(#stock<before,'control: tapping stock should draw cards with expected app order')
print('CONFIRMED: unchanged Solitaire ignores stock on_touch(x,y,event); alternate app order draws stock')
