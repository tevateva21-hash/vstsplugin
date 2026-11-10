# PsyLab — Suite de 3 plugins VST3 / Standalone (noir & rouge mat)

Projet JUCE 8 / CMake produisant trois plugins dans la même charte graphique (noir `#09090a`, rouge mat `#b3121b`, sans dégradé).

---

## 1. ECLIPSE — Psychedelic Sampler (`Eclipse Psy Sampler.vst3`)
- **8 pistes**, 24 samples synthétisés intégrés + **chargement de tes propres samples** (`LOAD` ou glisser-déposer wav/aiff/mp3/flac/ogg par piste, sauvegardé dans le projet).
- **Grille jusqu'à 32 pas** par piste, résolution indépendante (1/8, 1/16, 1/16T, 1/32), swing, outils euclidiens / aléatoire / décalage / inversion.
- **19 effets par pas** : ratchet ×2/×3/×4, reverse, octave +/−, chop, echo throw, pitch aléatoire, proba 50 %, tape stop, pitch rise, bitcrush, ring mod, filtre qui ferme/ouvre, distorsion, stutter.
- **FX LANE (14 effets maître sur la grille)** : delay throw, delay freeze, reverb swell, filtres LP/HP, beat repeat 1/16 & 1/32, tape stop, bitcrush, trance gate, ring mod, drive, drop.
- **7 patterns MIDI proposés** + export `.mid` + entrée MIDI (notes 36–43 = pistes 1–8, autres notes = jeu chromatique).
- **Delay psychédélique** (sync, wobble, ping-pong, smear, drive, tone), reverb, phaser, filtre maître + LFO.

## 2. HELIOS — Psychedelic Guitar Synthesizer (`Helios Guitar.vst3`)
Instrument MIDI inspiré de l'univers Pink Floyd / Gilmour (aucune entrée audio requise, jouable au clavier MIDI ou au clavier intégré) :
- **Moteur de cordes Karplus-Strong** polyphonique 12 voix ou **Mono Legato** (slides/hammer-on) : position et dureté du médiator, bruit d'attaque, résonance de caisse, brillance, sustain jusqu'au **sustain infini**, doublage **12 cordes**, largeur stéréo, vibrato (LFO + molette de modulation), pitch bend, glide, **accords automatiques** (Power, Majeur, Mineur, Sus2/4, Add9, Maj7, Min7, Min9, Open) avec **strum** réglable (vitesse, haut/bas/alterné).
- **Pédalier complet** : Compresseur sustain → Octaver (−1 / +1) → Fuzz / Overdrive / **Big Muff** / Boost / Distortion → **Wah** (pédale, auto-wah enveloppe, LFO) → **Ampli & baffle** (EQ 4 bandes + 1×12, 2×12, 4×12) → **Tremolo** (bias / harmonique) → **Phaser** → **Chorus / Flanger / Vibrato / Leslie rotatif** (lent/rapide avec inertie) → **Tape / Binson Delay** → **Reverb** (Spring, Plate, Hall, **Shimmer +1 octave**).
- **18 presets d'usine** classés (*Comfortably Numb Lead, Shine On Clean, Echoes Pompeii, Run Like Hell Dotted 8th, Breathe Uni-Vibe, Any Colour Leslie, Time Fuzz Solo, Dogs Harmony, Sorrow Heavy Wall, Money Funk Wah, Wish You Were 12-String, Dark Side Shimmer Pad…*) + sauvegarde de tes presets + bouton `DICE`.

## 3. PRISM — Psychedelic Filter Sampler (`Prism Filter.vst3`)
Sampler filtrant monophonique conçu pour sculpter un sample avec des filtres et des modulations :
- **Source** : **glisser-déposer / `LOAD SAMPLE`** (wav, aiff, mp3, flac, ogg) ou 12 textures/boucles synthétisées intégrées. Modes **Boucle libre**, **Clavier MIDI (retrig)** ou **Clavier MIDI (legato)** avec suivi de hauteur, glide, boucle avant / aller-retour / one-shot, lecture inverse, début/fin déplaçables sur la forme d'onde.
- **Moteur granulaire** : taille, densité, jitter de position et de pitch, largeur stéréo.
- **2 filtres multi-modes** en **Série** ou **Parallèle** (avec pan et balance) : LP 12/24, HP 12/24, BP 12/24, Notch, **Ladder 24 dB acide**, **Comb +/−** résonnant, **Formant voyelles (A-E-I-O-U)**, chacun avec cutoff, résonance, drive, suivi de clavier et morphing, + **courbe de réponse en temps réel**.
- **Modulations** : 2 enveloppes ADSR (ampli + filtre), **2 LFO** synchronisables (7 formes dont S&H et aléatoire lisse → F1, F2, résonance, pan, pitch), **séquenceur de filtre jusqu'à 32 pas** dessinable à la souris (menu de formes, lissage, swing, envoi vers F1, F2, trance gate et pitch).
- **Effets de fin de chaîne** : bitcrusher, ring modulator, phaser, delay psychédélique, reverb.
- **16 presets d'usine** + sauvegarde utilisateur + `DICE`.

---

## Récupérer et tester les 3 plugins
1. Sur GitHub, ouvre l'onglet **Actions** → dernier run **Build** → télécharge l'artefact `psylab-windows-latest` ou `psylab-macos-latest`.
2. Il contient les 3 dossiers `.vst3` (`Eclipse Psy Sampler.vst3`, `Helios Guitar.vst3`, `Prism Filter.vst3`) ainsi que les exécutables **Standalone**.
3. Copie les `.vst3` dans `C:\Program Files\Common Files\VST3` (Windows) ou `~/Library/Audio/Plug-Ins/VST3` (macOS), rescanne dans ta DAW et charge-les sur des pistes instrument.

## Compiler en local
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
```
