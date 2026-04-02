/* ============================================================
   SUPER BULLES - ihm.c
   Gestion des entrées clavier (découplée de la logique)
   ============================================================ */

#include "ihm.h"
#include <allegro.h>
#include<main.h>

void ihm_read_keys(InputState *inp)
{
    inp->left    = key[KEY_LEFT];
    inp->right   = key[KEY_RIGHT];
    inp->up      = key[KEY_UP];
    inp->down    = key[KEY_DOWN];
    inp->shoot   = key[KEY_SPACE];
    inp->enter   = key[KEY_ENTER];
    inp->escape  = key[KEY_ESC];
    inp->last_char = 0;
}

int ihm_menu_navigate(InputState *inp, int *selection, int nb_items)
{
    /* Retourne 1 si Enter enfoncé */
    static int prev_up = 0, prev_down = 0, prev_enter = 0;

    if (inp->up && !prev_up) {
        (*selection)--;
        if (*selection < 0) *selection = nb_items - 1;
    }
    if (inp->down && !prev_down) {
        (*selection)++;
        if (*selection >= nb_items) *selection = 0;
    }
    int confirmed = (inp->enter && !prev_enter);
    prev_up    = inp->up;
    prev_down  = inp->down;
    prev_enter = inp->enter;
    return confirmed;
}

int ihm_text_input(InputState *inp, char *buf, int maxlen, int *cursor)
{
    /* Lecture de l'entrée clavier pour saisie texte */
    /* Retourne 1 si Enter validé */
    int c;
    (void)inp;
    while ((c = ureadkey(NULL)) != 0) {
        if (c == KEY_ENTER || c == '\r' || c == '\n') return 1;
        if (c == KEY_BACKSPACE || c == 8) {
            if (*cursor > 0) { (*cursor)--; buf[*cursor] = '\0'; }
        } else if (c >= 32 && c < 127 && *cursor < maxlen - 1) {
            buf[(*cursor)++] = (char)c;
            buf[*cursor] = '\0';
        }
    }
    return 0;
}
