-- Solitaire Game Engine (compact bit-packed cards)
local function make_card(s, r, u) return (s << 5) | (r << 1) | (u and 1 or 0) end
local function card_suit(c) return c >> 5 end
local function card_rank(c) return (c >> 1) & 0x0F end
local function card_up(c) return (c & 1) == 1 end
local function card_set_up(c, u) return (c & ~1) | (u and 1 or 0) end
local function is_red_suit(s) return (s == 2 or s == 4) end

stock = {}
waste = {}
foundations = {{}, {}, {}, {}}
tableau = {{}, {}, {}, {}, {}, {}, {}}

moves_count = 0
games_won = 0
is_won = false
banner_msg = ""
banner_expires = 0

show_reset_confirm = false
back_held_start = nil
back_suppress_release = false

sel_zone = 0
sel_col = 0
sel_card = 0

cur_zone = 2
cur_col = 1
cur_card = 1

undo_history = {}

function show_banner(msg, ms)
    banner_msg = msg
    local now = (smudge.millis and smudge.millis()) or (os.time() * 1000)
    banner_expires = now + (ms or 2000)
    if smudge.request_update then smudge.request_update() end
end

local function split_str(str, sep)
    local result = {}
    local from = 1
    local delim_from, delim_to = string.find(str, sep, from, true)
    while delim_from do
        table.insert(result, string.sub(str, from, delim_from - 1))
        from = delim_to + 1
        delim_from, delim_to = string.find(str, sep, from, true)
    end
    table.insert(result, string.sub(str, from))
    return result
end

function serialize_game()
    local parts = {
        "2", tostring(moves_count or 0), is_won and "1" or "0",
        tostring(cur_zone or 2), tostring(cur_col or 1), tostring(cur_card or 1),
        table.concat(stock, ","), table.concat(waste, ","),
        table.concat(foundations[1], ","), table.concat(foundations[2], ","),
        table.concat(foundations[3], ","), table.concat(foundations[4], ","),
        table.concat(tableau[1], ","), table.concat(tableau[2], ","),
        table.concat(tableau[3], ","), table.concat(tableau[4], ","),
        table.concat(tableau[5], ","), table.concat(tableau[6], ","),
        table.concat(tableau[7], ",")
    }
    return table.concat(parts, ";")
end

function save_game()
    if not smudge.save then return end
    smudge.save("games_won", tostring(games_won or 0))
    if is_won then smudge.save("game_state", ""); return end
    smudge.save("game_state", serialize_game())
end

local function deserialize_pile(s)
    local pile = {}
    if not s or s == "" then return pile end
    for val in string.gmatch(s, "[^,]+") do
        local n = tonumber(val)
        if not n then return nil end
        table.insert(pile, n)
    end
    return pile
end

function restore_game_state(data)
    if not data or data == "" then return false end
    local parts = split_str(data, ";")
    if #parts < 19 or tonumber(parts[1]) ~= 2 then return false end
    local moves = tonumber(parts[2]) or 0
    if tonumber(parts[3]) == 1 then return false end

    local new_stock = deserialize_pile(parts[7])
    local new_waste = deserialize_pile(parts[8])
    if not new_stock or not new_waste then return false end

    local new_f = {}
    for f = 1, 4 do
        local p = deserialize_pile(parts[8 + f])
        if not p then return false end
        new_f[f] = p
    end

    local new_t = {}
    for t = 1, 7 do
        local p = deserialize_pile(parts[12 + t])
        if not p then return false end
        new_t[t] = p
    end

    local total = #new_stock + #new_waste
    for f = 1, 4 do total = total + #new_f[f] end
    for t = 1, 7 do total = total + #new_t[t] end
    if total ~= 52 then return false end

    moves_count = moves
    is_won = false
    sel_zone = 0
    cur_zone = tonumber(parts[4]) or 2
    cur_col = tonumber(parts[5]) or 1
    cur_card = tonumber(parts[6]) or 1

    stock = new_stock
    waste = new_waste
    foundations = new_f
    tableau = new_t
    return true
end

function load_game()
    if not smudge.load then return false end
    local data = smudge.load("game_state")
    if restore_game_state(data) then
        undo_history = {}
        show_banner("Game Resumed", 1500)
        return true
    end
    return false
end

function push_undo()
    local s = serialize_game()
    table.insert(undo_history, s)
    if #undo_history > 3 then table.remove(undo_history, 1) end
end

function do_undo()
    if #undo_history == 0 then
        show_banner("Nothing to undo", 1500)
        return
    end
    local s = table.remove(undo_history)
    restore_game_state(s)
    show_banner("Move Undone", 1500)
    collectgarbage("step")
    if smudge.request_update then smudge.request_update() end
end

function new_game()
    math.randomseed((smudge.millis and smudge.millis()) or os.time())
    moves_count = 0
    sel_zone = 0
    is_won = false
    undo_history = {}
    banner_msg = ""

    local deck = {}
    for s = 1, 4 do
        for r = 1, 13 do
            table.insert(deck, make_card(s, r, false))
        end
    end
    for i = #deck, 2, -1 do
        local j = math.random(1, i)
        deck[i], deck[j] = deck[j], deck[i]
    end

    stock = {}
    waste = {}
    for f = 1, 4 do foundations[f] = {} end
    for t = 1, 7 do tableau[t] = {} end

    local d = 1
    for col = 1, 7 do
        for r = 1, col do
            local c = deck[d]
            d = d + 1
            if r == col then c = card_set_up(c, true) end
            table.insert(tableau[col], c)
        end
    end
    while d <= #deck do
        table.insert(stock, deck[d])
        d = d + 1
    end

    show_banner("New Game Dealt", 1500)
    save_game()
    if smudge.request_update then smudge.request_update() end
