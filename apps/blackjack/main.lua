-- Blackjack game for CrossSmudge (Lua)
-- Faithful reproduction of BlackjackActivity.h with Insurance, Double Down, and Split

local ranks = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"}
local chip_values = {1, 5, 10, 25, 50, 100}

local suit_files = {
    [1] = "sprites/clubs.raw",
    [2] = "sprites/diamonds.raw",
    [3] = "sprites/hearts.raw",
    [4] = "sprites/spades.raw"
}

local bankroll = 50
local current_bet = 5
local chip_idx = 2 -- $5 default
local phase = "betting" -- "betting", "insurance", "player_turn", "round_over"

local dealer_hand = {}
local deck = {}
local deck_top = 0
local status_message = ""
local w, h = 480, 800

local kCardWidth = 84
local kCardHeight = 120
local kCardCornerRadius = 8
local kCardGap = 10

-- Hands management for Split support
-- Each hand: { cards = {c1, c2, ...}, bet = X, busted = false, result = "" }
local player_hands = {}
local active_hand_idx = 1
local review_hand_idx = 1
local insurance_bet = 0

local function card_rank(c)
    return ((c - 1) % 13) + 1
end

local function card_suit(c)
    return math.floor((c - 1) / 13) + 1
end

local function card_value(c)
    local r = card_rank(c)
    if r > 10 then return 10 end
    if r == 1 then return 11 end
    return r
end

local function init_deck()
    local idx = 1
    for d = 1, 4 do -- 4 deck shoe (208 cards, integer encoded)
        for s = 1, 4 do
            for r = 1, 13 do
                deck[idx] = (s - 1) * 13 + r
                idx = idx + 1
            end
        end
    end
    deck_top = idx - 1
    -- Shuffle in-place
    for i = deck_top, 2, -1 do
        local j = math.random(1, i)
        deck[i], deck[j] = deck[j], deck[i]
    end
end

local function draw_card()
    if deck_top <= 0 then
        -- Reshuffle shoe in-place
        for i = #deck, 2, -1 do
            local j = math.random(1, i)
            deck[i], deck[j] = deck[j], deck[i]
        end
        deck_top = #deck
    end
    local c = deck[deck_top]
    deck_top = deck_top - 1
    return c
end

local function hand_score(hand_cards)
    local score = 0
    local aces = 0
    for _, c in ipairs(hand_cards) do
        score = score + card_value(c)
        if card_rank(c) == 1 then aces = aces + 1 end
    end
    while score > 21 and aces > 0 do
        score = score - 10
        aces = aces - 1
    end
    return score
end

local function clear_table(t)
    for i = #t, 1, -1 do
        t[i] = nil
    end
end

