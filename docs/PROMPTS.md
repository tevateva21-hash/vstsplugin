# Prompts prêts à l'emploi

Contexte à coller au début de chaque prompt :
> Projet JUCE 8 / CMake (repo vstsplugin). Plugin « Eclipse » : sampler 8 pistes, grille de pas, effets par pas, delay psychédélique. Style UI : noir (#09090a) et rouge mat (#b3121b), aucun dégradé ni brillance, polices sobres. Code audio sans allocation ni verrou dans processBlock. Garde le code existant cohérent (APVTS, std::atomic pour la grille).

## Sampler (évolutions)
1. « Ajoute le chargement de samples utilisateur (drag & drop wav/aiff) par piste, avec fallback sur la banque intégrée, sauvegardés dans l'état du plugin. »
2. « Ajoute une enveloppe AHDSR et un filtre par piste, ainsi qu'un mode choke group (hats ouverts coupés par fermés). »
3. « Ajoute un mode de pas conditionnels (1:2, 3:4, fill) et des locks de paramètres par pas (pitch, pan, envoi delay). »
4. « Ajoute un système de scènes/patterns A–H avec enchaînement automatique et changement via notes MIDI. »
5. « Ajoute un MIDI drag & drop du pattern vers la DAW et l'import d'un fichier .mid sur la grille. »

## Effets psychédéliques
6. « Ajoute un delay shimmer (pitch shift +12 dans le feedback) et un mode freeze/infinite. »
7. « Ajoute un effet reverse reverb et un granulaire (tail freeze) sur le bus reverb. »
8. « Ajoute une section modulation : 2 LFO et une enveloppe assignables par glisser-déposer à n'importe quel bouton. »
9. « Ajoute un bitcrusher, un ring modulator et un tape-stop déclenchable depuis un pas. »
10. « Ajoute un sidechain interne (pompage du bus delay/reverb par le kick). »

## Autres plugins de la série
11. « Crée un synthé acid/psytrance (VST3, JUCE) : 2 oscillateurs, filtre ladder résonnant, séquenceur 303-style avec slides/accents, même charte noir/rouge mat. »
12. « Crée un effet multi-FX psychédélique (phaser, flanger, delay, reverb, filtre) avec enchaînement réorganisable et macro unique. »
13. « Crée un générateur de basses rolling psy (MIDI → audio) avec octave pattern et sidechain intégré. »
14. « Crée un utilitaire de génération de patterns MIDI (euclidien, polyrythmes, probabilités) qui sort du MIDI vers d'autres instruments. »

## UI
15. « Ajoute le redimensionnement de l'interface (50–150 %) sans perdre la netteté, et un vu-mètre rouge discret. »
