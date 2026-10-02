import os
import glob
from PIL import Image, ImageEnhance, ImageOps

brain_dir = "/var/home/brady/.gemini/antigravity-cli/brain/f3ae393c-0fe0-4c78-8343-eed07f750bcc"

def despeckle(img):
    w, h = img.size
    pixels = img.load()
    out = img.copy()
    out_pixels = out.load()
    for y in range(1, h - 1):
        for x in range(1, w - 1):
            if pixels[x, y] == 0:
                n = 0
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        if (dx != 0 or dy != 0) and pixels[x + dx, y + dy] == 0:
                            n += 1
                if n <= 1:
                    out_pixels[x, y] = 255
    return out

crops = {
    # Monster 0..6 (Originals)
    "SpriteImp": ("mon_ink_imp_*.jpg", (120, 40, 880, 860), 128),
    "SpriteGargoyle": ("mon_stone_gargoyle_*.jpg", (120, 60, 880, 820), 128),
    "SpriteScribe": ("mon_cursed_scribe_*.jpg", (240, 20, 880, 840), 128),
    "SpriteGhoul": ("mon_parchment_ghoul_*.jpg", (160, 60, 880, 820), 128),
    "SpriteGolem": ("mon_book_golem_*.jpg", (100, 20, 940, 900), 128),
    "SpriteFiend": ("mon_marginalia_fiend_*.jpg", (40, 40, 960, 940), 128),
    "SpriteInquisitor": ("inquisitor_judge_*.jpg", (140, 50, 884, 794), 128),
    # New Monsters 7..11
    "SpriteHound": ("mon_quill_hound_*.jpg", (40, 40, 980, 980), 128),
    "SpriteWarden": ("mon_crypt_warden_*.jpg", (60, 20, 960, 960), 128),
    "SpriteSpineHorror": ("mon_spine_horror_*.jpg", (40, 40, 980, 980), 128),
    "SpriteIronScriptor": ("mon_iron_scriptor_*.jpg", (60, 40, 940, 940), 128),
    "SpriteArchHeretic": ("mon_arch_heretic_*.jpg", (60, 20, 960, 960), 128),
    # Shopkeeper
    "SpriteShopMerchant": ("shop_merchant_*.jpg", (50, 50, 970, 970), 128),
    # App Icons & Title Art
    "CodexIcon": ("codex_icon_woodcut_*.jpg", (60, 90, 940, 840), 32),
    "SpriteCodexLarge": ("codex_icon_woodcut_*.jpg", (60, 90, 940, 840), 96),
    # Events 0..8
    "SpriteEventReliquary": ("event_reliquary_*.jpg", (100, 120, 920, 920), 128),
    "SpriteEventScriptorium": ("event_scriptorium_*.jpg", (50, 50, 970, 970), 128),
    "SpriteEventAlchemist": ("event_alchemist_*.jpg", (40, 40, 980, 980), 128),
    "SpriteEventAmbush": ("event_ambush_*.jpg", (50, 50, 970, 970), 128),
    "SpriteEventLectern": ("event_lectern_*.jpg", (150, 50, 870, 970), 128),
    "SpriteEventPeddler": ("event_peddler_*.jpg", (45, 45, 975, 975), 128),
    "SpriteEventBloodAltar": ("event_blood_altar_*.jpg", (50, 50, 970, 970), 128),
    "SpriteEventTortureVault": ("event_torture_vault_*.jpg", (40, 40, 980, 980), 128),
    "SpriteEventFountain": ("fountain_ink_*.jpg", (60, 60, 960, 960), 128)
}

results = {}

