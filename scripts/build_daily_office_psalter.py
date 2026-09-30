#!/usr/bin/env python3
"""Build psalter.txt and DailyOfficePsalterData.h for Coverdale Psalter."""

import os
import re
import urllib.request
import zlib


def clean_psalm_body(text):
  text = re.sub(r"<BR\s*/?>", "\n", text, flags=re.I)
  text = re.sub(r"</?P[^>]*>", "\n\n", text, flags=re.I)
  text = re.sub(r"&nbsp;", " ", text)
  text = re.sub(r"&#97;", "a", text)
  text = re.sub(
      r"<span[^>]*font-variant:\s*small-caps[^>]*>Lord</span>",
      "LORD",
      text,
      flags=re.I,
  )
  text = re.sub(
      r"<span[^>]*font-variant:\s*small-caps[^>]*>God</span>",
      "GOD",
      text,
      flags=re.I,
  )
  text = re.sub(r"<[^>]+>", "", text)
  text = re.sub(r"\r", "", text)

  raw_lines = text.split("\n")
  cleaned_lines = []
  for l in raw_lines:
    l = l.strip()
    if (
        re.match(r"^The \w+ Day\.$", l)
        or re.match(r"^The Twenty-\w+ Day\.$", l)
        or re.match(r"^The Thirty-\w+ Day\.$", l)
        or l in ("Morning Prayer.", "Evening Prayer.")
    ):
      continue
    cleaned_lines.append(l)

  out = []
  for l in cleaned_lines:
    if not l:
      if out and out[-1] != "":
        out.append("")
    else:
      out.append(l)
  return "\n".join(out).strip()


print("Fetching Coverdale Psalter books 1 to 5...")
bk_html = {}
for bk in range(1, 6):
  url = f"http://www.covert.org/Psalter/Bk{bk}.html"
  req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
  with urllib.request.urlopen(req, timeout=15) as resp:
    bk_html[bk] = resp.read().decode("utf-8", errors="replace")

all_html = "\n".join(bk_html[b] for b in range(1, 6))

matches = list(
    re.finditer(
        r"<a\s+name=[\"\x27]?Ps(\d+)[a-z]?[\"\x27]?></a>(.*?)(?=<a\s+name=[\"\x27]?Ps\d+|\Z)",
        all_html,
        re.S | re.I,
    )
)

print(f"Matched {len(matches)} psalms.")

psalms = {}
for m in matches:
  num = int(m.group(1))
  body = clean_psalm_body(m.group(2))
  psalms[num] = body

# Extract Psalm 119 sub-portions from Bk5
bk5 = bk_html[5]
p119_tags = [
    ("119:1-32", "Day24E", "Day25M"),
    ("119:33-72", "Day25M", "Day25E"),
    ("119:73-104", "Day25E", "Day26M"),
    ("119:105-144", "Day26M", "Day26E"),
    ("119:145-176", "Day26E", "Ps120"),
]
p119_sections = {}
for label, start_tag, end_tag in p119_tags:
  i1 = bk5.find(start_tag)
  i2 = bk5.find(end_tag)
  if i1 != -1 and i2 != -1:
    snippet = bk5[i1:i2]
    idx = snippet.find(">")
    if idx != -1:
      snippet = snippet[idx + 1 :]
    p119_sections[label] = clean_psalm_body(snippet)

# Build full psalter text
output = []
for num in range(1, 151):
  if num == 119:
    # Add each section of 119
    for label, _s, _e in p119_tags:
      if label in p119_sections:
        output.append(f"[PSALM {label}]")
        output.append(p119_sections[label])
        output.append("")
    # Also add general [PSALM 119]
    output.append("[PSALM 119]")
    output.append(psalms[119])
    output.append("")
  elif num in psalms:
    output.append(f"[PSALM {num}]")
    output.append(psalms[num])
    output.append("")

full_psalter = "\n".join(output)
psalter_bytes = full_psalter.encode("utf-8")
print(
    f"Uncompressed psalter text: {len(psalter_bytes)} bytes"
    f" ({len(psalter_bytes)/1024:.1f} KB)"
)

# Raw deflate compression (wbits=-15) for InflateStream
compressor = zlib.compressobj(level=9, wbits=-15)
compressed = compressor.compress(psalter_bytes) + compressor.flush()
print(
    f"Raw Deflate compressed size: {len(compressed)} bytes"
    f" ({len(compressed)/1024:.1f} KB)"
)

# Write psalter.txt files
os.makedirs("data/daily_office", exist_ok=True)
with open("data/daily_office/psalter.txt", "w", encoding="utf-8") as f:
  f.write(full_psalter)

os.makedirs("fs_/.smudge/daily_office", exist_ok=True)
with open("fs_/.smudge/daily_office/psalter.txt", "w", encoding="utf-8") as f:
  f.write(full_psalter)

# Generate DailyOfficePsalterData.h
header_lines = [
    "#pragma once",
    "",
    "#include <cstddef>",
    "#include <cstdint>",
    "",
    "namespace DailyOfficePsalterData {",
    "",
    f"static constexpr size_t kCompressedSize = {len(compressed)};",
    f"static constexpr size_t kUncompressedSize = {len(psalter_bytes)};",
    "",
    f"static const uint8_t kCompressedPsalter[kCompressedSize] = {{",
]

# Format hex bytes 19 per line
chunk_size = 19
for i in range(0, len(compressed), chunk_size):
  chunk = compressed[i : i + chunk_size]
  hex_strs = [f"0x{b:02x}" for b in chunk]
  sep = "," if i + chunk_size < len(compressed) else ""
  header_lines.append("    " + ", ".join(hex_strs) + sep)

header_lines.append("};")
header_lines.append("")
header_lines.append("}  // namespace DailyOfficePsalterData")
header_lines.append("")

header_path = "src/activities/apps/assets/DailyOfficePsalterData.h"
os.makedirs(os.path.dirname(header_path), exist_ok=True)
with open(header_path, "w", encoding="utf-8") as f:
  f.write("\n".join(header_lines))

print(f"Generated {header_path} successfully!")
