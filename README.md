# ECLIPSE – Psychedelic Sampler (VST3 / Standalone)

Premier plugin d'une série d'expérimentations techno psychédélique. Interface noire et rouge, sobre et mate.

## Contenu
- **8 pistes** avec 24 samples synthétisés intégrés (kicks, claps, hats, percus, basses, bleeps psy, FM, risers, glitch…).
- **Grille de pas** jusqu'à 32 pas par piste, avec résolution par piste (1/8, 1/16, 1/16T, 1/32) → polyrythmes / polymètres, swing global.
- **Effets par pas** : ratchet x2/x3/x4, reverse, octave +/-, chop, echo throw (envoi max dans le delay), pitch aléatoire, probabilité 50 %.
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
