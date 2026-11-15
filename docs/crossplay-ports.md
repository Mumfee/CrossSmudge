# CrossPlay Lua ports — review scope

**Incomplete collection: 18 local adaptations, 4 unchanged existing equivalents, 11 titles without a package (33 total).** No claim of complete original feature/UI/engine parity. No live publication has occurred.

| Original title | Package/status | Remaining parity or blocker |
|---|---|---|
| CHESS | chess | Existing apps/chess is a substitute already authorized; it does not establish original engine/search/UI or nearby parity. Nearby blocked. |
| BATTLESHIP | cp-battleship | Solo only. Computer is a smaller public-observation hunt/target strategy, not a claim of original gunner parity. Nearby, original art/statistics and named sunk-ship announcements absent. Restart abandons placement only after confirmation. |
| CONNECTIONS | Not implemented | No HTTP for pack updates. Existing archive needs actual data and its own redistribution terms, not invented questions. |
| SOLITAIRE | solitaire | apps/solitaire exists; matching title/core is not proof of all source settings and UI parity. |
| HEARTS | cp-hearts | Opponent is a new explicit public-information risk heuristic, not the original HeartsBrain. Opponent turns advance with a labeled action. Card list UI replaces original artwork; lifetime statistics absent. No hidden-hand peeking. |
| D&DIAGRAMS | Not implemented | Upstream docs/apps/dungeon.md:21-25 identifies 64 handmade Zachtronics Last Call BBS puzzles plus tutorial and explicitly says not to publish as a product. No bank copied. |
| INSIDER | cp-insider | Original artwork and ornamental record display are absent. English deck only. Timer resumes paused after closing; no wall-clock time is charged while closed. |
| JAIPUR | cp-jaipur | New public-information greedy opponent, not JaipurBrain. Solo only; no Nearby, original artwork or lifetime records. Exchange UI selects market slots and cycles offered goods/camels; token inspector lists remaining payouts. |
| SEA SALT | cp-seasalt | New own-hand/public-card heuristic, not SeaSaltBrain. Solo only; no Nearby or original art. Duo selection is two successive card choices to bound memory. Card inspector protects hidden opponent cards. Original labels retained; colour is written in text. |
| MURDLE | cp-murdle | Finite host-generated case set replaces unlimited on-device generation; selecting next case cycles its 16-case tier. No daily system, original art or record charts. No unrelated commercial puzzle bank was copied. An opponent is not applicable. |
| CHECKERS | cp-checkers | Nearby and original artwork/statistics absent. Full legal move generation has no original 48-move list truncation. Pieces use o/x and capital kings; selecting a final square chooses the first legal chain to that square. |
| CONNECT FOUR | cp-connectfour | Nearby unavailable. Rules and deterministic game-search AI fit stock Lua in principle. |
| YAHTZEE | cp-yahtzee | Nearby unavailable; scoring requires all original bonus/Joker logic. Dice Roller is not equivalent. |
| KNUCKLEBONES | cp-knucklebones | Nearby unavailable; local core and actual opponent scoring/search can port. |
| MINESWEEPER | cp-minesweeper | No fundamental blocker identified. |
| SUDOKU | sudoku | apps/sudoku exists; original settings/UI parity not established by name. |
| PICROSS | Not implemented | Original bank explicitly says not authorized to republish or offer downloadable pack. Code MIT does not license bank. |
| TOY BATTLE | cp-toybattle | New public-board/rack-size heuristic, not ToyBattleBrain; it does not plan Capn chains or special-base effects. Human can configure all effects and chains with the placement builder (0 declines, 1 uses; zero target means none). Solo only, no Nearby or original art. Map inspector gives numbered adjacency/regions/effects; this is a text board, not graphical parity. |
| FOREHEAD | cp-forehead | Original artwork and graphical record charts are absent. List-based result review. English only. Closed time is paused. Category selection replaces the last completed result display; persisted records remain. |
| TRIVIA | Not implemented | No HTTP and no byte-offset reads for original random-access pack. Prefix-reading entire pack exceeds Sticky 2 MiB. |
| WAVELENGTH | cp-wavelength | Original art and lifetime histogram/statistics absent. Team mode keeps the original shared session scoring formula, not invented separate team totals. Hidden target reopens behind a reveal gate. English only. |
| GO | cp-go | NOT full-history superko: matches the source bounded window and move-limit house rule. Optional 9x9 one-ply capture/liberty opponent, not Michi/search/difficulty parity. Human adjudicates dead groups and both score acceptances, including solo. No Nearby, original art, automatic life/death, resign button or original statistics. Coordinate-row UI replaces a graphical board; handicap UI is 9x9 only although core supports all three sizes. |
| HEX | cp-hex | Opponent is a smaller deterministic shortest-path strategy, NOT original RAVE Monte Carlo, bridge policy, tree reuse or difficulty levels. Board is a square coordinate grid; documented diagonal adjacency is upper-right/lower-left. Nearby and original art/statistics absent. |
| UNDERHAND | Not implemented | Data is original APK content used with Spoopy Squad permission dated 2026-09-24; header alone does not establish permission extends to this redistribution. Engine translation feasible. |
| WORDLE | wordle | apps/wordle is existing equivalent core; original archive/current-day network fetching not supplied by stock Lua API. |
| STUDY | Not implemented | No HTTPS/bridge transport. FSRS itself is numerical and portable; do not falsely call FSRS impossible. Existing prepared deck offset-reading/media need conversion or host facilities. |
| HACKER NEWS | Not implemented | No HTTP/TLS/network API. Offline snapshots would not be live app parity. |
| XKCD | Not implemented | No HTTP, original archive random reads unsupported by read_file; raw sprite API is not PNG decoder. Real artwork has separate licensing. |
| INSTAPAPER | Not implemented | No HTTPS/bridge transport. Do not insert keys or fabricate login; bridge handles account integration. Offline article list is not sync parity. |
| WALLPAPERS | Not implemented | Stock Lua has no wallpaper/sleep-setting hook or live sharing network. Arbitrary file writes must not be used to mutate global firmware state. |
| WIKIPEDIA | Not implemented | No byte-offset reads or zstd dictionary decoder; prefix-loading original multi-shard archive cannot fit. No firmware reader integration API. |
| CALCULATOR | cp-calculator | Binary Lua floating-point alone does not preserve exact decimal cancellation; decNumber native module unavailable. Decimal algorithm can be implemented in Lua; not fundamentally blocked. |
| NOTES | cp-notes | No phone network server API or sleep-screen note hook. Local card/checklist editor is portable but only subset. |

