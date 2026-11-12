#ifndef SEG7_H
#define SEG7_H

/*
 * Treiber fuer ein EINZELNES 7-Segment-Display (1 Ziffer), Typ 5161AS
 * (Common Cathode). Zeigt die freie Platzzahl 0..9 auf Board1 an.
 *
 * KEIN Multiplexing, KEIN Treiber-Chip: der STM32 schaltet die 7 Segmente
 * direkt. Jedes Segment = ein GPIO-Ausgang, Segment leuchtet bei HIGH
 * (Common Cathode: gemeinsame Kathode an GND).
 *
 * Verkabelung (Board1):
 *   COM (beide gemeinsamen Pins) -> GND
 *   jedes Segment: Board-Pin --[ 220..330 Ohm ]--> Segment-Pin am Display
 *
 *   Segment  Board-Pin
 *     a        D5  (PE11)
 *     b        D6  (PE9)
 *     c        D3  (PE13)
 *     d        D10 (PD14)
 *     e        D0  (PG9)
 *     f        D1  (PG14)
 *     g        D13 (PA5)
 *   (Pin-Zuordnung in seg7.c per Tabelle aenderbar.)
 *
 * Segment-Lage:
 *      aaa
 *     f   b
 *      ggg
 *     e   c
 *      ddd
 *
 * Hinweis: 1 Ziffer kann nur 0..9 anzeigen -> Gesamtplaetze in Node-RED auf
 * <= 9 setzen. Werte ausserhalb 0..9 (auch -1 = noch kein Status) -> "-".
 */

/** @brief GPIO-Pins als Ausgang konfigurieren (Display startet leer). */
void seg7_init(void);

/** @brief Ziffer 0..9 anzeigen; alles andere (z. B. -1) -> Strich "-". */
void seg7_show_digit(int value);

/** @brief Alle Segmente aus. */
void seg7_clear(void);

#endif /* SEG7_H */
