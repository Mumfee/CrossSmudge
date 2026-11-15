-- Neutral API adapters: no input normalization and no rendering claims.
local files, trace = {}, {}
local now, orientation = 1000, "portrait"
smudge = {}
for _, name in ipairs({"clear", "rect", "rounded_rect", "circle", "line", "thick_line",
    "rect_dither", "text", "header", "button_hints", "full_refresh", "invert_screen",
    "exit", "prevent_sleep", "request_update"}) do
    smudge[name] = function(...)
        local args = table.pack(...)
        local parts = {name}
        for i = 1, args.n do parts[#parts+1] = tostring(args[i]) end
        trace[#trace+1] = table.concat(parts, "|")
    end
end
smudge.get_bounds = function() if orientation == "landscape" then return 800,480 end return 480,800 end
smudge.set_orientation = function(value) orientation = value end
smudge.get_date = function() return {year=2026, month=10, day=6, hour=14, min=45, sec=0, wday=3} end
smudge.get_battery = function() return 80 end
smudge.millis = function() return now end
smudge.has_touch = function() return true end
smudge.is_button_down = function() return false end
smudge.read_file = function(path) return files[path] end
smudge.write_file = function(path, data) files[path] = data end
if DEVICE ~= "missing" then smudge.get_device = function() return DEVICE end end
local function load_app()
    assert(loadfile(SOURCE))()
    on_init()
end
load_app()
-- Read-only inspection of closure state; never inject or mutate app locals.
local function state(name)
    local seen = {}
    local function search(fn)
        if seen[fn] then return end
        seen[fn] = true
        for i=1,100 do
            local key,value = debug.getupvalue(fn,i)
            if not key then break end
            if key == name then return true,value end
            if type(value)=="function" then
                local found,result = search(value)
                if found then return true,result end
            end
        end
    end
    for _,fn in ipairs({on_touch,on_button,on_draw,on_update}) do
        local found,value = search(fn)
        if found then return value end
    end
    error("unreachable state "..name)
end
local function eq(name, expected)
    assert(state(name)==expected, name..": expected "..tostring(expected)..", got "..tostring(state(name)))
end
local function touch(x,y,event)
    event = event or "tap"
    if DEVICE == "sticky" or MODE == "ignored" then on_touch(x,y,event)
    else on_touch(event,x,y) end
end
local function draw() on_draw() end
local function advance(ms) now=now+ms; on_update() end

eq("today_drank",0); eq("daily_goal",2000); draw()
if MODE == "ignored" then
    touch(240,200); touch(420,40); eq("today_drank",0); eq("in_settings",false)
else
    touch(10,200); touch(240,500); touch(240,200,"drag"); eq("today_drank",0)
    touch(240,200); eq("today_drank",250); eq("in_settings",false); draw()
    touch(240,700,"click"); eq("today_drank",500); draw()
    touch(420,40); eq("in_settings",true); eq("today_drank",500); draw()
    touch(240,168); eq("units","ml"); eq("daily_goal",2000)
    touch(240,120); eq("units","cups"); eq("today_drank",500); draw()
    touch(240,200); eq("daily_goal",2500); eq("step_amount",250); draw()
    touch(240,280); eq("step_amount",500); eq("today_drank",500); draw()
    touch(240,420); eq("show_history",true); draw()
    touch(240,750); eq("show_history",false); eq("in_settings",true)
    touch(240,40); eq("in_settings",false)
    touch(240,200); eq("today_drank",1000)
    assert(files["water_state_v1.dat"]=="2026-10-06;1000;cups;2500;500;0;Oct 6")
    assert(files["water_history_v1.dat"]=="")
    load_app(); eq("today_drank",1000); eq("units","cups"); eq("daily_goal",2500); eq("step_amount",500); draw()
end
local before=state("today_drank")
on_button("right"); assert(state("today_drank")==before+state("step_amount"))
on_button("left"); eq("today_drank",before); draw()

local paths={}
for path in pairs(files) do paths[#paths+1]=path end
table.sort(paths)
for _,path in ipairs(paths) do print("SAVE",path,files[path]) end
-- Full deterministic draw/API trace allows exact baseline/non-Sticky comparison.
print(table.concat(trace,"\n"))
print("PASS",DEVICE,MODE)
