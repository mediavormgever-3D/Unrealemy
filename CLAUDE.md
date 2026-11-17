# Unrealemy: context voor Claude

Interactieve leerlijnen in de Unreal Editor voor MBO 4 Mediavormgeving (vakgroep 3D),
naar het model van de eerdere Blender- (Axionomy) en ComfyUI-pakketten. Uitleg van de
werking en de opbouw: `README.md`.

## Vastgelegde beslissingen (door de gebruiker bevestigd, 16-11-2026)

- **Alleen de plugin.** `Plugins/CurriculumCore` draait op een ongewijzigde UE 5.8. Geen
  engine-fork, geen engine-patches, geen Installed Build of installer in deze fase.
  Aanleiding: elke engine-patch kost een rebuild van uren en moet bij elke engine-update
  opnieuw.
- **Geen Epic-code in deze repo.** `source/` staat in `.gitignore`. Verspreiding van de editor
  aan studenten valt onder de Unreal Engine EULA en moet nog juridisch getoetst worden.
- **Reset zonder kaart per stap.** Een reset opent een kopie van de startmap van het
  hoofdstuk, draait `setup` en speelt de `solve` van alle eerdere stappen af. Er is dus geen
  `.umap` per stap.
- **Validatie in Python, UI in C++.** Python (`unrealemy`) bevat de controles; die zijn te
  testen zonder Unreal. C++ doet de Slate-UI, de editor-subsystem en wat Python niet kan
  (Blueprint-grafen lezen). Controle via polling (4×/s), omdat de viewport-camera geen
  editor-events geeft.

## Regels bij elke wijziging

- Elke stap heeft een `solve`. De tests eisen drie dingen:
  - een stap is vooraf niet voltooid;
  - een stap is na `solve` wel voltooid;
  - een reset levert de beginstaat opnieuw op.
- Een nieuw validatortype: functie in `validators.py` **en** het type in
  `Curriculum/schema/curriculum.schema.json`. Het controlescript bewaakt dat die twee
  overeenkomen.
- Teksten voor studenten in het Nederlands, op B1-niveau, met één handeling per stap. Het
  schema begrenst titel (40) en instructie (200 tekens). Meldingen zijn concreet ("Kubus
  staat op X 0, Y 0, Z 0. Doel: …"), zonder `==` of klassennamen als dat te vermijden is.
- Een stap herkent een actor bij voorkeur aan `mesh` + `label`, zodat hernoemen niets breekt.
- Draai voor elke commit:

  ```bash
  python Tools/validate_curriculum.py
  ```

  ```bash
  python -m unittest discover -s Tools/tests
  ```

## Wat nog nooit getest is

Deze container is Linux zonder Unreal Engine. Daarom zijn twee onderdelen nooit in Unreal
gedraaid:

- de C++-code: nooit gecompileerd;
- `scene_unreal.py`: nooit in de editor uitgevoerd.

De lijst met aannames over UE 5.8-API's staat in de README onder "Nog te controleren in de
editor". Wie Unreal-gedrag beweert zonder het te hebben kunnen testen, zegt dat erbij.

## Stand van zaken

- Branch `claude/dazzling-cori-tzqvvc`, twee commits:
  - de plugin;
  - hoofdstuk T1.02.
- Op 17-11-2026 kon de cloudsessie niet pushen (GitHub gaf 404). De gebruiker pusht de branch
  zelf vanaf een git-bundel.
- Klaar:
  - T1.01 *Eerste stappen* (12 stappen);
  - T1.02 *Ordenen en onderdelen* (11 stappen).
- Tracks 2–6 bestaan alleen als tegel in de hub.

## Volgende stappen

1. Draft pull request voor `claude/dazzling-cori-tzqvvc` openen, als die er nog niet is.
2. T1.03 *Licht en sfeer*: Directional Light, Sky Light en de belichting van een scène.
   Daarmee is Track 1 af.
3. Eerste build op een Windows-pc met UE 5.8 en Visual Studio 2022. De compileerfouten van
   de gebruiker oplossen en daarna de punten uit "Nog te controleren" afwerken.
