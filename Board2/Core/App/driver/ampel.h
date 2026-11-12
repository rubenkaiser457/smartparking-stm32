#ifndef AMPEL_H
#define AMPEL_H

/*
 * Ampel-LED fuer Board2 (Ausfahrt) -- Tor-Signal:
 *   rot   = Schranke zu / warten (Ruhezustand)
 *   gruen = Schranke offen / durchfahren (waehrend ein Auto rausfaehrt)
 * (Anders als Board1 ohne Platz-Check -- die Ausfahrt ist immer erlaubt.)
 *
 * Externe LEDs (Zio-Header CN10):
 *   GRUEN = D4 (PF14)
 *   ROT   = D2 (PF15)
 * (D7/D8 sind vom HC-SR04 belegt.)
 * Jede LED mit ~220 Ohm Vorwiderstand nach GND (siehe ampel.c).
 * Pin-Zuordnung bei Bedarf in ampel.c anpassen.
 */

/** @brief LED-Pins als Ausgang konfigurieren (startet: beide aus). */
void ampel_init(void);

/** @brief Gruen an, rot aus  (Schranke offen / durchfahren). */
void ampel_gruen(void);

/** @brief Rot an, gruen aus  (Schranke zu / warten). */
void ampel_rot(void);

/** @brief Beide LEDs aus. */
void ampel_aus(void);

#endif /* AMPEL_H */