## Reproduction

Run `python3 test/crossplay_ports/run.py`, then `additions_run.py`, `checkers_review_run.py`, and `package.py` in that same directory (full paths from the repository root). Existing source-oracle suites remain in `test/crossplay_ports/`; see the supplied handoff for all six-port differential commands. The native compiler builds the vendored release Lua VM; no PlatformIO or firmware worktree is used. `additions-results.json` and archive checksums live in the owner’s external evidence directory.

All original thirteen port directories remain byte-for-byte unchanged against the continuation-start SHA256 manifest (in addition to the original-six ZIP checks). Four upstream equivalents retain their IDs and files. New translations retain CrossPlay MIT notices. No Picross, D&Diagrams or Underhand protected banks/assets were copied.

## Publication and install boundary

Catalog entries describe local adaptations, not full CrossPlay ports. Every listed file must install at `/.crosssmudge/applications/<id>/<file>`. Individual ZIPs contain one `<id>/` root and can use the stock web uploader. The eighteen-new-port ZIP and the full 22-title feasible-local ZIP contain `.crosssmudge/applications/` and is SD-only. Do not overwrite an existing app directory or its saves.

The fork source URL prefix is `https://raw.githubusercontent.com/tobiasweissert/CrossSmudge/feat/crossplay-lua-apps/apps/`. Generated URL/hash manifests describe the prepared local files; remote byte readback is pending the parent’s commit/push. Stock `AppStoreActivity.h:32,717` hardcodes `Mumfee/CrossSmudge/main/apps/`; catalog repo/branch metadata does not redirect it. Fork publication does not make these apps available in the original store. Maintainer merge of the upstream PR is required.

## Exact missing host interfaces

