-- Rosary application for CrossSmudge (Lua)
-- Faithful reproduction of RosaryActivity.h with connecting lines, active bead, and authentic crucifix

local current_bead = 0
local w, h = 480, 800

local beads = {}
local largeBeadRadius = 10
local smallBeadRadius = 7
local totalSteps = 0

local function build_beads()
    beads = {}
    local m = smudge.get_metrics()
    local topOffset = m.top_padding + m.header_height
    local footerHeight = m.button_hints_height
    local centerX = math.floor(w / 2)
    local availableHeight = h - topOffset - footerHeight

    local loopCenterY = topOffset + math.floor(availableHeight * 0.32)
    local radiusX = (w / 2.0) - 62.0
    local radiusY = availableHeight * 0.28
    local centerpieceY = loopCenterY + math.floor(radiusY)

    local dropGap = 22
    local dropSpacing = 18
    local startY = centerpieceY + dropGap + (dropSpacing * 3)

    -- Drop beads (0..3) - in Lua table 1..4
    table.insert(beads, {x = centerX, y = startY, r = largeBeadRadius, isOurFather = true})                       -- 0: Bottom Our Father
    table.insert(beads, {x = centerX, y = startY - dropSpacing, r = smallBeadRadius, isOurFather = false})        -- 1: Hail Mary 1
    table.insert(beads, {x = centerX, y = startY - (dropSpacing * 2), r = smallBeadRadius, isOurFather = false})  -- 2: Hail Mary 2
    table.insert(beads, {x = centerX, y = startY - (dropSpacing * 3), r = smallBeadRadius, isOurFather = false})  -- 3: Hail Mary 3

    -- 54 Loop beads (indices 4..57)
    local totalLoopBeads = 54
    for i = 0, totalLoopBeads - 1 do
        local angle = (math.pi / 2.0) - (2.0 * math.pi * i / totalLoopBeads)
        local bx = centerX + math.floor(radiusX * math.cos(angle))
        local by = loopCenterY + math.floor(radiusY * math.sin(angle))
        local isSpacer = (i == 0 or i == 11 or i == 22 or i == 33 or i == 44)
        local r = isSpacer and largeBeadRadius or smallBeadRadius
        table.insert(beads, {x = bx, y = by, r = r, isOurFather = isSpacer})
    end

    totalSteps = #beads + 1
end

function on_init()
    w, h = smudge.get_bounds()
    build_beads()
    current_bead = tonumber(smudge.load("current_bead", "0")) or 0
    if current_bead < 0 or current_bead >= totalSteps then current_bead = 0 end
end

local function draw_bead(cx, cy, radius, is_active, show_ring)
    if is_active then
        smudge.circle(cx, cy, radius, true)
        if show_ring then
            smudge.circle(cx, cy, radius + 4, false)
            smudge.circle(cx, cy, radius + 3, false)
        end
    else
        smudge.circle(cx, cy, radius, false)
        smudge.circle(cx, cy, radius - 1, false)
    end
end

function on_draw()
    smudge.clear()

    -- 1. Header (in RosaryActivity.h: simply "Rosary", no subtitle)
    smudge.header("Rosary")

    -- 2. Connecting Lines
    -- Loop beads: indices 5 to #beads (1-based in Lua, corresponding to 4..57 in 0-based)
    local loopStart = 5
    for i = loopStart, #beads do
        local nextIdx = (i == #beads) and loopStart or (i + 1)
        smudge.line(beads[i].x, beads[i].y, beads[nextIdx].x, beads[nextIdx].y)
    end

    -- Drop strand line: centerpiece (beads[5]) to bottom Our Father (beads[1])
    smudge.line(beads[5].x, beads[5].y, beads[1].x, beads[1].y)

    -- Cross String
    local crossTopY = beads[1].y + 10
    smudge.line(beads[1].x, beads[1].y + beads[1].r, beads[1].x, crossTopY + 5)

    -- 3. Render Crucifix Icon at the base (64x64 centered)
    smudge.draw_sprite(beads[1].x - 32, crossTopY + 30 - 32, 64, 64, "sprites/crucifix.raw")

    -- 4. Determine Active Bead Index
    local isCompleted = (current_bead == #beads)
    local activeBeadIdx = isCompleted and 5 or (current_bead + 1)

    -- 5. Render All Beads
    for i, b in ipairs(beads) do
        local isActive = (i == activeBeadIdx)
        local showRing = (i == 5 and isCompleted)
        draw_bead(b.x, b.y, b.r, isActive, showRing)
    end

    -- 6. Footer Hints
    smudge.button_hints("Back", "Reset", "Prev", "Next")
end

function on_button(btn, pressed)
    if not pressed then return end

    if btn == "back" then
        smudge.save("current_bead", tostring(current_bead))
        smudge.exit()
    elseif btn == "confirm" then
        current_bead = 0
    elseif btn == "right" or btn == "down" or btn == "page_forward" then
        if current_bead + 1 < totalSteps then
            current_bead = current_bead + 1
        end
    elseif btn == "left" or btn == "up" or btn == "page_back" then
        if current_bead > 0 then
            current_bead = current_bead - 1
        end
    end
end

function on_tap(x, y)
    local m = smudge.get_metrics()
    if y > h - m.button_hints_height then
        if x < w / 4 then
            smudge.save("current_bead", tostring(current_bead))
            smudge.exit()
        elseif x < w / 2 then
            current_bead = 0
        elseif x < 3 * w / 4 then
            if current_bead > 0 then current_bead = current_bead - 1 end
        else
            if current_bead + 1 < totalSteps then current_bead = current_bead + 1 end
        end
        return
    end

    -- Tap anywhere in top half moves forward, bottom half moves back
    if y < h / 2 then
        if current_bead + 1 < totalSteps then current_bead = current_bead + 1 end
    else
        if current_bead > 0 then current_bead = current_bead - 1 end
    end
end
