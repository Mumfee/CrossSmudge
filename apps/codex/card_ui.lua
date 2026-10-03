-- Codex: Ink & Iron — Compact Card & Inspector Rendering
function coup_on(c)
    if not c or not c.couplet or c.couplet == "None" then return false end
    if state.playerStatus and state.playerStatus.transcendence then return true end
    local m = state.lastPlayedMeter
    return (c.couplet == "AfterBlade" and m == "Blade") or
           (c.couplet == "AfterWard" and m == "Ward") or
           (c.couplet == "AfterScript" and m == "Script")
end

function draw_card_item(x, y, cw, ch, c, isSel)
    if not c then return end
    local coup = coup_on(c)
    smudge.rect(x, y, cw, ch, true, false)
    smudge.rounded_rect(x, y, cw, ch, 4, false, isSel and 2 or 1)
    smudge.circle(x + 13, y + 13, 9, true)
    smudge.text(x + 9, y + 4, tostring(c.cost), "small", "bold", "center", false)
    smudge.text(x + cw - 6, y + 6, c.meter, "small", "bold", "right", true)
    local divY = y + 26
    smudge.line(x + 3, divY, x + cw - 3, divY)
    local nRes = smudge.wrapped_text(c.name, cw - 6, "small", "regular", 2)
    local ty = divY + 4
    for _, l in ipairs(nRes and nRes.lines or {}) do
        smudge.text(x + math.floor(cw / 2), ty, l, "small", "bold", "center", true)
        ty = ty + smudge.line_height("small") - 1
    end
    ty = ty + 3
    local dRes = smudge.wrapped_text(c.desc, cw - 6, "small", "regular", 3)
    for _, dl in ipairs(dRes and dRes.lines or {}) do
        smudge.text(x + math.floor(cw / 2), ty, dl, "small", "regular", "center", true)
        ty = ty + smudge.line_height("small") + 2
    end
    if c.cDesc then
        local bH = 44
        local by = y + ch - bH - 4
        smudge.rect(x + 3, by, cw - 6, bH, coup)
        local t1 = coup and "COUPLET" or "Couplet:"
        smudge.text(x + math.floor(cw / 2), by + 3, t1, "small", coup and "bold" or "regular", "center", not coup)
        smudge.text(x + math.floor(cw / 2), by + 3 + smudge.line_height("small"), c.cDesc, "small", "bold", "center", not coup)
    end
end

function render_card_inspector(ix, iy, iw, ih, c, checkCoup)
    if not c then return end
    local coup = checkCoup and coup_on(c)
    smudge.rect(ix, iy, iw, ih, false)
    smudge.rect(ix + 2, iy + 2, iw - 4, ih - 4, false)
    local divY = iy + 8 + smudge.line_height("ui12") + 4
    smudge.text(ix + 10, iy + 8, "[" .. c.cost .. " Ink] " .. c.name, "ui12", "bold", "left", true)
    smudge.text(ix + iw - 12, iy + 8, "[" .. string.upper(c.meter) .. "]", "ui12", "bold", "right", true)
    smudge.line(ix + 6, divY, ix + iw - 6, divY)
    local dy = divY + 6
    local dRes = smudge.wrapped_text(c.desc, iw - 20, "small", "regular", 2)
    for _, l in ipairs(dRes and dRes.lines or {}) do
        smudge.text(ix + 10, dy, l, "small", "regular", "left", true)
        dy = dy + smudge.line_height("small") + 2
    end
    if c.cDesc then
        local bH = 26
        local by = iy + ih - bH - 6
        smudge.rect(ix + 6, by, iw - 12, bH, coup)
        local cond = (c.couplet == "AfterBlade") and "Blade" or ((c.couplet == "AfterWard") and "Ward" or "Script")
        local cBuf = (coup and "COUPLET READY (" or "Couplet (") .. cond .. "): " .. c.cDesc
        smudge.text(ix + math.floor(iw / 2), by + math.floor((bH - smudge.line_height("small")) / 2), cBuf, "small", coup and "bold" or "regular", "center", not coup)
    end
end
