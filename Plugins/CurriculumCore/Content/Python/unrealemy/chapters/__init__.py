"""Setup- en solve-functies per hoofdstuk.

Bestandsnaam ``t<track>_c<hoofdstuk>.py``. Elke functie krijgt één argument:
de scène (``unrealemy.scene.Scene``). Gebruik alleen methoden van die
interface, dan werken de automatische tests ook zonder Unreal.

* ``setup(scene)`` bouwt de beginstaat na het laden van de startmap.
* ``solve_sNN(scene)`` voert stap NN precies zo uit als een student zou doen.
  De reset-knop speelt deze functies af om de beginstaat van een stap te maken.
"""
