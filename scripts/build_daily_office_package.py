#!/usr/bin/env python3
"""Build and package complete offline data for Divine Worship: Daily Office.

This script creates:
  1. psalter.txt - Full Coverdale Psalter (Psalms 1-150)
  2. propers.txt - 397-day liturgical calendar with appointed psalms, readings, collects
  3. lessons/ - Daily Scripture readings (RSV-2CE)
  4. daily_office_sd_package.zip - Ready to extract onto SD card in /.smudge/daily_office/
"""

import os
import shutil
import subprocess
import sys
import zipfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
DATA_DIR = os.path.join(REPO_ROOT, "data", "daily_office")
FS_DIR = os.path.join(REPO_ROOT, "fs_", ".smudge", "daily_office")
ZIP_PATH = os.path.join(REPO_ROOT, "data", "daily_office_sd_package.zip")

print("=== Building Daily Office Complete Offline Package ===")

# 1. Run Psalter Builder
psalter_script = os.path.join(SCRIPT_DIR, "build_daily_office_psalter.py")
print("1. Running Psalter builder...")
subprocess.run([sys.executable, psalter_script], check=True, cwd=REPO_ROOT)

# 2. Run Propers Builder
propers_script = os.path.join(SCRIPT_DIR, "build_daily_office_propers.py")
print("2. Running Propers builder...")
subprocess.run([sys.executable, propers_script], check=True, cwd=REPO_ROOT)

# 3. Create lessons directory
lessons_dir = os.path.join(DATA_DIR, "lessons")
fs_lessons_dir = os.path.join(FS_DIR, "lessons")
os.makedirs(lessons_dir, exist_ok=True)
os.makedirs(fs_lessons_dir, exist_ok=True)

