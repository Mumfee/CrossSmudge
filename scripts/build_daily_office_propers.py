#!/usr/bin/env python3
"""Build propers.txt for Divine Worship: Daily Office."""

import urllib.request
import re
import os

print("Fetching cspordo...")
req = urllib.request.Request("http://ordo.covert.org/cspordo", headers={"User-Agent": "Mozilla/5.0"})
with urllib.request.urlopen(req, timeout=10) as resp:
    data = resp.read().decode("utf-8", errors="replace")

splits = data.split("<tr id=")
entries = []

collects_lib = {
    "Michaelmas": "O EVERLASTING God, who hast ordained and constituted the services of Angels and men in a wonderful order: mercifully grant that as thy holy Angels alway do thee service in heaven; so by thy appointment they may succour and defend us on earth; through Jesus Christ thy Son our Lord, who liveth and reigneth with thee, in the unity of the Holy Spirit, ever one God, world without end. Amen.",
    "Jerome": "O GOD, who didst give unto thy Priest Saint Jerome a singular love for the holy Scriptures: grant that thy people may be more abundantly nourished by thy word, and find therein the source of eternal life; through Jesus Christ thy Son our Lord, who liveth and reigneth with thee, in the unity of the Holy Spirit, ever one God, world without end. Amen.",
    "Thérèse": "O GOD, who openest the gates of thy heavenly kingdom to the meek and lowly of heart: grant that we, following the footsteps of blessed Thérèse with childlike trust, may obtain the revelation of thy eternal glory; through Jesus Christ thy Son our Lord, who liveth and reigneth with thee, in the unity of the Holy Spirit, ever one God, world without end. Amen.",
    "Guardian Angels": "O GOD, who in thine unsearchable providence dost send thine holy Angels to succour and defend us: grant that we may alway be guarded by their protection, and rejoice in their perpetual companionship; through Jesus Christ thy Son our Lord, who liveth and reigneth with thee, in the unity of the Holy Spirit, ever one God, world without end. Amen.",
    "Francis": "O GOD, by whose grace Saint Francis, in poverty and contentment, was likened unto thy Son: give us, we beseech thee, the same mind; and grant that through all tribulations we may persevere, and with cheerful courage follow him who for our sakes became poor, even Jesus Christ thy Son our Lord. Amen.",
    "Luke": "ALMIGHTY God, who calledst Luke the Physician, whose praise is in the Gospel, to be an Evangelist and Physician of the soul: may it please thee that, by the wholesome medicines of the doctrine delivered by him, all the diseases of our souls may be healed; through the merits of thy Son Jesus Christ our Lord. Amen.",
    "Simon and Jude": "O ALMIGHTY God, who hast built thy Church upon the foundation of the Apostles and Prophets, Jesus Christ himself being the head corner-stone: grant us so to be joined together in unity of spirit by their doctrine, that we may be made an holy temple acceptable unto thee; through the same Jesus Christ our Lord. Amen.",
    "All Saints": "O ALMIGHTY God, who hast knit together thine elect in one communion and fellowship, in the mystical body of thy Son Christ our Lord: grant us grace so to follow thy blessed Saints in all virtuous and godly living, that we may come to those unspeakable joys, which thou hast prepared for them that unfeignedly love thee; through Jesus Christ our Lord. Amen.",
    "Advent": "ALMIGHTY God, give us grace that we may cast away the works of darkness, and put upon us the armour of light, now in the time of this mortal life, in which thy Son Jesus Christ came to visit us in great humility; that in the last day, when he shall come again in his glorious Majesty to judge both the quick and the dead, we may rise to the life immortal, through him who liveth and reigneth with thee and the Holy Ghost, now and ever. Amen.",
    "Christmas": "ALMIGHTY God, who hast given us thy only-begotten Son to take our nature upon him, and as at this time to be born of a pure Virgin: grant that we being regenerate, and made thy children by adoption and grace, may daily be renewed by thy Holy Spirit; through the same our Lord Jesus Christ, who liveth and reigneth with thee and the same Spirit, ever one God, world without end. Amen.",
    "Epiphany": "O GOD, who by the leading of a star didst manifest thy only-begotten Son to the Gentiles: mercifully grant, that we, which know thee now by faith, may after this life have the fruition of thy glorious Godhead; through Jesus Christ our Lord. Amen.",
    "Lent": "ALMIGHTY and everlasting God, who hatest nothing that thou hast made, and dost forgive the sins of all them that are penitent: create and make in us new and contrite hearts, that we worthily lamenting our sins, and acknowledging our wretchedness, may obtain of thee, the God of all mercy, perfect remission and forgiveness; through Jesus Christ our Lord. Amen.",
    "Easter": "ALMIGHTY God, who through thine only-begotten Son Jesus Christ hast overcome death, and opened unto us the gate of everlasting life: we humbly beseech thee, that, as by thy special grace preventing us thou dost put into our minds good desires, so by thy continual help we may bring the same to good effect; through Jesus Christ our Lord. Amen.",
    "Trinity": "ALMIGHTY and everlasting God, who hast given unto us thy servants grace, by the confession of a true faith, to acknowledge the glory of the eternal Trinity, and in the power of the Divine Majesty to worship the Unity: we beseech thee, that thou wouldest keep us stedfast in this faith, and evermore defend us from all adversities, who livest and reignest, one God, world without end. Amen.",
    "Trinity General": "LORD, we pray thee that thy grace may always precede and follow us, and make us continually to be given to all good works; through Jesus Christ thy Son our Lord, who liveth and reigneth with thee, in the unity of the Holy Spirit, ever one God, world without end. Amen.",
}

