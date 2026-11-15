#!/usr/bin/env python3
"""Verify every adapted deck byte against supplied original source, no download."""
import pathlib,re,json,hashlib
ROOT=pathlib.Path(__file__).resolve().parents[2];SRC=pathlib.Path('/root/github/crossplay/src/apps_local')
results=[]
for app,header,marker,total in [('insider','InsiderWords.h','kWords[kWordCount] = {',560),('forehead','ForeheadWords.h','kEntries[kEntryCount] = {',2474),('wavelength','WavelengthPairs.h','kPairsEn[] = {',245)]:
 source=SRC/app/header;text=source.read_text().split(marker)[1].split('};')[0]
 if app=='wavelength':words=[a+' / '+b for a,b in re.findall(r'\{"([^"]+)", "([^"]+)"\}',text)]
 else:words=re.findall(r'"([^"]+)"',text)
 assert len(words)==total
 root=ROOT/'apps'/('cp-'+app)/'data';assert len(list(root.glob('*.txt')))==(total+31)//32
 for start in range(0,total,32):
  expected=('\n'.join(words[start:start+32])+'\n').encode();path=root/f'{start//32}.txt';assert path.read_bytes()==expected and len(expected)<2048
 results.append(dict(app=app,entries=total,source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),all_bytes_match=True))
pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports/deck-results.json').write_text(json.dumps(results,indent=2)+'\n')
print('Verified 560 Insider words, 2474 Forehead entries, 245 spectrum pairs; complete exact bounded blocks')