local function resolve_round_end()
    phase = "round_over"
    review_hand_idx = 1

    local dScore = hand_score(dealer_hand)
    local dealerBust = (dScore > 21)
    local dealerBJ = (#dealer_hand == 2 and dScore == 21)

    local totalPayout = 0

    -- Insurance payout (2:1, pays 3x insurance bet)
    if insurance_bet > 0 and dealerBJ then
        totalPayout = totalPayout + (insurance_bet * 3)
    end

    for i = 1, #player_hands do
        local h = player_hands[i]
        local pScore = hand_score(h.cards)
        local pBJ = (#h.cards == 2 and pScore == 21 and #player_hands == 1)

        if h.busted then
            h.payout = 0
            h.result = "BUST (-$" .. h.bet .. ")"
        elseif pBJ then
            if dealerBJ then
                h.payout = h.bet
                h.result = "PUSH (Tie)"
            else
                local winAmount = math.floor(h.bet * 1.5)
                h.payout = h.bet + winAmount
                h.result = "BLACKJACK! (+$" .. winAmount .. ")"
            end
        elseif dealerBJ then
            h.payout = 0
            h.result = "DEALER BLACKJACK (-$" .. h.bet .. ")"
        elseif dealerBust or pScore > dScore then
            h.payout = h.bet * 2
            h.result = "WIN! (+$" .. h.bet .. ")"
        elseif pScore == dScore then
            h.payout = h.bet
            h.result = "PUSH (Tie)"
        else
            h.payout = 0
            h.result = "HOUSE WINS (-$" .. h.bet .. ")"
        end
        totalPayout = totalPayout + (h.payout or 0)
    end

    bankroll = bankroll + totalPayout
    if #player_hands == 1 then
        status_message = player_hands[1].result
    else
        status_message = "Round finished. Reviewing hands..."
    end
end

local function dealer_play()
    local allBusted = true
    for _, h in ipairs(player_hands) do
        if not h.busted then allBusted = false break end
    end

    if not allBusted then
        while hand_score(dealer_hand) < 17 do
            table.insert(dealer_hand, draw_card())
        end
    end

    resolve_round_end()
end

local function advance_player_turn()
    active_hand_idx = active_hand_idx + 1
    if active_hand_idx > #player_hands then
        dealer_play()
    end
end

local function start_player_turn()
    phase = "player_turn"
    status_message = ""

    local pScore = hand_score(player_hands[1].cards)
    local dScore = hand_score(dealer_hand)
    if pScore == 21 or dScore == 21 then
        resolve_round_end()
    end
end

local function deal_round()
    if bankroll < current_bet then return end
    bankroll = bankroll - current_bet
    insurance_bet = 0
    clear_table(dealer_hand)
    clear_table(player_hands)

    player_hands[1] = {
        cards = {draw_card(), draw_card()},
        bet = current_bet,
        busted = false,
        result = ""
    }
    dealer_hand[1] = draw_card()
    dealer_hand[2] = draw_card()

    active_hand_idx = 1
    review_hand_idx = 1
    status_message = ""

    -- If dealer's upcard is an Ace (card 1), offer insurance!
    if card_rank(dealer_hand[1]) == 1 then
        phase = "insurance"
        status_message = "Dealer shows Ace. Buy Insurance?"
    else
        start_player_turn()
    end
end

local function resolve_insurance(buy)
    if buy then
        local insCost = math.floor(current_bet / 2)
        if insCost <= bankroll then
            insurance_bet = insCost
            bankroll = bankroll - insCost
        end
    else
        insurance_bet = 0
    end

    start_player_turn()
end

local function can_double_down()
    if phase ~= "player_turn" then return false end
    local h = player_hands[active_hand_idx]
    return (h and #h.cards == 2 and bankroll >= h.bet)
end

local function can_split_hand()
    if phase ~= "player_turn" then return false end
    local h = player_hands[active_hand_idx]
    if not h or #h.cards ~= 2 then return false end
    if bankroll < h.bet or #player_hands >= 4 then return false end
    return (card_rank(h.cards[1]) == card_rank(h.cards[2]))
end

local function perform_double_down()
    if not can_double_down() then return end
    local h = player_hands[active_hand_idx]
    bankroll = bankroll - h.bet
    h.bet = h.bet * 2
    table.insert(h.cards, draw_card())
    if hand_score(h.cards) > 21 then
        h.busted = true
    end
    advance_player_turn()
end

local function perform_split()
    if not can_split_hand() then return end
    local h = player_hands[active_hand_idx]
    bankroll = bankroll - h.bet

    local secondCard = table.remove(h.cards)
    local newHand = {
        cards = {secondCard, draw_card()},
        bet = h.bet,
        busted = false,
        result = ""
    }
    table.insert(h.cards, draw_card())
    table.insert(player_hands, active_hand_idx + 1, newHand)
end

function on_init()
    w, h = smudge.get_bounds()
    math.randomseed(os.time())
    bankroll = tonumber(smudge.load("bankroll", "50")) or 50
    current_bet = math.min(5, bankroll)
    if current_bet <= 0 then current_bet = 1 end
    init_deck()
    collectgarbage()
end

local function save_state()
    smudge.save("bankroll", tostring(bankroll))
end

local function draw_empty_card_slot(x, y)
    smudge.rounded_rect(x, y, kCardWidth, kCardHeight, kCardCornerRadius, true, false)
    smudge.rounded_rect(x, y, kCardWidth, kCardHeight, kCardCornerRadius, false, 1, true)
end

local function draw_card_shape(x, y, card, is_face_down)
    smudge.rounded_rect(x, y, kCardWidth, kCardHeight, kCardCornerRadius, true, false)
    smudge.rounded_rect(x, y, kCardWidth, kCardHeight, kCardCornerRadius, false, 1, true)

    if is_face_down then
        smudge.rect_dither(x + 5, y + 5, kCardWidth - 10, kCardHeight - 10, false)
        return
    end

    local r = card_rank(card)
    local s = card_suit(card)
    local rankStr = ranks[r]
    smudge.text(x + 8, y + 8, rankStr, "ui12", true, "left")

    local iconX = x + math.floor((kCardWidth - 32) / 2)
    local iconY = y + math.floor(kCardHeight / 2) - 4

    local sprite = suit_files[s]
    if sprite then
        local is_red = (s == 2 or s == 3)
        smudge.draw_sprite(iconX, iconY, 32, 32, sprite, false, is_red)
    end
end

local function draw_hand_row(cards, cardY, hasHiddenCard)
    local maxHandW = w - 40
    local numCards = #cards
    if numCards == 0 then
        local totalW = 2 * kCardWidth + kCardGap
        local startX = math.floor((w - totalW) / 2)
        draw_empty_card_slot(startX, cardY)
        draw_empty_card_slot(startX + kCardWidth + kCardGap, cardY)
        return
    end

    local totalW = numCards * kCardWidth + (numCards - 1) * kCardGap
    if totalW <= maxHandW or numCards == 1 then
        local startX = math.floor((w - totalW) / 2)
        for i, card in ipairs(cards) do
            local cardX = startX + (i - 1) * (kCardWidth + kCardGap)
            local isFaceDown = (i == 2 and hasHiddenCard)
            draw_card_shape(cardX, cardY, card, isFaceDown)
        end
    else
        local step = math.floor((maxHandW - kCardWidth) / (numCards - 1))
        local startX = math.floor((w - (kCardWidth + (numCards - 1) * step)) / 2)
        for i, card in ipairs(cards) do
            local cardX = startX + (i - 1) * step
            local isFaceDown = (i == 2 and hasHiddenCard)
            draw_card_shape(cardX, cardY, card, isFaceDown)
        end
    end
end

function on_draw()
    smudge.clear()

    local m = smudge.get_metrics()
    local topBound = m.top_padding + m.header_height
    local bottomBound = h - m.button_hints_height
    local availableHeight = bottomBound - topBound

    -- 1. Header
    smudge.header("Blackjack", "Bank: $" .. bankroll)

    -- 2. DEALER SECTION
    local dealerScore = hand_score(dealer_hand)
    local hasHiddenCard = (#dealer_hand >= 2 and (phase == "player_turn" or phase == "insurance"))

    local dealerHeader = "Dealer"
    if #dealer_hand > 0 then
        if hasHiddenCard then
            dealerHeader = "Dealer (?)"
        elseif dealerScore > 21 then
            dealerHeader = "Dealer (" .. dealerScore .. ") [BUST]"
        else
            dealerHeader = "Dealer (" .. dealerScore .. ")"
        end
    end

    local dealerTextY = topBound + math.floor(availableHeight * 0.04)
    smudge.centered_text(dealerTextY, dealerHeader, "ui12", "bold", true)

    local dealerCardY = dealerTextY + 36
    draw_hand_row(dealer_hand, dealerCardY, hasHiddenCard)

    -- 3. PLAYER SECTION
    local activeH = player_hands[active_hand_idx]
    if phase == "round_over" then
        activeH = player_hands[review_hand_idx] or player_hands[1]
    end

    local playerHeader = "Player"
    if activeH and #activeH.cards > 0 then
        local pScore = hand_score(activeH.cards)
        local bustTag = activeH.busted and " [BUST]" or ""
        if #player_hands > 1 then
            local hNum = (phase == "round_over") and review_hand_idx or active_hand_idx
            playerHeader = "Hand " .. hNum .. " of " .. #player_hands .. " ($" .. activeH.bet .. ") - (" .. pScore .. ")" .. bustTag
        else
            playerHeader = "Player ($" .. activeH.bet .. ") - (" .. pScore .. ")" .. bustTag
        end
    end

    local playerTextY = topBound + math.floor(availableHeight * 0.38)
    smudge.centered_text(playerTextY, playerHeader, "ui12", "bold", true)

    local playerCardY = playerTextY + 36
    if activeH then
        draw_hand_row(activeH.cards, playerCardY, false)
    else
        draw_hand_row({}, playerCardY, false)
    end

    -- Split Indicator / Button (if pairs are present and can split)
    local canSplit = can_split_hand()
    if canSplit then
        local splitBoxW = 280
        local splitBoxH = 36
        local splitX = math.floor((w - splitBoxW) / 2)
        local splitY = playerCardY + kCardHeight + 16
        smudge.rounded_rect(splitX, splitY, splitBoxW, splitBoxH, 6, true, true)
        smudge.centered_text(splitY + 10, "SPLIT (Side Button / Tap)", "ui10", "bold", false)
    end

    -- 4. STATUS & BET READOUT
    local statusY = bottomBound - 50
    if phase == "betting" then
        smudge.centered_text(statusY, "Bet: $" .. current_bet .. "   (Chip: $" .. chip_values[chip_idx] .. ")", "ui10", "regular", true)
    elseif phase == "round_over" then
        local res = (activeH and activeH.result ~= "") and activeH.result or status_message
        smudge.centered_text(statusY, res, "ui12", "bold", true)
    elseif #status_message > 0 then
        smudge.centered_text(statusY, status_message, "ui10", "regular", true)
    end

    -- 5. Insurance Modal Dialog (when dealer shows Ace)
    if phase == "insurance" then
        local modalW = w - 40
        local modalH = 130
        local modalX = math.floor((w - modalW) / 2)
        local modalY = math.floor((h - modalH) / 2) - 10
        local insCost = math.floor(current_bet / 2)

        smudge.rect(modalX, modalY, modalW, modalH, true, true)
        smudge.rect(modalX + 2, modalY + 2, modalW - 4, modalH - 4, false, 1, false)

        smudge.centered_text(modalY + 16, "DEALER SHOWS ACE", "ui12", "bold", false)
        smudge.centered_text(modalY + 44, "Buy Insurance for $" .. insCost .. "?", "ui10", "regular", false)
        smudge.centered_text(modalY + 68, "Pays 2:1 if dealer has Blackjack", "small", "regular", false)
        smudge.centered_text(modalY + 96, "[ Insure ]           [ Decline ]", "ui10", "bold", false)
    end

    -- 6. FOOTER HINTS
    if phase == "betting" then
        smudge.button_hints("Back", "Deal", "- Bet", "+ Bet")
    elseif phase == "insurance" then
        smudge.button_hints("Decline", "Insure", "Insure", "Decline")
    elseif phase == "player_turn" then
        local btn2 = can_double_down() and "Double" or ""
        smudge.button_hints("Back", btn2, "Hit", "Stand")
    else
        local nextLabel = (#player_hands > 1 and review_hand_idx < #player_hands) and "Next Hand" or "Next"
        smudge.button_hints("Back", nextLabel, "", "")
    end
end

function on_button(btn, pressed)
    if not pressed then return end

    if phase == "insurance" then
        if btn == "confirm" or btn == "left" or btn == "page_back" or btn == "up" then
            resolve_insurance(true)
        elseif btn == "back" or btn == "right" or btn == "page_forward" or btn == "down" then
            resolve_insurance(false)
        end
        return
    end

    if btn == "back" then
        save_state()
        smudge.exit()
    elseif phase == "betting" then
        if btn == "confirm" then
            deal_round()
        elseif btn == "left" or btn == "page_back" or btn == "up" then
            current_bet = math.max(1, current_bet - chip_values[chip_idx])
        elseif btn == "right" or btn == "page_forward" or btn == "down" then
            current_bet = math.min(bankroll, current_bet + chip_values[chip_idx])
        end
    elseif phase == "player_turn" then
        -- Check Split on either top (up/page_back) or bottom (down/page_forward) button
        if (btn == "up" or btn == "down" or btn == "page_back" or btn == "page_forward") and can_split_hand() then
            perform_split()
            return
        end

        if btn == "confirm" then
            -- Confirm button triggers Double Down
            if can_double_down() then
                perform_double_down()
                save_state()
            end
        elseif btn == "left" then
            -- Hit
            local h = player_hands[active_hand_idx]
            table.insert(h.cards, draw_card())
            local score = hand_score(h.cards)
            if score >= 21 then
                if score > 21 then h.busted = true end
                advance_player_turn()
                save_state()
            end
        elseif btn == "right" then
            -- Stand
            advance_player_turn()
            save_state()
        end
    elseif phase == "round_over" then
        if btn == "confirm" then
            if #player_hands > 1 and review_hand_idx < #player_hands then
                review_hand_idx = review_hand_idx + 1
                return
            end

            if bankroll <= 0 then
                bankroll = 50
            end
            current_bet = math.min(current_bet, bankroll)
            if current_bet <= 0 then current_bet = 1 end
            clear_table(dealer_hand)
            clear_table(player_hands)
            insurance_bet = 0
            status_message = ""
            phase = "betting"
        end
    end
end

function on_tap(x, y)
    local m = smudge.get_metrics()

    -- 1. Tap on Insurance Modal
    if phase == "insurance" then
        if y > h - m.button_hints_height then
            if x < w / 2 then
                resolve_insurance(x >= w / 4) -- btn1 Decline, btn2 Insure
            else
                resolve_insurance(x < 3 * w / 4) -- btn3 Insure, btn4 Decline
            end
            return
        end
        local modalW = w - 40
        local modalH = 130
        local modalX = math.floor((w - modalW) / 2)
        local modalY = math.floor((h - modalH) / 2) - 10
        if x >= modalX and x <= modalX + modalW and y >= modalY and y <= modalY + modalH then
            if x < modalX + math.floor(modalW / 2) then
                resolve_insurance(true)
            else
                resolve_insurance(false)
            end
            return
        end
    end

    -- 2. Tap on Split Box
    if phase == "player_turn" and can_split_hand() then
        local topBound = m.top_padding + m.header_height
        local bottomBound = h - m.button_hints_height
        local availableHeight = bottomBound - topBound
        local playerTextY = topBound + math.floor(availableHeight * 0.38)
        local playerCardY = playerTextY + 36
        local splitBoxW = 280
        local splitBoxH = 36
        local splitX = math.floor((w - splitBoxW) / 2)
        local splitY = playerCardY + kCardHeight + 16
        if x >= splitX and x <= splitX + splitBoxW and y >= splitY and y <= splitY + splitBoxH then
            perform_split()
            return
        end
    end

    -- 3. Bottom Button Hints Tap
    if y > h - m.button_hints_height then
        if x < w / 4 then
            save_state()
            smudge.exit()
        elseif phase == "betting" then
            if x < w / 2 then
                deal_round()
            elseif x < 3 * w / 4 then
                current_bet = math.max(1, current_bet - chip_values[chip_idx])
            else
                current_bet = math.min(bankroll, current_bet + chip_values[chip_idx])
            end
        elseif phase == "player_turn" then
            if x >= w / 4 and x < w / 2 then
                -- Double Down
                if can_double_down() then
                    perform_double_down()
                    save_state()
                end
            elseif x >= w / 2 and x < 3 * w / 4 then
                -- Hit
                local h = player_hands[active_hand_idx]
                table.insert(h.cards, draw_card())
                local score = hand_score(h.cards)
                if score >= 21 then
                    if score > 21 then h.busted = true end
                    advance_player_turn()
                    save_state()
                end
            elseif x >= 3 * w / 4 then
                -- Stand
                advance_player_turn()
                save_state()
            end
        elseif phase == "round_over" then
            if x >= w / 4 and x < w / 2 then
                if #player_hands > 1 and review_hand_idx < #player_hands then
                    review_hand_idx = review_hand_idx + 1
                    return
                end
                if bankroll <= 0 then
                    bankroll = 50
                end
                current_bet = math.min(current_bet, bankroll)
                if current_bet <= 0 then current_bet = 1 end
                clear_table(dealer_hand)
                clear_table(player_hands)
                insurance_bet = 0
                status_message = ""
                phase = "betting"
            end
        end
    end
end