# Write sample/today lessons
sample_date = "2026-09-30"
sample_content = """[MP_L1]
Those of the foreigners who escaped went and reported to Lysias all that had happened. 27 When he heard it, he was perplexed and discouraged, for things had not happened to Israel as he had intended, nor had they turned out as the king had commanded him. 28 But the next year he mustered sixty thousand picked infantrymen and five thousand cavalry to subdue them. 29 They came into Idumea and encamped at Beth-zur, and Judas met them with ten thousand men. 30 When he saw that the army was strong, he prayed, saying, "Blessed art thou, O Savior of Israel, who didst crush the attack of the mighty warrior by the hand of thy servant David, and didst give the camp of the Philistines into the hands of Jonathan, the son of Saul, and of the man who carried his armor. 31 So do thou hem in this army by the hand of thy people Israel, and let them be ashamed of their troops and their cavalry. 32 Fill them with cowardice; melt the boldness of their strength; let them tremble in their destruction. 33 Strike them down with the sword of those who love thee, and let all who know thy name praise thee with hymns." 34 Then both sides attacked, and there fell of the army of Lysias five thousand men; they fell in action. 35 And when Lysias saw the rout of his troops and observed the boldness which inspired those of Judas, and how ready they were either to live or to die nobly, he departed to Antioch and enlisted mercenaries, to invade Judea again with an even larger army.

[MP_L2]
Paul, a prisoner for Christ Jesus, and Timothy our brother, To Philemon our beloved fellow worker 2 and Apphia our sister and Archippus our fellow soldier, and the church in your house: 3 Grace to you and peace from God our Father and the Lord Jesus Christ. 4 I thank my God always when I remember you in my prayers, 5 because I hear of your love and of the faith which you have toward the Lord Jesus and all the saints, 6 and I pray that the sharing of your faith may promote the knowledge of all the good that is ours in Christ. 7 For I have derived much joy and comfort from your love, my brother, because the hearts of the saints have been refreshed through you. 8 Accordingly, though I am bold enough in Christ to command you to do what is required, 9 yet for love's sake I prefer to appeal to you--I, Paul, an ambassador and now a prisoner also for Christ Jesus-- 10 I appeal to you for my child, Onesimus, whose father I have become in my imprisonment. 11 (Formerly he was useless to you, but now he is indeed useful to you and to me.) 12 I am sending him back to you, sending my very heart. 13 I would have been glad to keep him with me, in order that he might serve me on your behalf during my imprisonment for the gospel; 14 but I preferred to do nothing without your consent in order that your goodness might not be by compulsion but of your own free will. 15 Perhaps this is why he was parted from you for a while, that you might have him back for ever, 16 no longer as a slave but more than a slave, as a beloved brother, especially to me but how much more to you, both in the flesh and in the Lord. 17 So if you consider me your partner, receive him as you would receive me. 18 If he has wronged you at all, or owes you anything, charge that to my account. 19 I, Paul, write this with my own hand, I will repay it--to say nothing of your owing me even your own self. 20 Yes, brother, I want some benefit from you in the Lord. Refresh my heart in Christ. 21 Confident of your obedience, I write to you, knowing that you will do even more than I say. 22 At the same time, prepare a guest room for me, for I am hoping through your prayers to be granted to you. 23 Epaphras, my fellow prisoner in Christ Jesus, sends greetings to you, 24 and so do Mark, Aristarchus, Demas, and Luke, my fellow workers. 25 The grace of the Lord Jesus Christ be with your spirit.

[EP_L1]
Then said Judas and his brothers, "Behold, our enemies are crushed; let us go up to cleanse the sanctuary and dedicate it." 37 So all the army assembled and they went up to Mount Zion. 38 And they saw the sanctuary desolate, the altar profaned, and the gates burned. In the courts they saw bushes sprung up as in a thicket, or as on one of the mountains. They saw also the chambers of the priests in ruins. 39 Then they rent their clothes, and mourned with great lamentation, and sprinkled themselves with ashes. 40 They fell face down on the ground, and sounded the signal on the trumpets, and cried out to Heaven. 41 Then Judas detailed men to fight against those in the citadel until he had cleansed the sanctuary. 42 He chose blameless priests devoted to the law, 43 and they cleansed the sanctuary and removed the defiled stones to an unclean place. 44 They deliberated what to do about the altar of burnt offering, which had been profaned. 45 And they thought it best to tear it down, lest it bring reproach upon them, for the Gentiles had defiled it. So they tore down the altar, 46 and stored the stones in a convenient place on the temple hill until there should come a prophet to tell what to do with them. 47 Then they took unhewn stones, as the law directs, and built a new altar like the former one. 48 They also rebuilt the sanctuary and the interior of the temple, and consecrated the courts. 49 They made new holy vessels, and brought the lampstand, the altar of incense, and the table into the temple. 50 Then they burned incense on the altar and lighted the lamps on the lampstand, and these gave light in the temple. 51 They placed the bread on the table and hung up the curtains. Thus they finished all the work they had undertaken. 52 Early in the morning on the twenty-fifth day of the ninth month, which is the month of Chislev, in the one hundred and forty-eighth year, 53 they rose and offered sacrifice, as the law directs, on the new altar of burnt offering which they had built. 54 At the very season and on the very day that the Gentiles had profaned it, it was dedicated with songs and harps and lutes and cymbals. 55 All the people fell on their faces and worshiped and blessed Heaven, who had prospered them. 56 So they celebrated the dedication of the altar for eight days, and offered burnt offerings with gladness; they offered a sacrifice of deliverance and praise. 57 They decorated the front of the temple with golden crowns and small shields; they restored the gates and the chambers for the priests, and furnished them with doors. 58 There was very great gladness among the people, and the reproach of the Gentiles was removed. 59 Then Judas and his brothers and all the assembly of Israel determined that every year at that season the days of dedication of the altar should be observed with gladness and joy for eight days, beginning with the twenty-fifth day of the month of Chislev. 60 At that time they fortified Mount Zion with high walls and strong towers round about, to keep the Gentiles from coming and trampling them down as they had done before. 61 And he stationed a garrison there to hold it. He also fortified Beth-zur, so that the people might have a stronghold that faced Idumea.

[EP_L2]
Then they led Jesus from the house of Caiaphas to the praetorium. It was early. They themselves did not enter the praetorium, so that they might not be defiled, but might eat the passover. 29 So Pilate went out to them and said, "What accusation do you bring against this man?" 30 They answered him, "If this man were not an evildoer, we would not have handed him over." 31 Pilate said to them, "Take him yourselves and judge him by your own law." The Jews said to him, "It is not lawful for us to put any man to death." 32 This was to fulfill the word which Jesus had spoken to show by what death he was to die. 33 Pilate entered the praetorium again and called Jesus, and said to him, "Are you the King of the Jews?" 34 Jesus answered, "Do you say this of your own accord, or did others say it to you about me?" 35 Pilate answered, "Am I a Jew? Your own nation and the chief priests have handed you over to me; what have you done?" 36 Jesus answered, "My kingship is not of this world; if my kingship were of this world, my servants would fight, that I might not be handed over to the Jews; but my kingship is not from the world." 37 Pilate said to him, "So you are a king?" Jesus answered, "You say that I am a king. For this I was born, and for this I have come into the world, to bear witness to the truth. Every one who is of the truth hears my voice." 38 Pilate said to him, "What is truth?" After he had said this, he went out to the Jews again, and told them, "I find no crime in him. 39 But you have a custom that I should release one man for you at the Passover; will you have me release for you the King of the Jews?" 40 They cried out again, "Not this man, but Barabbas!" Now Barabbas was a robber.
"""

with open(
    os.path.join(lessons_dir, f"{sample_date}.txt"), "w", encoding="utf-8"
) as f:
  f.write(sample_content.strip())

with open(
    os.path.join(fs_lessons_dir, f"{sample_date}.txt"), "w", encoding="utf-8"
) as f:
  f.write(sample_content.strip())

# 4. Create ZIP package
print("4. Packaging into zip...")
with zipfile.ZipFile(ZIP_PATH, "w", zipfile.ZIP_DEFLATED) as zf:
  for root, dirs, files in os.walk(DATA_DIR):
    for file in files:
      abs_p = os.path.join(root, file)
      rel_p = os.path.relpath(abs_p, DATA_DIR)
      # Store inside daily_office folder
      zf.write(abs_p, arcname=os.path.join("daily_office", rel_p))

zip_sz = os.path.getsize(ZIP_PATH)
print(f"Created {ZIP_PATH} ({zip_sz} bytes, {zip_sz/1024:.1f} KB)")
print("Package contents:")
print("  - psalter.txt (Complete Coverdale Psalter, Psalms 1-150)")
print("  - propers.txt (Full year ordo, collects, appointed readings)")
print("  - lessons/ (Daily lessons in RSV-2CE)")
print("")
print("Installation methods for device:")
print(
    "  1. Built-in: Firmare automatically unpacks psalter.txt and propers.txt"
    " to SD."
)
print(
    "  2. Web Portal: Open http://<device-ip>/files and upload to"
    " /.smudge/daily_office/"
)
print("  3. SD Reader: Copy the 'daily_office' folder into /.smudge/ on SD card")
