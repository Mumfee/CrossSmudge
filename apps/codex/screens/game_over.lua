-- Codex: Ink & Iron — Game Over Screen

local screen = {}

function screen.draw()
    smudge.header("Game Over", "Your Journey Ends")
    local m = smudge.get_metrics()
    local topY = m.top_padding + m.header_height + 2
    local bottomY = h - m.button_hints_height - 2
    draw_page_frame(topY, bottomY)

    smudge.centered_text(250, "THOU HAST FALLEN", "ui12", "bold", true)
    smudge.centered_text(290, "Thy verses dissolve into forgotten dust.", "small", "regular", true)
    smudge.button_hints("Back", "Retry", "", "")
end

function screen.on_button(btn, pressed)
    if not pressed then return end
    if btn == "back" or btn == "confirm" then
        delete_save_state()
        state.inRun = false
        set_screen("title")
    end
end

function screen.on_tap(x, y)
    screen.on_button("confirm", true)
end

return screen
