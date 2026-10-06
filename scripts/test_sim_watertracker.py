import os, subprocess, shutil
from PIL import Image

artifact_dir = "/var/home/brady/.gemini/antigravity-cli/brain/f3ae393c-0fe0-4c78-8343-eed07f750bcc"

env = os.environ.copy()
for key in list(env):
    if key.startswith(("CROSSPOINT_SIM_", "CROSSINK_SIMULATOR_SMOKE")):
        del env[key]

# Clear previous water saved data
saved_dir = "fs_/.smudge/saved_data/watertracker"
os.makedirs(saved_dir, exist_ok=True)

# Write pre-populated history data so the history screen showcases past days
hist_content = "Oct 4,2250,2000,1|Oct 3,2000,2000,1|Oct 2,1500,2000,0|Oct 1,2500,2000,1|"
open(f"{saved_dir}/water_history_v1.dat", "w").write(hist_content)
app_dir = "fs_/.crosssmudge/applications/watertracker"
os.makedirs(app_dir, exist_ok=True)
open(f"{app_dir}/water_history_v1.dat", "w").write(hist_content)

# Start today with 0 ml
open(f"{saved_dir}/water_state_v1.dat", "w").write(
    "2026-10-05;0;ml;2000;250;3;Oct 5"
)
shutil.copy("apps/watertracker/main.lua", f"{app_dir}/main.lua")
shutil.copy("apps/watertracker/manifest.json", f"{app_dir}/manifest.json")
shutil.copy("apps/watertracker/icon.raw", f"{app_dir}/icon.raw")

bmp_apps = "/tmp/sim_water_apps.bmp"
bmp_empty = "/tmp/sim_water_empty.bmp"
bmp_mid = "/tmp/sim_water_mid.bmp"
bmp_goal = "/tmp/sim_water_goal.bmp"
bmp_settings = "/tmp/sim_water_settings.bmp"
bmp_history = "/tmp/sim_water_history.bmp"

for b in [bmp_apps, bmp_empty, bmp_mid, bmp_goal, bmp_settings, bmp_history]:
    if os.path.exists(b): os.remove(b)

t = 1000
inputs = [f"{t}:BACK"]
t += 300
inputs.append(f"{t}:DOWN")
t += 300
inputs.append(f"{t}:ENTER") # Enter Applications
t += 600

# Press DOWN 10 times to reach Water Tracker
for _ in range(10):
    inputs.append(f"{t}:DOWN")
    t += 150

t += 200
snap_apps_t = t # Snapshot: Applications menu showing aligned Water Tracker icon
t += 300

inputs.append(f"{t}:ENTER") # Launch Water Tracker!
t += 1000
snap_empty_t = t # Snapshot 1: Empty cup (0 / 2000 ml)

# Press Button 4 / RIGHT (+ 250ml) 4 times to reach 1000ml (50%)
for _ in range(4):
    inputs.append(f"{t}:RIGHT")
    t += 300

t += 1200 # Let banner expire so status bar is shown clean
snap_mid_t = t # Snapshot 2: Half-full (1000 / 2000 ml)

# Press Button 4 / RIGHT 4 more times to reach 2000ml (100% Goal Met!)
for _ in range(4):
    inputs.append(f"{t}:RIGHT")
    t += 300

t += 1200 # Let banner settle
snap_goal_t = t # Snapshot 3: Goal Reached! (2000 / 2000 ml)

# Press Button 2 (ENTER / Confirm) to open Settings Menu
inputs.append(f"{t}:ENTER")
t += 800
snap_settings_t = t # Snapshot 4: Settings Menu showing row 1 selected

# In Settings menu, RIGHT moves selection DOWN through rows:
# Move from row 1 down to row 5 (View Past Days): 4 steps
for _ in range(4):
    inputs.append(f"{t}:RIGHT")
    t += 250

t += 300
inputs.append(f"{t}:ENTER") # Open Past Days History
t += 800
snap_history_t = t # Snapshot 5: History / Past Days

t += 400
inputs.append(f"{t}:QUIT")

env["CROSSPOINT_SIM_INPUT_SCRIPT"] = ";".join(inputs)
env["CROSSPOINT_SIM_SCREENSHOTS"] = f"{snap_apps_t}:{bmp_apps};{snap_empty_t}:{bmp_empty};{snap_mid_t}:{bmp_mid};{snap_goal_t}:{bmp_goal};{snap_settings_t}:{bmp_settings};{snap_history_t}:{bmp_history}"
env["SDL_VIDEODRIVER"] = "dummy"

print("Running simulator test for Water Tracker...")
proc = subprocess.run([".pio/build/simulator/program"], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=40)
print(proc.stdout.decode()[-1000:])

screenshots = [
    ("apps", bmp_apps),
    ("empty", bmp_empty),
    ("mid", bmp_mid),
    ("goal", bmp_goal),
    ("settings", bmp_settings),
    ("history", bmp_history)
]

for name, bmp in screenshots:
    if os.path.exists(bmp):
        png_path = os.path.join(artifact_dir, f"sim_water_{name}.png")
        Image.open(bmp).save(png_path)
        print(f"Saved: {png_path}")
    else:
        print(f"Warning: {bmp} not found")
