#pragma once

// Patterns MIDI/grille proposés. 8 pistes, chaque piste = une chaîne de pas.
// Légende : . vide | x normal | X accent | o fantôme | 2/3/4 ratchet x2/x3/x4
//           v reverse | u octave haut | d octave bas | c chop | t "throw" (écho max)
//           ? pitch aléatoire | p probabilité 50%
namespace Eclipse
{
struct PatternPreset
{
    const char* name;
    int bpm;
    int samples[8];
    const char* rows[8];
};

// indices : 0 K Rolling 1 K Deep 2 K Hard 3 Clap 4 Snare 5 Rim 6 HatC 7 HatO 8 HatShim 9 Tom 10 Conga
// 11 Metal 12 Zap 13 BassRoll 14 BassAcid 15 SubDrop 16 Bleep 17 FMBell 18 Alien 19 Riser 20 Pad 21 Glitch 22 SweepDn 23 Arp
inline const PatternPreset* patternPresets (int& count)
{
    static const PatternPreset p[] = {
        { "Psy Rolling 145", 145, { 0, 3, 6, 7, 10, 13, 16, 22 },
          { "x...x...x...x...", "........X.......", "o.o.o.o.o.o.o.o.", "..x...x...x...x.",
            "..x..x..x.x..x.x", ".xxx.xxx.xxx.xxx", "..o...x...o..x.2", "...............v" } },
        { "Dark Techno 138", 138, { 2, 3, 6, 7, 9, 14, 16, 19 },
          { "x...x...x...x...", "....x.......x...", "x.o.x.o.x.o.x.o.", "..x...x...x...x.",
            "..x..x..x..x..x.", "..x...x...x...x.", ".......x.......2", "p...............", } },
        { "Acid Hypno 130", 130, { 0, 4, 6, 7, 12, 14, 18, 21 },
          { "x...x...x...x...", "....x.......x..2", "xoxoxoxoxoxoxoxo", "..x...x...x...x.",
            "........t.......", "x.xx.xx.xx.xxx.x", "x.......u.......", "....p...p...p..3" } },
        { "Tribal Ritual (polymètre)", 136, { 1, 10, 9, 5, 6, 15, 11, 17 },
          { "x.....x...x.....", "x.xx.x.xx.x.", "..x...x..x", "x..x..x.",
            "x.xxx.xx", "x...............", "......t.........", "..........u....." } },
        { "Glitch Mind 150", 150, { 0, 3, 6, 21, 12, 13, 23, 22 },
          { "x...x..xx...x...", "....X.......X.3.", "x.x4x.xx2.x.x3xo", "p.?.p.?.p.?.p.?c",
            "..t...v...t...v.", ".xxx.xxx.xxx.x4x", "?.?.?.?.?.?.?.?.", "v.......v......." } },
        { "Hypnotic Minimal 128", 128, { 1, 5, 6, 8, 10, 15, 17, 20 },
          { "x...x...x...x...", "........o.......", "..x...x...x...x.", "......t.......t.",
            "x..x..x..x..x..x", "x.......x.......", "........u.......", "x...............", } },
        { "Full-On Night 148", 148, { 0, 3, 6, 7, 16, 13, 23, 19 },
          { "x...x...x...x...", "....x.......x...", "o.o.o.o.o.o.o.o.", "..x...x...x...x.",
            "x.x.x.x.x.x.x.xX", ".xxx.xxx.xxx.xxx", "x.xx.x.xx.xx.x.x", "...............c" } },
    };
    count = (int) (sizeof (p) / sizeof (p[0]));
    return p;
}

// FX lane par preset (16 pas, 1/16). Lettres : . T F W / \\ H R r < B G M D X (voir laneChars)
inline const char* presetLanes (int i)
{
    static const char* l[] = {
        "..............T.", "............R...", "..T.....G.......", "..........F...<.",
        "......r...R...<B", "...............W", "...........T..R/" };
    return l[i % 7];
}
}