- Nearby games: permission-controlled peer discovery/session pairing and bounded ESP-NOW message send/receive callbacks with cancellation and disconnect handling. Stock Lua exposes none.
- Connections, Trivia, Hacker News, XKCD: cancellable bounded HTTPS requests/download-to-app-file, certificate validation, status/error reporting. Original indexed packs also need app-relative seek/read-at-offset. Artwork/content rights remain separate.
- Study / Instapaper: the above HTTPS transport plus authorized bridge pairing/token storage, request headers/body and incremental sync; no account credentials are bundled. FSRS itself remains portable implementation work.
- Wikipedia: bounded offset reads, streaming dictionary-zstd decompression and a supported reader handoff. Entire archive prefix loading is not viable.
- Wallpapers / Notes integration: explicit host wallpaper/note sleep-screen selection APIs, safe import transactions, and secure transfer callbacks; global settings writes are not a substitute.
- Go’s original Michi opponent: a host engine interface or a substantive validated Lua engine translation. Native-code loading is absent; Go rules themselves remain portable.

## Hardware verification

No physical device, production renderer or simulator/HAL test was performed. On Sticky v1.6.5.3, install only new IDs; exercise buttons/taps, both orientations, cancel/new/save/reopen and sleep/wake for each package. Follow each PARITY.md’s rule-specific checklist. Inspect internal free/largest heap, PSRAM and serial Application Error/OOM logs. The adapter cap excludes native C/HAL allocations. There are no screenshots offered as visual evidence and no EPUB cache reset is needed.

## Local delivery boundary

Jaipur, Sea Salt, Toy Battle, Murdle and Go now have real local implementations, saved UI flows and scoped catalog entries. The full feasible-local ZIP includes eighteen adaptations and four unchanged optional equivalents. This is **22 of 33 titles**, not complete CrossPlay parity. The eleven unimplemented titles and their exact runtime/content/permission gaps remain in the table above. No placeholder packages exist for them.

Murdle intentionally supplies 64 cases (16 per tier) generated and uniqueness/deduction-checked by the original C++ source, rather than an on-device generator. The other four additions use explicitly simpler Lua heuristics; Go's optional opponent is 9x9 and requires human dead-stone adjudication. Go preserves the original eight-position situational repetition window and move-limit house rule, not full-history superko. Toy Battle exposes nine playable maps; its tenth test terrain remains hidden.

Run the new suites sequentially: `python3 test/crossplay_ports/five_run.py`, `python3 test/crossplay_ports/five_review_run.py`, `python3 test/crossplay_ports/catalog_review.py`, then `python3 test/crossplay_ports/package.py`. Rebuild source-derived data only with `generate_murdle.py` / `generate_toy.py`; both use the supplied original source checkout and isolated `/tmp` executables. No shared firmware/compiler cache or PlatformIO build is used.

The five-port runtime suite exercises 76,800-byte and 2,097,152-byte allocator caps, portrait and landscape, complete games/cases, torn journals, corrupted state types, late-round Sea Salt UI capacity, Go rules and Toy Battle effects/chains. Full-game source-oracle traces run at 2 MiB because their comparison instrumentation retains extra data; independent smaller full-game regressions run at 75 KiB. These tests do not establish production HAL or physical-device headroom. Preserve that distinction in publication claims.

## Final local evidence

The five additions pass 100 adapter cases across both caps/orientations. Forty-five complete source-core comparison games match 4,683 actions (including Jaipur exchanges); Murdle's 64 cases pass source uniqueness/deduction gates and local accusation checks. The original suites pass 111 + 104 cases, and all 216 original-thirteen source files remain identical. Catalog-only installs verify 343 files across eighteen entries. Twenty-four indexed ZIPs pass payload readback, with forty extracted-byte rule/launch cases.

Five-port 75 KiB suite peaks: Jaipur 74,357; Sea Salt 76,796; Toy Battle 76,789; Murdle 68,472; Go 69,840 bytes. Sea Salt and Toy Battle are exceptionally close to the cap; these measurements exclude C/HAL allocations and do not establish physical-device safety margins. Follow the hardware checklist before claiming hardware validation.
