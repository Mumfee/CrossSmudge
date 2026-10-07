-- Solitaire High-Contrast Suit Bitmaps and Renderers

local S14 = {
    -- 1: Spades ♠ (Solid)
    0x00c0, 0x01e0, 0x03f0, 0x07f8, 0x0ffc, 0x1ffe, 0x3fff, 0x3fff, 0x1ffe, 0x39e7, 0x20c1, 0x00c0, 0x03f0, 0x07f8,
    -- 2: Hearts ♡ (Outlined)
    0x1e1e, 0x3f3f, 0x3333, 0x3003, 0x3003, 0x1806, 0x0c0c, 0x0618, 0x0330, 0x01e0, 0x00c0, 0x00c0, 0x0000, 0x0000,
    -- 3: Clubs ♣ (Solid)
    0x01e0, 0x03f0, 0x03f0, 0x01e0, 0x3ccf, 0x3fff, 0x3fff, 0x3fff, 0x1ffe, 0x03f0, 0x00c0, 0x01e0, 0x03f0, 0x07f8,
    -- 4: Diamonds ♢ (Outlined)
    0x00c0, 0x01e0, 0x0330, 0x0618, 0x0c0c, 0x1806, 0x3003, 0x3003, 0x1806, 0x0c0c, 0x0618, 0x0330, 0x01e0, 0x00c0
}

local S22 = {
    -- 1: Spades ♠ (Solid)
    0x000c00, 0x001e00, 0x003f00, 0x007f80, 0x00ffc0, 0x01ffe0,
    0x03fff0, 0x07fff8, 0x0ffffc, 0x1ffffe, 0x3fffff, 0x3fffff,
    0x3fffff, 0x1ffffe, 0x3e3f1f, 0x381e07, 0x200c01, 0x000c00,
    0x001e00, 0x007f80, 0x01ffe0, 0x07fff8,
    -- 2: Hearts ♡ (Outlined)
    0x07c0f8, 0x0fe1fc, 0x1ff3fe, 0x383f07, 0x301e03, 0x300c03,
    0x300003, 0x300003, 0x180006, 0x180006, 0x0c000c, 0x060018,
    0x030030, 0x018060, 0x00c0c0, 0x006180, 0x003300, 0x001e00,
    0x000c00, 0x000c00, 0x000000, 0x000000,
    -- 3: Clubs ♣ (Solid)
    0x003f00, 0x007f80, 0x00ffc0, 0x00ffc0, 0x007f80, 0x003f00,
    0x3f1e3f, 0x3fdeff, 0x3fffff, 0x3fffff, 0x3fffff, 0x3fffff,
    0x1ffffe, 0x0ffffc, 0x03fff0, 0x007f80, 0x000c00, 0x001e00,
    0x003f00, 0x007f80, 0x01ffe0, 0x07fff8,
    -- 4: Diamonds ♢ (Outlined)
    0x000c00, 0x001e00, 0x003f00, 0x006180, 0x00c0c0, 0x018060,
    0x030030, 0x060018, 0x0c000c, 0x180006, 0x300003, 0x300003,
    0x180006, 0x0c000c, 0x060018, 0x030030, 0x018060, 0x00c0c0,
    0x006180, 0x003f00, 0x001e00, 0x000c00
}

function draw_suit_14(ox, oy, suit_id, fill_color)
    local offset = (suit_id - 1) * 14
    for y = 0, 13 do
        local r = S14[offset + y + 1]
        local x = 0
        while x < 14 do
            if (r >> (13 - x)) & 1 == 1 then
                local span = 1
                while x + span < 14 and ((r >> (13 - (x + span))) & 1 == 1) do
                    span = span + 1
                end
                smudge.rect(ox + x, oy + y, span, 1, true, fill_color)
                x = x + span
            else
                x = x + 1
            end
        end
    end
end

function draw_suit_22(ox, oy, suit_id, fill_color)
    local offset = (suit_id - 1) * 22
    for y = 0, 21 do
        local r = S22[offset + y + 1]
        local x = 0
        while x < 22 do
            if (r >> (21 - x)) & 1 == 1 then
                local span = 1
                while x + span < 22 and ((r >> (21 - (x + span))) & 1 == 1) do
                    span = span + 1
                end
                smudge.rect(ox + x, oy + y, span, 1, true, fill_color)
                x = x + span
            else
                x = x + 1
            end
        end
    end
end