for name, (pat, crop_box, sz) in crops.items():
    matches = glob.glob(os.path.join(brain_dir, pat))
    if not matches:
        print(f"Error: {pat} not found")
        continue
    raw = Image.open(matches[0]).convert('L')
    cropped = raw.crop(crop_box)
    
    w, h = cropped.size
    max_dim = max(w, h)
    sq = Image.new('L', (max_dim, max_dim), 255)
    sq.paste(cropped, ((max_dim - w) // 2, (max_dim - h) // 2))
    
    if name == "SpriteInquisitor":
        sq = ImageEnhance.Brightness(sq).enhance(1.25)

    enh = ImageEnhance.Contrast(sq)
    contrasted = enh.enhance(1.6)
    
    inner = sz - 2
    resized = contrasted.resize((inner, inner), Image.Resampling.LANCZOS)
    canvas = Image.new('L', (sz, sz), 255)
    canvas.paste(resized, (1, 1))
    
    bw = canvas.point(lambda p: 255 if p > 135 else 0, mode='1').convert('L')
    cleaned = despeckle(bw)
    
    if name.startswith("SpriteEvent") or name.startswith("SpriteShop") or name in ("SpriteHound", "SpriteWarden", "SpriteSpineHorror", "SpriteIronScriptor", "SpriteArchHeretic"):
        cpix = cleaned.load()
        cw, ch = cleaned.size
        for cy in range(ch):
            for cx in range(cw):
                if cx <= 3 or cx >= cw - 4 or cy <= 3 or cy >= ch - 4:
                    cpix[cx, cy] = 255

    os.makedirs("/tmp/codex_final", exist_ok=True)
    cleaned.save(f"/tmp/codex_final/{name}.png")
    results[name] = (cleaned, sz)
    print(f"Generated {name} ({sz}x{sz})")

# Process 12 Relic Icons (48x48) from relics_grid_sheet_*.jpg
relic_sheet_matches = glob.glob(os.path.join(brain_dir, "relics_grid_sheet_*.jpg"))
if relic_sheet_matches:
    sheet = Image.open(relic_sheet_matches[0]).convert('L')
    sw, sh = sheet.size
    cell_w = sw / 4
    cell_h = sh / 4
    relic_coords = [
        (0, 0, 'SpriteRelicSilverQuill'),
        (0, 1, 'SpriteRelicIronSigil'),
        (0, 2, 'SpriteRelicVampiricSeal'),
        (1, 0, 'SpriteRelicScholarsRing'),
        (1, 1, 'SpriteRelicWhetstone'),
        (1, 2, 'SpriteRelicMonksRosary'),
        (2, 0, 'SpriteRelicGoldenBookmark'),
        (2, 1, 'SpriteRelicObsidianInkwell'),
        (2, 2, 'SpriteRelicBarbedBookmark'),
        (3, 0, 'SpriteRelicHourglassOfSand'),
        (3, 2, 'SpriteRelicCenserOfCleansing'),
        (3, 3, 'SpriteRelicAlchemicalFlask'),
    ]
    for r, c, rname in relic_coords:
        x0, y0 = int(c * cell_w), int(r * cell_h)
        x1, y1 = int((c + 1) * cell_w), int((r + 1) * cell_h)
        cell = sheet.crop((x0, y0, x1, y1))
        
        # Auto-crop content bounding box
        cw, ch = cell.size
        min_x, min_y, max_x, max_y = cw, ch, 0, 0
        found = False
        for cy in range(ch):
            for cx in range(cw):
                if cell.getpixel((cx, cy)) < 235:
                    found = True
                    if cx < min_x: min_x = cx
                    if cx > max_x: max_x = cx
                    if cy < min_y: min_y = cy
                    if cy > max_y: max_y = cy
        if found:
            min_x = max(0, min_x - 4)
            min_y = max(0, min_y - 4)
            max_x = min(cw - 1, max_x + 4)
            max_y = min(ch - 1, max_y + 4)
            cell = cell.crop((min_x, min_y, max_x + 1, max_y + 1))
            
        bw, bh = cell.size
        scale = 44 / max(bw, bh)
        nw, nh = max(1, int(bw * scale)), max(1, int(bh * scale))
        resized = cell.resize((nw, nh), Image.Resampling.LANCZOS)
        
        rel_img = Image.new('L', (48, 48), 255)
        bx = (48 - nw) // 2
        by = (48 - nh) // 2
        for cy in range(nh):
            for cx in range(nw):
                if resized.getpixel((cx, cy)) < 170:
                    rel_img.putpixel((bx + cx, by + cy), 0)
                    
        cleaned_relic = despeckle(rel_img)
        # Clear 1px boundary
        rpix = cleaned_relic.load()
        for i in range(48):
            rpix[i, 0] = 255
            rpix[i, 47] = 255
            rpix[0, i] = 255
            rpix[47, i] = 255
            
        cleaned_relic.save(f"/tmp/codex_final/{rname}.png")
        results[rname] = (cleaned_relic, 48)
        print(f"Generated {rname} (48x48)")

def to_c_array(img, name, sz):
    # Rotate 90 CCW for CrossSmudge GfxRenderer::drawIcon format
    rot = img.rotate(90, expand=True).convert('L')
    w, h = rot.size
    pixels = list(rot.getdata())
    packed = []
    for y in range(h):
        for x in range(0, w, 8):
            byte = 0
            for b in range(8):
                if x + b < w:
                    v = pixels[y * w + x + b]
                    # 1=White, 0=Black
                    bit = 1 if v >= 128 else 0
                    byte |= (bit << (7 - b))
            packed.append(byte)
            
    s = f"// {sz}x{sz} 1-bit monochrome woodcut\n"
    s += f"static const uint8_t {name}[{len(packed)}] = {{\n    "
    for i, b in enumerate(packed):
        s += f"0x{b:02x}, "
        if (i + 1) % 16 == 0 and i + 1 < len(packed):
            s += "\n    "
    s = s.rstrip(", \n") + "\n};\n\n"
    return s

# Write src/components/icons/codex.h (CodexIcon 32x32)
with open("src/components/icons/codex.h", "w") as f:
    f.write("#pragma once\n#include <cstdint>\n\n")
    f.write(to_c_array(results["CodexIcon"][0], "CodexIcon", 32))

# Write src/activities/apps/codex/CodexSprites.h
with open("src/activities/apps/codex/CodexSprites.h", "w") as f:
    f.write("#pragma once\n#include <cstdint>\n\nnamespace codex {\n\n")
    
    # 1. Monster sprites and Title art
    monster_names = [
        "SpriteImp", "SpriteGargoyle", "SpriteScribe", "SpriteGhoul",
        "SpriteGolem", "SpriteFiend", "SpriteInquisitor",
        "SpriteHound", "SpriteWarden", "SpriteSpineHorror",
        "SpriteIronScriptor", "SpriteArchHeretic"
    ]
    for name in monster_names + ["SpriteCodexLarge", "SpriteShopMerchant"]:
        if name in results:
            f.write(to_c_array(results[name][0], name, results[name][1]))
    
    f.write(f"constexpr int kMonsterSpriteSize = 128;\n\n")
    f.write(f"static const uint8_t* const kMonsterSprites[{len(monster_names)}] = {{\n")
    f.write("    " + ", ".join(monster_names) + "\n")
    f.write("};\n\n")

    # 2. Event sprites
    event_names = [
        "SpriteEventReliquary", "SpriteEventScriptorium", "SpriteEventAlchemist",
        "SpriteEventAmbush", "SpriteEventLectern", "SpriteEventPeddler",
        "SpriteEventBloodAltar", "SpriteEventTortureVault", "SpriteEventFountain"
    ]
    for name in event_names:
        if name in results:
            f.write(to_c_array(results[name][0], name, results[name][1]))

    f.write("constexpr int kEventSpriteSize = 128;\n\n")
    f.write(f"static const uint8_t* const kEventSprites[{len(event_names)}] = {{\n")
    f.write("    " + ", ".join(event_names) + "\n")
    f.write("};\n\n")

    # 3. Relic sprites
    relic_names = [r[2] for r in relic_coords]
    for name in relic_names:
        if name in results:
            f.write(to_c_array(results[name][0], name, results[name][1]))

    f.write("constexpr int kRelicSpriteSize = 48;\n\n")
    f.write(f"static const uint8_t* const kRelicSprites[{len(relic_names)}] = {{\n")
    f.write("    " + ", ".join(relic_names) + "\n")
    f.write("};\n\n")

    f.write("} // namespace codex\n")

print("Generated codex.h and CodexSprites.h successfully with monsters, events, shop, and relics!")