current_sunday_collect = collects_lib["Trinity General"]

for s in splits[1:]:
    day_id = s[:8]
    if not day_id.isdigit():
        continue

    y = day_id[:4]
    m = day_id[4:6]
    d = day_id[6:8]
    date_key = f"{y}-{m}-{d}"

    m_t = re.search(r"<td colspan=5>(.*?)</td>", s)
    title = re.sub(r"<[^>]+>", " ", m_t.group(1)).strip() if m_t else ""
    title = re.sub(r"&nbsp;", " ", title)
    title = re.sub(r"\s+", " ", title).strip()

    m_color = re.search(r"\b(Green|White|Red|Violet|Rose|Black)\b", s)
    color = m_color.group(1) if m_color else ""

    txt = re.sub(r"<[^>]+>", " ", s)
    txt = re.sub(r"&nbsp;", " ", txt)
    txt = re.sub(r"\s+", " ", txt).strip()

    m_mp = re.search(r"MP\s+Pss:\s*([^L]+?)\s*Lessons:\s*([^♦]+)♦\s*([^♦\n\r]+?)(?=(?:\s+Te\s+Deum|\s+EP|\s*$))", txt)
    mp_pss = m_mp.group(1).strip() if m_mp else ""
    mp_l1 = m_mp.group(2).strip() if m_mp else ""
    mp_l2 = m_mp.group(3).strip() if m_mp else ""

    ep_pss = ""
    ep_l1 = ""
    ep_l2 = ""
    idx_ep = txt.find("EP")
    if idx_ep != -1:
        ep_part = txt[idx_ep:]
        m_ep_l = re.search(r"([^♦]+)♦\s*([^♦]+?)(?=(?:\s*$|\s*DWM|\s*Lec|\s*❖|\s*Edition))", ep_part)
        if m_ep_l:
            left = m_ep_l.group(1).strip()
            ep_l2 = m_ep_l.group(2).strip()
            m_bk = re.search(r"\b(Gn|Ex|Lv|Nm|Dt|Jos|Jgs|Ru|1 Sm|2 Sm|1 Kgs|2 Kgs|1 Chr|2 Chr|Ezr|Neh|Est|Jb|Ps|Prv|Eccl|Sg|Is|Jer|Lam|Bar|Ez|Dn|Hos|Jl|Am|Ob|Jon|Mic|Nah|Hab|Zep|Hag|Zec|Mal|1 Mc|2 Mc|Wis|Sir|Tob|Jdt|Mt|Mk|Lk|Jn|Acts|Rom|1 Cor|2 Cor|Gal|Eph|Phil|Col|1 Thes|2 Thes|1 Tm|2 Tm|Ti|Phlm|Heb|Jas|1 Pt|2 Pt|1 Jn|2 Jn|3 Jn|Jude|Rv)\b.*", left)
            if m_bk:
                ep_l1 = m_bk.group(0).strip()
                ep_pss = left[:m_bk.start()].strip()
            else:
                ep_l1 = left

    mp_pss = re.sub(r"\s+", " ", mp_pss).strip()
    mp_l1 = re.sub(r"\s+", " ", mp_l1).strip()
    mp_l2 = re.sub(r"\s+", " ", mp_l2).strip()
    ep_pss = re.sub(r"EP\s*[12]?\s*", "", ep_pss).strip()
    ep_pss = re.sub(r"Pss:\s*", "", ep_pss).strip()
    ep_pss = re.sub(r"\s+", " ", ep_pss).strip()
    ep_l1 = re.sub(r"\s+", " ", ep_l1).strip()
    ep_l2 = re.sub(r"\s+", " ", ep_l2).strip()

    collect = ""
    if "Michael" in title:
        collect = collects_lib["Michaelmas"]
    elif "Jerome" in title:
        collect = collects_lib["Jerome"]
    elif "Thérèse" in title:
        collect = collects_lib["Thérèse"]
    elif "Guardian Angels" in title:
        collect = collects_lib["Guardian Angels"]
    elif "Francis" in title:
        collect = collects_lib["Francis"]
    elif "Luke" in title:
        collect = collects_lib["Luke"]
    elif "Simon and Jude" in title or "Jude" in title:
        collect = collects_lib["Simon and Jude"]
    elif "All Saints" in title:
        collect = collects_lib["All Saints"]
    elif "Advent" in title:
        current_sunday_collect = collects_lib["Advent"]
        collect = current_sunday_collect
    elif "Christmas" in title or "Nativity" in title:
        current_sunday_collect = collects_lib["Christmas"]
        collect = current_sunday_collect
    elif "Epiphany" in title:
        current_sunday_collect = collects_lib["Epiphany"]
        collect = current_sunday_collect
    elif "Lent" in title:
        current_sunday_collect = collects_lib["Lent"]
        collect = current_sunday_collect
    elif "Easter" in title:
        current_sunday_collect = collects_lib["Easter"]
        collect = current_sunday_collect
    elif "Trinity" in title:
        current_sunday_collect = collects_lib["Trinity General"]
        collect = current_sunday_collect
    else:
        collect = current_sunday_collect

    if date_key == "2026-09-30":
        mp_l1_text = "Those of the foreigners who escaped went and reported to Lysias all that had happened. 27 When he heard it, he was perplexed and discouraged, for things had not happened to Israel as he had intended, nor had they turned out as the king had commanded him. 28 But the next year he mustered sixty thousand picked infantrymen and five thousand cavalry to subdue them. 29 They came into Idumea and encamped at Beth-zur, and Judas met them with ten thousand men. 30 When he saw that the army was strong, he prayed, saying, \"Blessed art thou, O Savior of Israel, who didst crush the attack of the mighty warrior by the hand of thy servant David, and didst give the camp of the Philistines into the hands of Jonathan, the son of Saul, and of the man who carried his armor. 31 So do thou hem in this army by the hand of thy people Israel, and let them be ashamed of their troops and their cavalry. 32 Fill them with cowardice; melt the boldness of their strength; let them tremble in their destruction. 33 Strike them down with the sword of those who love thee, and let all who know thy name praise thee with hymns.\" 34 Then both sides attacked, and there fell of the army of Lysias five thousand men; they fell in action. 35 And when Lysias saw the rout of his troops and observed the boldness which inspired those of Judas, and how ready they were either to live or to die nobly, he departed to Antioch and enlisted mercenaries, to invade Judea again with an even larger army."
        mp_l2_text = "Paul, a prisoner for Christ Jesus, and Timothy our brother, To Philemon our beloved fellow worker 2 and Apphia our sister and Archippus our fellow soldier, and the church in your house: 3 Grace to you and peace from God our Father and the Lord Jesus Christ. 4 I thank my God always when I remember you in my prayers, 5 because I hear of your love and of the faith which you have toward the Lord Jesus and all the saints, 6 and I pray that the sharing of your faith may promote the knowledge of all the good that is ours in Christ. 7 For I have derived much joy and comfort from your love, my brother, because the hearts of the saints have been refreshed through you. 8 Accordingly, though I am bold enough in Christ to command you to do what is required, 9 yet for love\'s sake I prefer to appeal to you--I, Paul, an ambassador and now a prisoner also for Christ Jesus-- 10 I appeal to you for my child, Onesimus, whose father I have become in my imprisonment. 11 (Formerly he was useless to you, but now he is indeed useful to you and to me.) 12 I am sending him back to you, sending my very heart. 13 I would have been glad to keep him with me, in order that he might serve me on your behalf during my imprisonment for the gospel; 14 but I preferred to do nothing without your consent in order that your goodness might not be by compulsion but of your own free will. 15 Perhaps this is why he was parted from you for a while, that you might have him back for ever, 16 no longer as a slave but more than a slave, as a beloved brother, especially to me but how much more to you, both in the flesh and in the Lord. 17 So if you consider me your partner, receive him as you would receive me. 18 If he has wronged you at all, or owes you anything, charge that to my account. 19 I, Paul, write this with my own hand, I will repay it--to say nothing of your owing me even your own self. 20 Yes, brother, I want some benefit from you in the Lord. Refresh my heart in Christ. 21 Confident of your obedience, I write to you, knowing that you will do even more than I say. 22 At the same time, prepare a guest room for me, for I am hoping through your prayers to be granted to you. 23 Epaphras, my fellow prisoner in Christ Jesus, sends greetings to you, 24 and so do Mark, Aristarchus, Demas, and Luke, my fellow workers. 25 The grace of the Lord Jesus Christ be with your spirit."
        ep_l1_text = "Then said Judas and his brothers, \"Behold, our enemies are crushed; let us go up to cleanse the sanctuary and dedicate it.\" 37 So all the army assembled and they went up to Mount Zion. 38 And they saw the sanctuary desolate, the altar profaned, and the gates burned. In the courts they saw bushes sprung up as in a thicket, or as on one of the mountains. They saw also the chambers of the priests in ruins. 39 Then they rent their clothes, and mourned with great lamentation, and sprinkled themselves with ashes. 40 They fell face down on the ground, and sounded the signal on the trumpets, and cried out to Heaven. 41 Then Judas detailed men to fight against those in the citadel until he had cleansed the sanctuary. 42 He chose blameless priests devoted to the law, 43 and they cleansed the sanctuary and removed the defiled stones to an unclean place. 44 They deliberated what to do about the altar of burnt offering, which had been profaned. 45 And they thought it best to tear it down, lest it bring reproach upon them, for the Gentiles had defiled it. So they tore down the altar, 46 and stored the stones in a convenient place on the temple hill until there should come a prophet to tell what to do with them. 47 Then they took unhewn stones, as the law directs, and built a new altar like the former one. 48 They also rebuilt the sanctuary and the interior of the temple, and consecrated the courts. 49 They made new holy vessels, and brought the lampstand, the altar of incense, and the table into the temple. 50 Then they burned incense on the altar and lighted the lamps on the lampstand, and these gave light in the temple. 51 They placed the bread on the table and hung up the curtains. Thus they finished all the work they had undertaken. 52 Early in the morning on the twenty-fifth day of the ninth month, which is the month of Chislev, in the one hundred and forty-eighth year, 53 they rose and offered sacrifice, as the law directs, on the new altar of burnt offering which they had built. 54 At the very season and on the very day that the Gentiles had profaned it, it was dedicated with songs and harps and lutes and cymbals. 55 All the people fell on their faces and worshiped and blessed Heaven, who had prospered them. 56 So they celebrated the dedication of the altar for eight days, and offered burnt offerings with gladness; they offered a sacrifice of deliverance and praise. 57 They decorated the front of the temple with golden crowns and small shields; they restored the gates and the chambers for the priests, and furnished them with doors. 58 There was very great gladness among the people, and the reproach of the Gentiles was removed. 59 Then Judas and his brothers and all the assembly of Israel determined that every year at that season the days of dedication of the altar should be observed with gladness and joy for eight days, beginning with the twenty-fifth day of the month of Chislev. 60 At that time they fortified Mount Zion with high walls and strong towers round about, to keep the Gentiles from coming and trampling them down as they had done before. 61 And he stationed a garrison there to hold it. He also fortified Beth-zur, so that the people might have a stronghold that faced Idumea."
        ep_l2_text = "Then they led Jesus from the house of Caiaphas to the praetorium. It was early. They themselves did not enter the praetorium, so that they might not be defiled, but might eat the passover. 29 So Pilate went out to them and said, \"What accusation do you bring against this man?\" 30 They answered him, \"If this man were not an evildoer, we would not have handed him over.\" 31 Pilate said to them, \"Take him yourselves and judge him by your own law.\" The Jews said to him, \"It is not lawful for us to put any man to death.\" 32 This was to fulfill the word which Jesus had spoken to show by what death he was to die. 33 Pilate entered the praetorium again and called Jesus, and said to him, \"Are you the King of the Jews?\" 34 Jesus answered, \"Do you say this of your own accord, or did others say it to you about me?\" 35 Pilate answered, \"Am I a Jew? Your own nation and the chief priests have handed you over to me; what have you done?\" 36 Jesus answered, \"My kingship is not of this world; if my kingship were of this world, my servants would fight, that I might not be handed over to the Jews; but my kingship is not from the world.\" 37 Pilate said to him, \"So you are a king?\" Jesus answered, \"You say that I am a king. For this I was born, and for this I have come into the world, to bear witness to the truth. Every one who is of the truth hears my voice.\" 38 Pilate said to him, \"What is truth?\" After he had said this, he went out to the Jews again, and told them, \"I find no crime in him. 39 But you have a custom that I should release one man for you at the Passover; will you have me release for you the King of the Jews?\" 40 They cried out again, \"Not this man, but Barabbas!\" Now Barabbas was a robber."
    else:
        mp_l1_text = ""
        mp_l2_text = ""
        ep_l1_text = ""
        ep_l2_text = ""

    entries.append({
        "date": date_key,
        "month_day": f"{m}-{d}",
        "title": title,
        "color": color,
        "mp_pss": mp_pss,
        "mp_l1": mp_l1,
        "mp_l1_text": mp_l1_text,
        "mp_l2": mp_l2,
        "mp_l2_text": mp_l2_text,
        "ep_pss": ep_pss,
        "ep_l1": ep_l1,
        "ep_l1_text": ep_l1_text,
        "ep_l2": ep_l2,
        "ep_l2_text": ep_l2_text,
        "collect": collect
    })

