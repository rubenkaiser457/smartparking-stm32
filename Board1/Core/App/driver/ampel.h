#ifndef AMPEL_H
#define AMPEL_H

/*
 * Ampel-LED fuer Board1 (Einfahrt):  gruen = Platz frei,  rot = voll.
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

/** @brief Gruen an, rot aus  (Platz frei). */
void ampel_gruen(void);

/** @brief Rot an, gruen aus  (voll / Stand unbekannt). */
void ampel_rot(void);

/** @brief Beide LEDs aus. */
void ampel_aus(void);

#endif /* AMPEL_H */
