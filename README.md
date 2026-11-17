# Unrealemy

Interactieve leerlijnen in de Unreal Editor voor MBO 4 Mediavormgeving. De plugin
`CurriculumCore` voegt aan een ongewijzigde Unreal Engine 5.8 het volgende toe:

- een overzicht van alle leerlijnen en hoofdstukken;
- een stappenpaneel met micro-stappen;
- live controle of de student de stap echt heeft uitgevoerd;
- een rode markering rond het juiste paneel;
- per stap verborgen of getoonde panelen;
- een knop **Reset stap**.

Aan de engine zelf verandert niets.

## Status

| Onderdeel | Status |
|---|---|
| Leerlijnen in JSON, schema, controlescript | Werkt; automatisch getest |
| Validatie, reset, solve (Python-logica) | Werkt; getest met een nagebootste scène |
| Hoofdstuk T1.01 *Eerste stappen* (12 stappen) | Getest met de nagebootste scène, nog niet in de editor |
| Hoofdstuk T1.02 *Ordenen en onderdelen* (11 stappen) | Getest met de nagebootste scène, nog niet in de editor |
| C++-plugin: hub, stappenpaneel, highlight, UI-profielen | **Nooit gecompileerd.** Geschreven tegen UE 5.x, niet gecontroleerd tegen 5.8 |
| `scene_unreal.py` (koppeling met de echte editor) | **Nooit in de editor gedraaid** |
| Tracks 2 t/m 6 | Alleen de tegels in de hub; nog geen hoofdstukken |

## Opbouw

```
Unrealemy.uproject                     project dat de plugin laadt
Config/DefaultEditor.ini               instellingen [Unrealemy]
Plugins/CurriculumCore/
  Curriculum/
    curriculum.json                    volgorde van de leerlijnen
    tracks/<leerlijn>/track.json       titel en hoofdstukken van een leerlijn
    tracks/<leerlijn>/cNN-*.json       één hoofdstuk met zijn stappen
    ui_profiles/*.json                 welke panelen zichtbaar zijn
    schema/curriculum.schema.json      regels voor al deze bestanden
  Content/Python/unrealemy/
    validators.py                      controles per stap
    runner.py                          begin, controle, reset, solve
    scene_unreal.py                    koppeling met de editor
    fake_scene.py                      nagebootste scène voor tests
    chapters/tNN_cNN.py                setup en oplossing per hoofdstuk
  Source/CurriculumCore/               C++: Slate-UI, subsystem, Blueprint-inspectie
Tools/validate_curriculum.py           controle van alle JSON
Tools/tests/                           tests die elk hoofdstuk afspelen
```

## Hoe een stap werkt

1. De student opent een hoofdstuk. De plugin opent een kopie van de startmap en draait
   `setup`. Daarna speelt hij de `solve` van alle eerdere stappen af. Zo staat de scène
   precies in de beginstaat van de stap.
2. Het UI-profiel van de stap verbergt of toont panelen. De markering wijst het juiste
   paneel aan.
3. Vier keer per seconde controleert Python de scène met het `validate`-blok. De
   student ziet direct feedback, bv. *"Kubus staat op X 0, Y 0, Z 0. Doel: X 0, Y 0, Z 50."*
4. Klopt het, dan springt de stap op **Voltooid**. Na 1,5 seconde opent de volgende stap.
5. **Reset stap** bewaart eerst het level in `/Game/StudentWork/<gebruiker>/Backups/`
   (de laatste 10) en bouwt daarna de beginstaat opnieuw op.

De voortgang staat per Windows-gebruiker in `%LOCALAPPDATA%\Unrealemy\progress.json`.

## Een hoofdstuk toevoegen

1. Maak `Curriculum/tracks/<leerlijn>/cNN-naam.json` en zet de bestandsnaam in `track.json`.
2. Maak `Content/Python/unrealemy/chapters/tN_cNN.py` met `setup(scene)` en per stap
   een `solve_sNN(scene)`. Die doet wat de student moet doen.
3. Controleer alles:

   ```bash
   pip install jsonschema
   ```

   ```bash
   python Tools/validate_curriculum.py
   ```

   ```bash
   python -m unittest discover -s Tools/tests
   ```

De tests weigeren een hoofdstuk in deze gevallen:

- een stap is al voltooid voordat de student iets doet;
- een stap is na zijn `solve` nog niet voltooid;
- **Reset stap** levert een andere beginstaat op.

GitHub Actions draait dezelfde controles bij elke pull request.

Docentmodus (`UNREALEMY_TEACHER=1` of `bTeacherMode=True`) geeft twee extra knoppen:

- **Oplossing**: voert de stap automatisch uit;
- **Herlaad**: leest JSON en Python opnieuw in zonder de editor te herstarten.

