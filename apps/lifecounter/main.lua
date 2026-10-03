-- Life Counter application for CrossSmudge (Lua)
-- Faithful reproduction of LifeCounterActivity.h

local lifeTotal = 20
local w, h = 480, 800

function on_init()
    w, h = smudge.get_bounds()
    lifeTotal = tonumber(smudge.load("life", "20")) or 20
end

local function save_state()
    smudge.save("life", tostring(lifeTotal))
end

function on_draw()
    smudge.clear()

    -- 1. Standard Header
    smudge.header("Life Counter")

    -- 2. Geometry
    local centerX = math.floor(w / 2)
    local centerY = math.floor(h / 2) - 10
    local sideHintY = centerY - 14

    -- Side button hints (-5 on left edge, +5 on right edge)
    smudge.text(10, sideHintY, "-5", 0, false, "left")
    smudge.text(w - 36, sideHintY, "+5", 0, false, "left")

    -- 3. Render Gray Filled Heart & Black Outline using parametric formula
    smudge.heart(centerX, centerY, 165, 5, true)

    -- 4. Life Total Display centered inside Heart using Reader font
    smudge.text(centerX, centerY - 14, tostring(lifeTotal), 0, true, "center")

    -- 5. Footer Hints
    smudge.button_hints("Back", "Reset", "-1", "+1")
end

function on_button(btn, pressed)
    if not pressed then return end

    if btn == "back" then
        save_state()
        smudge.exit()
    elseif btn == "confirm" then
        lifeTotal = 20
        save_state()
    elseif btn == "up" then
        lifeTotal = lifeTotal - 5
        save_state()
    elseif btn == "down" then
        lifeTotal = lifeTotal + 5
        save_state()
    elseif btn == "left" then
        lifeTotal = lifeTotal - 1
        save_state()
    elseif btn == "right" then
        lifeTotal = lifeTotal + 1
        save_state()
    end
end

function on_tap(x, y)
    local m = smudge.get_metrics()
    if y < m.top_padding + m.header_height then return end
    if y > h - m.button_hints_height then
        -- Bottom button hints area
        if x < w / 4 then
            save_state()
            smudge.exit()
        elseif x < w / 2 then
            lifeTotal = 20
            save_state()
        elseif x < 3 * w / 4 then
            lifeTotal = lifeTotal - 1
            save_state()
        else
            lifeTotal = lifeTotal + 1
            save_state()
        end
        return
    end

    -- Touch on screen
    if x < 60 then
        lifeTotal = lifeTotal - 5
        save_state()
    elseif x > w - 60 then
        lifeTotal = lifeTotal + 5
        save_state()
    elseif x < w / 2 then
        lifeTotal = lifeTotal - 1
        save_state()
    else
        lifeTotal = lifeTotal + 1
        save_state()
    end
end
