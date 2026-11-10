# ECLIPSE – Psychedelic Sampler (VST3 / Standalone)

Premier plugin d'une série d'expérimentations techno psychédélique. Interface noire et rouge, sobre et mate.

## Contenu
- **8 pistes** avec 24 samples synthétisés intégrés (kicks, claps, hats, percus, basses, bleeps psy, FM, risers, glitch…).
- **Grille de pas** jusqu'à 32 pas par piste, avec résolution par piste (1/8, 1/16, 1/16T, 1/32) → polyrythmes / polymètres, swing global.
- **Tes propres samples** : bouton `LOAD` sur chaque piste ou glisser-déposer un fichier (wav, aiff, mp3, flac, ogg) sur la ligne de la piste. Le chemin est sauvegardé avec le projet.
- **Effets par pas (19)** : ratchet x2/x3/x4, reverse, octave +/-, chop, echo throw, pitch aléatoire, probabilité 50 %, tape stop, pitch rise, bitcrush, ring mod, filtre qui ferme/ouvre, distorsion, stutter.
- **FX LANE (15 effets maître)** : une piste de grille en plus pour placer où tu veux delay throw/freeze, reverb swell, balayages de filtre LP/HP, beat repeat 1/16 et 1/32, tape stop, bitcrush, trance gate, ring mod, drive, drop.
- **Outils de grille** : pinceau (effet + vélocité), clic droit = changer l'effet, molette = vélocité, menu par piste (euclidien, aléatoire, décaler, inverser, vider).
- **7 patterns MIDI proposés** (Psy Rolling, Dark Techno, Acid Hypno, Tribal Ritual, Glitch Mind, Hypnotic Minimal, Full-On Night) + **export MIDI**.
- **Delay psychédélique** (sync, wobble tape, ping-pong, smear, drive, tonalité), reverb, filtre maître avec LFO, phaser. Envois delay/reverb par piste.
- **MIDI in** : notes 36–43 (C1–G1) = pistes 1–8 ; autres notes = jeu chromatique sur la piste choisie (« Clavier MIDI »).
- Joue en synchro avec la DAW (position PPQ) ou avec le bouton PLAY interne.

## Compiler
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Le VST3 est généré dans `build/Eclipse_artefacts/Release/VST3/`. JUCE est téléchargé automatiquement.
Linux : `libasound2-dev libfreetype6-dev libfontconfig1-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libgl1-mesa-dev`.
Le workflow GitHub Actions compile aussi Windows/macOS/Linux et publie les VST3 en artefacts.

> Licence : JUCE est sous AGPLv3/commercial. Voir juce.com/juce-licensing avant toute distribution.

Prompts pour la suite : voir [docs/PROMPTS.md](docs/PROMPTS.md).

## Tester
1. Récupère le VST3 : onglet **Actions** du repo → dernier run « Build » → artefact `eclipse-windows-latest` / `eclipse-macos-latest` (ou compile en local, voir plus haut).
2. Copie le dossier `Eclipse Psy Sampler.vst3` dans `C:\Program Files\Common Files\VST3` (Windows) ou `~/Library/Audio/Plug-Ins/VST3` (macOS), puis rescanne les plugins dans ta DAW et charge-le sur une piste instrument. Sans DAW, lance l'exécutable Standalone.
3. Appuie sur **PLAY** (ou lance la lecture de la DAW) : le pattern « Psy Rolling » joue. Teste les autres via le menu « Patterns MIDI proposés ».
4. Samples : clique `LOAD` sur une piste (ou glisse un .wav dessus) → la piste passe sur « [ Mon sample ] ».
5. Effets par pas : choisis un effet dans le pinceau « PAS » (panneau OUTILS DE GRILLE) puis clique/glisse sur la grille ; clic droit sur un pas = effet suivant.
6. FX LANE : choisis un effet dans « LANE », clique des pas sur la ligne FX LANE, écoute pendant la lecture.
7. Sans lecture : joue des notes MIDI C1–G1 (36–43) pour déclencher les pistes.