print(f"Compiled {len(entries)} entries.")

propers_path = "data/daily_office/propers.txt"
lines = []
for e in entries:
    lines.append(f"[{e['date']}]")
    lines.append(f"MONTH_DAY={e['month_day']}")
    if e["title"]:
        lines.append(f"TITLE={e['title']}")
    if e["color"]:
        lines.append(f"COLOR={e['color']}")
    if e["mp_pss"]:
        lines.append(f"MP_PSS={e['mp_pss']}")
    if e["mp_l1"]:
        lines.append(f"MP_L1={e['mp_l1']}")
    if e.get("mp_l1_text"):
        lines.append(f"MP_L1_TEXT={e['mp_l1_text']}")
    if e["mp_l2"]:
        lines.append(f"MP_L2={e['mp_l2']}")
    if e.get("mp_l2_text"):
        lines.append(f"MP_L2_TEXT={e['mp_l2_text']}")
    if e["ep_pss"]:
        lines.append(f"EP_PSS={e['ep_pss']}")
    if e["ep_l1"]:
        lines.append(f"EP_L1={e['ep_l1']}")
    if e.get("ep_l1_text"):
        lines.append(f"EP_L1_TEXT={e['ep_l1_text']}")
    if e["ep_l2"]:
        lines.append(f"EP_L2={e['ep_l2']}")
    if e.get("ep_l2_text"):
        lines.append(f"EP_L2_TEXT={e['ep_l2_text']}")
    if e["collect"]:
        lines.append(f"COLLECT={e['collect']}")
    lines.append("")