## Validatortypen

| Type | Controleert |
|---|---|
| `actor_exists` | Er staat minstens N keer een actor van deze klasse of met dit label |
| `transform_near` | Location, Rotation en/of Scale binnen een marge |
| `actor_moved` | Actor minstens X cm verplaatst sinds het begin van de stap |
| `selection_is` | Precies deze actor is geselecteerd |
| `property_compare` | Eigenschap vergelijken, bv. `point_light_component.attenuation_radius == 500`; met `component` op een component |
| `actor_in_folder` | Actor staat in deze map van de Outliner |
| `attached_to` | Actor is een child van een andere actor |
| `component_exists` | Actor heeft een component van deze klasse, bv. `PointLightComponent` |
| `viewport_camera_moved` / `_rotated` | De student heeft gevlogen of rondgekeken |
| `viewport_looks_at` | De camera kijkt van dichtbij naar een actor (toets F) |
| `material_input_from` | Er hangt een bepaalde node aan een materiaalingang, bv. een ScalarParameter aan Roughness |
| `blueprint_exec_linked` | Een event is via exec-lijnen verbonden met een functie, bv. BeginPlay met PrintString |

Een actor kies je op `label`, `class` en/of `mesh` (asset-pad). `mesh` blijft werken als de student
de actor hernoemt. Combineer checks met `{"all": [...]}` of `{"any": [...]}`. Een `message` per check
vervangt de standaardmelding. Een nieuw type schrijf je in `validators.py` en voeg je
toe aan het schema.

## Starten (Windows)

Vereist: UE 5.8 en Visual Studio 2022 met de workload *Game development with C++*.

1. Rechtsklik op `Unrealemy.uproject` en kies *Generate Visual Studio project files*.
2. Open het project. Unreal vraagt de plugin te compileren; kies *Yes*.
3. De editor opent met het overzicht van de leerlijnen.

Een snelkoppeling met `UnrealEditor.exe "<pad>\Unrealemy.uproject"` slaat de Project Browser over.

## Nog te controleren in de editor

Deze punten zijn geschreven op basis van kennis van UE 5.x en nog niet gecontroleerd in 5.8.

**Compileren**

- Kunnen API-namen afwijken?
  - `FButtonStyle::Set*Foreground`
  - `FSlateRoundedBoxBrush`
  - `SDockTab::GetContent`
  - `FTabManager::HasTabSpawner`
  - `FGeometry::ToPaintGeometry(Size, Transform)`
  - `IPythonScriptPlugin::ExecPythonCommandEx`
  - `UK2Node_Event::EventReference`
- Bestaat de werkbalk-extensie `LevelEditor.LevelEditorToolBar.User` nog?

**Tab-ID's**

- Kloppen de ID's van de panelen?
  - `LevelEditorViewport`
  - `LevelEditorSceneOutliner`
  - `LevelEditorSelectionDetails`
  - `PlacementBrowser`
  - `ContentBrowserTab1`
- Controleer ze met *Tools → Debug → Widget Reflector*.

**Startmap**

- Bestaat `/Engine/Maps/Templates/Template_Default` in 5.8?
  `scene_unreal.py` valt anders terug op een eigen lege map.

**Python**

- Geeft `get_level_viewport_camera_info()` een tuple terug, of `None` zonder actieve viewport?
- Werken `EditorLoadingAndSavingUtils.new_map_from_template` en `save_map` zoals verwacht?
- Hoofdstuk T1.02: werken `get_folder_path` / `set_folder_path`, `get_attach_parent_actor` /
  `attach_to_actor` en vooral `SubobjectDataSubsystem.add_new_subobject` (component toevoegen
  via Python, nodig voor **Reset stap** vanaf stap 9)?
- Bestaat *Move To → Create New Folder* nog in het rechtsklikmenu van de Outliner (T1.02 stap 3)?

**Reset**

- Opent `new_map_from_template(..., False)` een kopie **zonder** vraagvenster
  "wijzigingen opslaan?" als het level gewijzigd is? Als het venster toch verschijnt,
  moet de back-up eerst de wijzigingen opslaan.

**Overige punten**

- Material Editor: de wijzigingen zijn pas zichtbaar na *Apply*. De stap moet dat
  expliciet vragen.
- Eerste start: het overzicht opent als zwevend venster. Docken onthoudt Unreal per gebruiker.

## Juridisch

Dit project bevat geen code van Epic. De broncode van Unreal Engine hoort niet in deze
repository: zie `.gitignore`. Distributie van de editor aan studenten valt onder de
Unreal Engine EULA. Laat dit toetsen voordat er een installer naar eigen laptops gaat.
