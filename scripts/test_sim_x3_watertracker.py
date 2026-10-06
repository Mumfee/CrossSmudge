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
app_dir = "fs_/.crosssmudge/applications/watertracker"
os.makedirs(app_dir, exist_ok=True)

# Remove any old saved state
for d in [saved_dir, app_dir]:
    for fn in ["water_state_v1.dat", "water_history_v1.dat"]:
        p = os.path.join(d, fn)
        if os.path.exists(p):
            os.remove(p)

# Pre-populate 14-day history
hist_content = "Oct 4,2250,2000,1|Oct 3,2000,2000,1|Oct 2,1500,2000,0|Oct 1,2500,2000,1|"
open(f"{saved_dir}/water_history_v1.dat", "w").write(hist_content)
open(f"{app_dir}/water_history_v1.dat", "w").write(hist_content)

# Start today with 0 ml
today_str = "2026-10-06;0;ml;2000;250;3;Oct 6"
open(f"{saved_dir}/water_state_v1.dat", "w").write(today_str)
open(f"{app_dir}/water_state_v1.dat", "w").write(today_str)

shutil.copy("apps/watertracker/main.lua", f"{app_dir}/main.lua")
shutil.copy("apps/watertracker/manifest.json", f"{app_dir}/manifest.json")
shutil.copy("apps/watertracker/icon.raw", f"{app_dir}/icon.raw")

bmp_empty = "/tmp/sim_x3_water_empty.bmp"
bmp_mid = "/tmp/sim_x3_water_mid.bmp"
bmp_goal = "/tmp/sim_x3_water_goal.bmp"
bmp_settings = "/tmp/sim_x3_water_settings.bmp"
bmp_history = "/tmp/sim_x3_water_history.bmp"

for b in [bmp_empty, bmp_mid, bmp_goal, bmp_settings, bmp_history]:
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
inputs.append(f"{t}:ENTER") # Launch Water Tracker
t += 1000
snap_empty_t = t # Snapshot 1: Empty cup (0 / 2000 ml)

# Press Button 4 / RIGHT (+ 250ml) 4 times to reach 1000ml (50%)
for _ in range(4):
    inputs.append(f"{t}:RIGHT")
    t += 300

t += 100
snap_banner_active_t = t # Snapshot: banner active ("+250 ml logged!")

t += 2600 # Wait for 2200ms timeout + redraw to settle
snap_banner_settled_t = t # Snapshot: banner timed out, normal top bar restored!

# Press Button 4 / RIGHT 4 more times to reach 2000ml (100% Goal Met!)
for _ in range(4):
    inputs.append(f"{t}:RIGHT")
    t += 300

t += 100
snap_goal_active_t = t # Snapshot: "Goal Reached! Fantastic!" banner

t += 3500 # Wait for 3000ms goal timeout + redraw
snap_goal_settled_t = t # Snapshot: Goal met, normal top bar restored!

# Press Button 2 (ENTER / Confirm) to open Settings Menu
inputs.append(f"{t}:ENTER")
t += 800
snap_settings_t = t # Snapshot 4: Settings Menu

# In Settings, navigate to row 5 (Past Days History)
for _ in range(4):
    inputs.append(f"{t}:RIGHT")
    t += 250

t += 300
inputs.append(f"{t}:ENTER") # Open History
t += 800
snap_history_t = t # Snapshot 5: History

t += 400
inputs.append(f"{t}:QUIT")

bmp_banner_active = "/tmp/sim_x3_banner_active.bmp"
bmp_banner_settled = "/tmp/sim_x3_banner_settled.bmp"
bmp_goal_active = "/tmp/sim_x3_goal_active.bmp"
bmp_goal_settled = "/tmp/sim_x3_goal_settled.bmp"

env["CROSSPOINT_SIM_INPUT_SCRIPT"] = ";".join(inputs)
env["CROSSPOINT_SIM_SCREENSHOTS"] = f"{snap_empty_t}:{bmp_empty};{snap_banner_active_t}:{bmp_banner_active};{snap_banner_settled_t}:{bmp_banner_settled};{snap_goal_active_t}:{bmp_goal_active};{snap_goal_settled_t}:{bmp_goal_settled};{snap_settings_t}:{bmp_settings};{snap_history_t}:{bmp_history}"
env["SDL_VIDEODRIVER"] = "dummy"

print("Running simulator-X3 test for Water Tracker (528x792)...")
cmd = [".pio/build/simulator-X3/program"]
res = subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

print("Exit code:", res.returncode)

screenshots = [
    (bmp_empty, f"{artifact_dir}/sim_x3_water_empty.png"),
    (bmp_banner_active, f"{artifact_dir}/sim_x3_banner_active.png"),
    (bmp_banner_settled, f"{artifact_dir}/sim_x3_banner_settled.png"),
    (bmp_goal_active, f"{artifact_dir}/sim_x3_goal_active.png"),
    (bmp_goal_settled, f"{artifact_dir}/sim_x3_goal_settled.png"),
    (bmp_settings, f"{artifact_dir}/sim_x3_water_settings.png"),
    (bmp_history, f"{artifact_dir}/sim_x3_water_history.png"),
]

for src, dst in screenshots:
    if os.path.exists(src):
        Image.open(src).save(dst)
        print("Saved:", dst)
    else:
        print("Missing:", src)