content = "\n".join(lines)
os.makedirs("data/daily_office", exist_ok=True)
with open(propers_path, "w", encoding="utf-8") as f:
    f.write(content)

os.makedirs("fs_/.smudge/daily_office", exist_ok=True)
with open("fs_/.smudge/daily_office/propers.txt", "w", encoding="utf-8") as f:
    f.write(content)

print(f"Wrote {len(content)} bytes ({len(content)/1024:.1f} KB) to data/daily_office/propers.txt and fs_/.smudge/daily_office/propers.txt")

import zlib
content_bytes = content.encode("utf-8")
comp = zlib.compressobj(level=9, wbits=-15)
compressed = comp.compress(content_bytes) + comp.flush()

header_lines = [
    "#pragma once",
    "",
    "#include <cstddef>",
    "#include <cstdint>",
    "",
    "namespace DailyOfficePropersData {",
    "",
    f"static constexpr size_t kCompressedSize = {len(compressed)};",
    f"static constexpr size_t kUncompressedSize = {len(content_bytes)};",
    "",
    f"static const uint8_t kCompressedPropers[kCompressedSize] = {{"
]

chunk_size = 19
for i in range(0, len(compressed), chunk_size):
    chunk = compressed[i : i + chunk_size]
    hex_strs = [f"0x{b:02x}" for b in chunk]
    sep = "," if i + chunk_size < len(compressed) else ""
    header_lines.append("    " + ", ".join(hex_strs) + sep)

header_lines.append("};")
header_lines.append("")
header_lines.append("}  // namespace DailyOfficePropersData")
header_lines.append("")

header_path = "src/activities/apps/assets/DailyOfficePropersData.h"
with open(header_path, "w", encoding="utf-8") as f:
    f.write("\n".join(header_lines))

print(f"Generated {header_path} ({len(compressed)} bytes compressed, {len(content_bytes)} uncompressed)")