end

local function can_place_on_tableau(c, col)
    local pile = tableau[col]
    local r = card_rank(c)
    local s = card_suit(c)
    if #pile == 0 then return (r == 13) end
    local top = pile[#pile]
    if not card_up(top) then return false end
    return (card_rank(top) == r + 1) and (is_red_suit(card_suit(top)) ~= is_red_suit(s))
end

local function can_place_on_foundation(c, f_idx)
    local pile = foundations[f_idx]
    local r = card_rank(c)
    local s = card_suit(c)
    if #pile == 0 then return (r == 1 and s == f_idx) end
    local top = pile[#pile]
    return (s == card_suit(top) and r == card_rank(top) + 1)
end

function check_win()
    local total = 0
    for f = 1, 4 do total = total + #foundations[f] end
    if total == 52 and not is_won then
        is_won = true
        games_won = games_won + 1
        save_game()
        show_banner("VICTORY! All 52 cards completed!", 5000)
        if smudge.full_refresh then smudge.full_refresh() end
    end
end

function try_auto_foundation(c, src_zone, src_col)
    for f = 1, 4 do
        if can_place_on_foundation(c, f) then
            push_undo()
            if src_zone == 1 then
                table.remove(waste)
            elseif src_zone == 2 then
                local pile = tableau[src_col]
                table.remove(pile)
                if #pile > 0 and not card_up(pile[#pile]) then
                    pile[#pile] = card_set_up(pile[#pile], true)
                end
            end
            table.insert(foundations[f], c)
            moves_count = moves_count + 1
            check_win()
            save_game()
            if smudge.request_update then smudge.request_update() end
            return true
        end
    end
    return false
end

function flip_stock()
    push_undo()
    if #stock > 0 then
        local c = table.remove(stock)
        table.insert(waste, card_set_up(c, true))
        moves_count = moves_count + 1
    else
        if #waste == 0 then return end
        while #waste > 0 do
            local c = table.remove(waste)
            table.insert(stock, card_set_up(c, false))
        end
        moves_count = moves_count + 1
    end
    sel_zone = 0
    save_game()
    collectgarbage("step")
    if smudge.request_update then smudge.request_update() end
end

function auto_complete_step()
    if #waste > 0 then
        local c = waste[#waste]
        if try_auto_foundation(c, 1, 2) then return true end
    end
    for col = 1, 7 do
        local pile = tableau[col]
        if #pile > 0 and card_up(pile[#pile]) then
            local c = pile[#pile]
            if try_auto_foundation(c, 2, col) then return true end
        end
    end
    return false
end

function execute_move(target_zone, target_col)
    if sel_zone == 0 then return false end

    if sel_zone == 1 then
        if #waste == 0 then sel_zone = 0; return false end
        local c = waste[#waste]
        if target_zone == 2 then
            if can_place_on_tableau(c, target_col) then
                push_undo()
                table.remove(waste)
                table.insert(tableau[target_col], c)
                moves_count = moves_count + 1
                sel_zone = 0
                save_game()
                if smudge.request_update then smudge.request_update() end
                return true
            end
        elseif target_zone == 1 and target_col >= 4 then
            local f = target_col - 3
            if can_place_on_foundation(c, f) then
                push_undo()
                table.remove(waste)
                table.insert(foundations[f], c)
                moves_count = moves_count + 1
                sel_zone = 0
                check_win()
                save_game()
                if smudge.request_update then smudge.request_update() end
                return true
            end
        end
    elseif sel_zone == 2 then
        local src_pile = tableau[sel_col]
        local start_idx = sel_card
        if not src_pile or start_idx < 1 or start_idx > #src_pile then
            sel_zone = 0; return false
        end
        local moving_card = src_pile[start_idx]

        if target_zone == 2 and target_col ~= sel_col then
            if can_place_on_tableau(moving_card, target_col) then
                push_undo()
                local moving_stack = {}
                for i = start_idx, #src_pile do table.insert(moving_stack, src_pile[i]) end
                for i = #src_pile, start_idx, -1 do table.remove(src_pile) end
                if #src_pile > 0 and not card_up(src_pile[#src_pile]) then
                    src_pile[#src_pile] = card_set_up(src_pile[#src_pile], true)
                end
                for _, card in ipairs(moving_stack) do table.insert(tableau[target_col], card) end
                moves_count = moves_count + 1
                sel_zone = 0
                save_game()
                collectgarbage("step")
                if smudge.request_update then smudge.request_update() end
                return true
            end
        elseif target_zone == 1 and target_col >= 4 and start_idx == #src_pile then
            local f = target_col - 3
            if can_place_on_foundation(moving_card, f) then
                push_undo()
                table.remove(src_pile)
                if #src_pile > 0 and not card_up(src_pile[#src_pile]) then
                    src_pile[#src_pile] = card_set_up(src_pile[#src_pile], true)
                end
                table.insert(foundations[f], moving_card)
                moves_count = moves_count + 1
                sel_zone = 0
                check_win()
                save_game()
                collectgarbage("step")
                if smudge.request_update then smudge.request_update() end
                return true
            end
        end
    end

    sel_zone = 0
    if smudge.request_update then smudge.request_update() end
    return false
end

-- Export card accessors
card_s = card_suit
card_r = card_rank
card_u = card_up
