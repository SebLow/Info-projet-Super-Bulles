#include <allegro.h>
#include <string.h>
#include "ihm.h"

int tir_pret = 1;

int ihm_direction(void) {
    if (key[KEY_LEFT])  return -1;
    if (key[KEY_RIGHT]) return  1;
    return 0;
}

int ihm_tir(void) {
    if (key[KEY_SPACE]) {
        if (tir_pret) { tir_pret = 0; return 1; }
        return 0;
    }
    tir_pret = 1;
    return 0;
}

int ihm_menu_haut(void)  { return key[KEY_UP]; }
int ihm_menu_bas(void)   { return key[KEY_DOWN]; }
int ihm_valider(void)    { return key[KEY_ENTER]; }
int ihm_quitter_jeu(void){ return key[KEY_ESC]; }

int ihm_saisie_pseudo(char *buf, int *len, int max_len) {
    /* Parcourt les touches ASCII */
    int c;
    for (c = KEY_A; c <= KEY_Z; c++) {
        if (key[c]) {
            if (*len < max_len - 1) {
                buf[(*len)++] = 'A' + (c - KEY_A);
                buf[*len]     = '\0';
            }
            clear_keybuf();
            return 0;
        }
    }
    for (c = KEY_0; c <= KEY_9; c++) {
        if (key[c]) {
            if (*len < max_len - 1) {
                buf[(*len)++] = '0' + (c - KEY_0);
                buf[*len]     = '\0';
            }
            clear_keybuf();
            return 0;
        }
    }
    if (key[KEY_BACKSPACE] && *len > 0) {
        buf[--(*len)] = '\0';
        clear_keybuf();
        return 0;
    }
    if (key[KEY_ENTER] && *len > 0) return 1;
    return 0;
}

void ihm_attendre_touche(void) {
    clear_keybuf();
    while (!keypressed()) { /* attente */ }
    clear_keybuf();
}
