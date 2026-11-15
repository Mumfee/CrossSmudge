from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[2];src=Path('/root/github/crossplay/src/apps_local/toybattle');dst=root/'apps/cp-toybattle/maps';dst.mkdir(exist_ok=True)
subprocess.run(['c++','-std=c++17','-O2','-I'+str(src),str(root/'test/crossplay_ports/export_toy.cpp'),str(src/'ToyBattleCore.cpp'),'-o','/tmp/cp-toy-export'],check=True)
for i in range(10):(dst/f'{i}.lua').write_bytes(subprocess.check_output(['/tmp/cp-toy-export',str(i)]))
