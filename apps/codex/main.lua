-- Codex: Ink & Iron for CrossSmudge (Lua)
-- Modular roguelike deckbuilder with dynamic screen loading and low heap footprint

w, h = 480, 800

dofile("state.lua")
dofile("ui_base.lua")
collectgarbage("collect")

active_screen = nil
active_screen_name = nil
pending_screen = nil
pending_screen_arg = nil

function set_screen(name, arg)
    pending_screen = name
    pending_screen_arg = arg
end

function apply_screen_transition()
    if not pending_screen then return end
    local name = pending_screen
    local arg = pending_screen_arg
    pending_screen = nil
    pending_screen_arg = nil

    if active_screen_name == name and active_screen then
        collectgarbage("collect")
        if active_screen.enter then
            active_screen.enter(arg)
        end
        smudge.request_update()
        return
    end

    if active_screen and active_screen.exit then
        active_screen.exit()
    end
    active_screen = nil
    collectgarbage("collect")
    collectgarbage("collect")

    if not kCards and (name == "combat" or name == "reward" or name == "shop" or name == "deck_view") then
        dofile("cards.lua")
        collectgarbage("collect")
    end

    if name == "combat" or name == "reward" then
        if not play_card then
            dofile("combat_engine.lua")
            collectgarbage("collect")
        end
        if not draw_card_item then
            dofile("card_ui.lua")
            collectgarbage("collect")
        end
    end

    active_screen_name = name
    active_screen = dofile("screens/" .. name .. ".lua")
    collectgarbage("collect")

    if active_screen and active_screen.enter then
        active_screen.enter(arg)
    end
    collectgarbage("collect")
    smudge.request_update()
end

function on_init()
    w, h = smudge.get_bounds()
    math.randomseed(os.time())
    local loader = dofile("load_save.lua")
    if loader then loader() end
    loader = nil
    collectgarbage("collect")
    set_screen("title")
    apply_screen_transition()
end

function on_draw()
    if pending_screen then
        apply_screen_transition()
    end
    smudge.clear()
    if active_screen and active_screen.draw then
        active_screen.draw()
    end
    collectgarbage("step", 50)
end

function on_button(btn, pressed)
    if active_screen and active_screen.on_button then
        active_screen.on_button(btn, pressed)
    end
    if pending_screen then
        apply_screen_transition()
    end
    collectgarbage("step", 50)
end

function on_tap(x, y)
    local m = smudge.get_metrics()
    if y > h - m.button_hints_height then
        if x < w / 4 then
            on_button("back", true)
        elseif x < w / 2 then
            on_button("confirm", true)
        elseif x < 3 * w / 4 then
            on_button("left", true)
        else
            on_button("right", true)
        end
        return
    end

    if active_screen and active_screen.on_tap then
        active_screen.on_tap(x, y)
    end
end
