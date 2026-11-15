-- Real main/input/engine/suit code; only host APIs are mocked. No game replacement.
assert(math.maxinteger == 2147483647)
local saved, texts, buttons = {}, {}, {}
local clock, exited = 123456, false
local function noop() end
smudge = {
    get_bounds = function() return test_width, test_height end,
    millis = function() return clock end,
    save = function(k, v) saved[k] = v end,
    load = function(k, fallback) return saved[k] or fallback end,
    request_update = noop, full_refresh = noop,
    is_button_down = function(k) return buttons[k] or false end,
    exit = function() exited = true end,
    clear = function() texts = {} end,
    rounded_rect = noop, rect = noop, rect_dither = noop, circle = noop, line = noop,
    header = noop, button_hints = noop,
    text = function(x, y, s) texts[s] = {x, y} end,
}
if test_device ~= 'missing' then
    smudge.get_device = function() return test_device end
end
dofile('main.lua')
on_init()
on_draw()
local margin = math.floor((w - 452) / 2)
local function touch(x, y, event)
    if test_device == 'sticky' then on_touch(x, y, event)
    else on_touch(event, x, y) end
end
local function tap(x, y) touch(x, y, 'tap') end
local function x(col) return margin + (col - 1) * 66 + 28 end
local function card(s, r, up) return (s << 5) | (r << 1) | (up == false and 0 or 1) end
local function fixture()
    stock, waste = {}, {}
    foundations = {{}, {}, {}, {}}
    tableau = {{}, {}, {}, {}, {}, {}, {}}
    sel_zone, moves_count, is_won, show_reset_confirm = 0, 0, false, false
end
local function dialog()
    tap(w - 30, 25)
    assert(show_reset_confirm, 'header tap must open reset confirmation')
    on_draw()
    assert(texts.Cancel and texts.Reset, 'draw actual dialog buttons')
end
local function click_label(label)
    local p = assert(texts[label])
    tap(p[1], p[2] + 8)
end
-- CASE stock
    local n = #stock
    touch(x(1), 150, 'down')
    assert(#stock == n, 'non-tap event must not deal')
    tap(x(1), 150)
    assert(#stock == n - 1 and #waste == 1 and moves_count == 1, 'direct stock tap must deal')
-- CASE card
    fixture()
    tableau[1], tableau[2] = {card(1, 7)}, {card(2, 8)}
    tap(x(1), 250)
    assert(sel_zone == 2 and sel_col == 1 and sel_card == 1, 'face-up card tap must select')
    tap(x(2), 250)
    assert(#tableau[1] == 0 and #tableau[2] == 2 and moves_count == 1, 'direct target tap must move')
-- CASE run
    fixture()
    tableau[1] = {card(3, 9, false), card(1, 7), card(2, 6)}
    tableau[2] = {card(2, 8)}
    tap(x(1), 228)
    assert(sel_zone == 0, 'face-down card must not select')
    tap(x(1), 240)
    assert(sel_zone == 2 and sel_card == 2, 'exposed run card tap must select run')
    tap(x(2), 250)
    assert(#tableau[1] == 1 and card_u(tableau[1][1]) and #tableau[2] == 3 and moves_count == 1)
-- CASE waste
    fixture()
    waste, tableau[1] = {card(1, 7)}, {card(2, 8)}
    tap(x(2), 150)
    assert(sel_zone == 1, 'waste tap must select')
    tap(x(1), 250)
    assert(#waste == 0 and #tableau[1] == 2 and moves_count == 1)
    waste = {card(1, 1)}
    tap(x(2), 150)
    assert(#waste == 0 and #foundations[1] == 1, 'ace tap must auto-foundation')
-- CASE empty
    fixture()
    tableau[1] = {card(1, 13)}
    tap(x(1), 250)
    tap(x(2), 250)
    assert(#tableau[1] == 0 and #tableau[2] == 1 and moves_count == 1, 'king must move to empty target')
-- CASE illegal
    fixture()
    tableau[1], tableau[2] = {card(1, 7)}, {card(1, 8)}
    local before = serialize_game()
    tap(x(1), 250)
    assert(sel_zone == 2)
    tap(x(2), 250)
    assert(serialize_game() == before, 'same-color target must not mutate game')
    tap(x(1), 250)
    tap(x(3), 250)
    assert(serialize_game() == before, 'non-king must not move to empty target')
-- CASE bounds
    fixture()
    tableau[1] = {card(1, 13)}
    tap(margin + 60, 250)
    assert(sel_zone == 0, 'column gap must not select a card')
    tap(x(1), 301)
    assert(sel_zone == 0, 'space below card must not select')
    tap(margin + 55, 298)
    assert(sel_zone == 2, 'inside bottom-right card edge must select')
    tap(x(2), 301)
    assert(#tableau[1] == 1 and #tableau[2] == 0, 'below empty slot must not move')
    tap(x(2), 298)
    assert(#tableau[1] == 0 and #tableau[2] == 1)
-- CASE cancel
    local before = serialize_game()
    dialog()
    click_label('Cancel')
    assert(not show_reset_confirm and serialize_game() == before, 'Cancel must preserve deal')
-- CASE reset reset_edge
    flip_stock()
    assert(moves_count == 1)
    dialog()
    clock = clock + 17
    if test_case == 'reset_edge' then
        -- Inside the actual drawn Reset button's bottom-right corner.
        local dw = math.min(w - 40, 420)
        local dx, dy = math.floor((w - dw) / 2), math.floor((h - 156) / 2)
        tap(dx + dw - 14, dy + 130)
    else
        click_label('Reset')
    end
    assert(not show_reset_confirm and moves_count == 0 and #stock == 24 and #waste == 0,
           'Reset must deal fresh, including drawn button edges')
-- CASE buttons
    cur_zone, cur_col = 1, 1
    on_button('confirm', true)
    assert(#stock == 23 and #waste == 1)
    on_button('page_forward', true)
    assert(cur_col == 2)
    buttons.back = true
    on_update()
    clock = clock + 501
    on_update()
    assert(show_reset_confirm, 'hold Back must still open dialog')
    buttons.back = false
    on_button('back', true) -- suppress release from long press
    assert(show_reset_confirm)
    on_button('back', true)
    assert(not show_reset_confirm and moves_count == 1)
    on_button('back', true)
    assert(exited)
-- CASE save
    tap(x(1), 150)
    assert(moves_count == 1)
    on_exit()
    local before = serialize_game()
    assert(saved.game_state == before, 'on_exit must persist the actual game')
    dofile('main.lua') -- fresh app globals, retaining only mocked persistent storage
    on_init()
    assert(serialize_game() == before, 'schema 2 save must resume unchanged')

-- CASE dispatch
    local before = serialize_game()
    if test_device == 'sticky' then
        on_touch('tap', x(1), 150)
    else
        on_touch(x(1), 150, 'tap')
    end
    assert(serialize_game() == before, 'other callback order must remain ignored')
    tap(x(1), 150)
    assert(moves_count == 1 and #waste == 1)
    if test_device ~= 'sticky' then
        fixture()
        tableau[1] = {card(1, 13)}
        tap(margin + 60, 250)
        assert(sel_zone == 2, 'legacy gap hit behavior must remain unchanged')
        dialog()
        local dw = math.min(w - 40, 420)
        local dx, dy = math.floor((w - dw) / 2), math.floor((h - 156) / 2)
        tap(dx + dw - 14, dy + 130)
        assert(#tableau[1] == 1 and not show_reset_confirm, 'legacy reset edge dismisses unchanged')
    end
