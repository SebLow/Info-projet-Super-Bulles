
#include <allegro.h>
#include <time.h>
#include /* ============================================================
   SUPER BULLES - main.c
   Point d'entrée, boucle principale, machine à états
   ============================================================ */

#include <allegro.h>
#include <time.h>
#include "game.h"
#include "logic.h"
#include "graphics.h"
#include "ihm.h"

/* Fréquence cible : 60 fps */
#define TARGET_FPS 60

/* Variable volatile pour le timer Allegro */
static volatile int ticks = 0;
static void ticker(void) { ticks++; }
END_OF_FUNCTION(ticker)

/* ------------------------------------------------------------------ */
int main(void)
{
    srand((unsigned)time(NULL));

    /* --- Initialisation Allegro --- */
    allegro_init();
    install_keyboard();
    install_timer();

    /* Résolution et mode graphique */
    set_color_depth(32);
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, SCREEN_W, SCREEN_H, 0, 0) != 0) {
        /* Fallback plein écran */
        if (set_gfx_mode(GFX_AUTODETECT, SCREEN_W, SCREEN_H, 0, 0) != 0) {
            allegro_message("Impossible d'initialiser le mode graphique : %s",
                            allegro_error);
            return 1;
        }
    }

    set_window_title("Super Bulles - ECE 2025-2026");

    /* Timer 60fps */
    LOCK_VARIABLE(ticks);
    LOCK_FUNCTION(ticker);
    install_int_ex(ticker, BPS_TO_TIMER(TARGET_FPS));

    /* Buffer hors-écran (double buffering) */
    BITMAP *buf = create_bitmap(SCREEN_W, SCREEN_H);
    if (!buf) {
        allegro_message("Impossible de créer le buffer graphique.");
        return 1;
    }

    /* Initialisation du rendu */
    gfx_init();

    /* --- État global du jeu --- */
    GameData g;
    memset(&g, 0, sizeof(g));
    g.state = STATE_MENU;
    logic_load_saves(&g);

    /* Variables de navigation UI */
    int menu_sel     = 0;
    int load_sel     = 0;
    char pseudo_buf[PSEUDO_LEN] = {0};
    int  pseudo_cursor = 0;
    char load_buf[PSEUDO_LEN]   = {0};
    int  load_cursor = 0;
    int  cursor_anim = 0;

    InputState inp;
    int prev_enter = 0, prev_esc = 0;
    int prev_s = 0,  prev_q = 0;

    /* ================================================================
       BOUCLE PRINCIPALE
       ================================================================ */
    while (g.state != STATE_QUIT) {

        /* --- Attendre le tick 60fps --- */
        while (ticks == 0) { /* busy-wait léger */ }
        ticks = 0;

        /* --- Lecture des entrées --- */
        ihm_read_keys(&inp);
        cursor_anim++;

        /* ============================================================
           MACHINE A ETATS
           ============================================================ */

        switch (g.state) {

        /* ---- MENU PRINCIPAL ---- */
        case STATE_MENU: {
            int confirmed = ihm_menu_navigate(&inp, &menu_sel, 4);
            if (confirmed) {
                switch (menu_sel) {
                    case 0: g.state = STATE_RULES;    break;
                    case 1:
                        memset(pseudo_buf, 0, PSEUDO_LEN);
                        pseudo_cursor = 0;
                        g.state = STATE_PSEUDO;
                        break;
                    case 2:
                        memset(load_buf, 0, PSEUDO_LEN);
                        load_cursor = 0;
                        load_sel = 0;
                        g.state = STATE_LOAD_GAME;
                        break;
                    case 3: g.state = STATE_QUIT; break;
                }
            }
            break;
        }

        /* ---- RÈGLES ---- */
        case STATE_RULES: {
            int ce = (inp.enter && !prev_enter);
            int ce2 = (inp.escape && !prev_esc);
            if (ce || ce2) g.state = STATE_MENU;
            break;
        }

        /* ---- SAISIE PSEUDO (nouvelle partie) ---- */
        case STATE_PSEUDO: {
            int done = ihm_text_input(&inp, pseudo_buf, PSEUDO_LEN, &pseudo_cursor);
            if (inp.escape && !prev_esc) { g.state = STATE_MENU; break; }
            if (done && strlen(pseudo_buf) > 0) {
                logic_init_game(&g, pseudo_buf);
                logic_init_level(&g, 1);
            }
            break;
        }

        /* ---- CHARGEMENT PARTIE ---- */
        case STATE_LOAD_GAME: {
            /* Saisie pseudo */
            ihm_text_input(&inp, load_buf, PSEUDO_LEN, &load_cursor);
            /* Navigation dans la liste */
            static int prev_up2 = 0, prev_dn2 = 0;
            if (inp.up && !prev_up2 && load_sel > 0) load_sel--;
            if (inp.down && !prev_dn2 && load_sel < g.nb_saves - 1) load_sel++;
            prev_up2 = inp.up; prev_dn2 = inp.down;

            if (inp.escape && !prev_esc) { g.state = STATE_MENU; break; }
            if (inp.enter && !prev_enter) {
                /* Cherche par pseudo tapé, sinon par sélection liste */
                int idx = -1;
                if (strlen(load_buf) > 0) idx = logic_find_save(&g, load_buf);
                if (idx == -1 && g.nb_saves > 0) idx = load_sel;
                if (idx >= 0) {
                    strncpy(g.player.pseudo, g.saves[idx].pseudo, PSEUDO_LEN-1);
                    g.score = g.saves[idx].score;
                    g.player.alive = 1;
                    logic_init_level(&g, g.saves[idx].level);
                }
                else g.state = STATE_MENU;
            }
            break;
        }

        /* ---- COMPTE À REBOURS ---- */
        case STATE_COUNTDOWN: {
            logic_update(&g, 0, 0, 0, 1.0f/TARGET_FPS);
            break;
        }

        /* ---- JEU EN COURS ---- */
        case STATE_PLAY: {
            logic_update(&g, inp.left, inp.right, inp.shoot,
                         1.0f / TARGET_FPS);
            /* Pause/menu */
            if (inp.escape && !prev_esc) {
                logic_save(&g);
                g.state = STATE_MENU;
            }
            break;
        }

        /* ---- VICTOIRE NIVEAU ---- */
        case STATE_LEVEL_WIN: {
            int ce = (inp.enter && !prev_enter);
            int cs = (key[KEY_S]   && !prev_s);
            int cq = (key[KEY_Q]   && !prev_q);
            int ce2= (inp.escape   && !prev_esc);
            if (cs) logic_save(&g);
            if (ce) {
                /* Niveau suivant */
                logic_init_level(&g, g.current_level + 1);
            }
            if (ce2) g.state = STATE_MENU;
            if (cq)  g.state = STATE_QUIT;
            break;
        }

        /* ---- ECHEC NIVEAU ---- */
        case STATE_LEVEL_LOSE: {
            int ce = (inp.enter && !prev_enter);
            int cs = (key[KEY_S]   && !prev_s);
            int cq = (key[KEY_Q]   && !prev_q);
            int ce2= (inp.escape   && !prev_esc);
            if (cs) logic_save(&g);
            if (ce) {
                /* Recommencer le même niveau */
                logic_init_level(&g, g.current_level);
            }
            if (ce2) g.state = STATE_MENU;
            if (cq)  g.state = STATE_QUIT;
            break;
        }

        /* ---- VICTOIRE FINALE ---- */
        case STATE_GAME_WIN: {
            int ce = (inp.enter && !prev_enter);
            if (ce) g.state = STATE_MENU;
            break;
        }

        /* ---- GAME OVER (alias lose au dernier niveau) ---- */
        case STATE_GAME_OVER: {
            int ce = (inp.enter && !prev_enter);
            if (ce) g.state = STATE_MENU;
            break;
        }

        default: break;
        }

        /* --- Mémorisation état précédent des touches spéciales --- */
        prev_enter = inp.enter;
        prev_esc   = inp.escape;
        prev_s     = key[KEY_S];
        prev_q     = key[KEY_Q];

        /* ============================================================
           RENDU
           ============================================================ */
        clear_to_color(buf, 0);

        switch (g.state) {
            case STATE_MENU:
                gfx_draw_menu(buf, menu_sel);
                break;
            case STATE_RULES:
                gfx_draw_rules(buf);
                break;
            case STATE_PSEUDO:
                gfx_draw_pseudo_input(buf, pseudo_buf, cursor_anim);
                break;
            case STATE_LOAD_GAME:
                gfx_draw_load_screen(buf, &g, load_sel, load_buf, cursor_anim);
                break;
            case STATE_COUNTDOWN:
                gfx_draw_countdown(buf, &g);
                break;
            case STATE_PLAY:
                gfx_draw_game(buf, &g);
                break;
            case STATE_LEVEL_WIN:
                gfx_draw_level_result(buf, &g, 1);
                break;
            case STATE_LEVEL_LOSE:
                gfx_draw_level_result(buf, &g, 0);
                break;
            case STATE_GAME_WIN:
                gfx_draw_game_win(buf, &g);
                break;
            default:
                break;
        }

        /* Flip buffer */
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }

    /* --- Nettoyage --- */
    destroy_bitmap(buf);
    gfx_destroy();
    allegro_exit();
    return 0;
}
END_OF_MAIN()
"game.h"
#include "logic.h"
#include "graphics.h"
#include "ihm.h"

/* Fréquence cible : 60 fps */
#define TARGET_FPS 60

/* Variable volatile pour le timer Allegro */
static volatile int ticks = 0;
static void ticker(void) { ticks++; }
END_OF_FUNCTION(ticker)

/* ------------------------------------------------------------------ */
int main(void)
{
    srand((unsigned)time(NULL));

    /* --- Initialisation Allegro --- */
    allegro_init();
    install_keyboard();
    install_timer();

    /* Résolution et mode graphique */
    set_color_depth(32);
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, SCREEN_W, SCREEN_H, 0, 0) != 0) {
        /* Fallback plein écran */
        if (set_gfx_mode(GFX_AUTODETECT, SCREEN_W, SCREEN_H, 0, 0) != 0) {
            allegro_message("Impossible d'initialiser le mode graphique : %s",
                            allegro_error);
            return 1;
        }
    }

    set_window_title("Super Bulles - ECE 2025-2026");

    /* Timer 60fps */
    LOCK_VARIABLE(ticks);
    LOCK_FUNCTION(ticker);
    install_int_ex(ticker, BPS_TO_TIMER(TARGET_FPS));

    /* Buffer hors-écran (double buffering) */
    BITMAP *buf = create_bitmap(SCREEN_W, SCREEN_H);
    if (!buf) {
        allegro_message("Impossible de créer le buffer graphique.");
        return 1;
    }

    /* Initialisation du rendu */
    gfx_init();

    /* --- État global du jeu --- */
    GameData g;
    memset(&g, 0, sizeof(g));
    g.state = STATE_MENU;
    logic_load_saves(&g);

    /* Variables de navigation UI */
    int menu_sel     = 0;
    int load_sel     = 0;
    char pseudo_buf[PSEUDO_LEN] = {0};
    int  pseudo_cursor = 0;
    char load_buf[PSEUDO_LEN]   = {0};
    int  load_cursor = 0;
    int  cursor_anim = 0;

    InputState inp;
    int prev_enter = 0, prev_esc = 0;
    int prev_s = 0,  prev_q = 0;

    /* ================================================================
       BOUCLE PRINCIPALE
       ================================================================ */
    while (g.state != STATE_QUIT) {

        /* --- Attendre le tick 60fps --- */
        while (ticks == 0) { /* busy-wait léger */ }
        ticks = 0;

        /* --- Lecture des entrées --- */
        ihm_read_keys(&inp);
        cursor_anim++;

        /* ============================================================
           MACHINE A ETATS
           ============================================================ */

        switch (g.state) {

        /* ---- MENU PRINCIPAL ---- */
        case STATE_MENU: {
            int confirmed = ihm_menu_navigate(&inp, &menu_sel, 4);
            if (confirmed) {
                switch (menu_sel) {
                    case 0: g.state = STATE_RULES;    break;
                    case 1:
                        memset(pseudo_buf, 0, PSEUDO_LEN);
                        pseudo_cursor = 0;
                        g.state = STATE_PSEUDO;
                        break;
                    case 2:
                        memset(load_buf, 0, PSEUDO_LEN);
                        load_cursor = 0;
                        load_sel = 0;
                        g.state = STATE_LOAD_GAME;
                        break;
                    case 3: g.state = STATE_QUIT; break;
                }
            }
            break;
        }

        /* ---- RÈGLES ---- */
        case STATE_RULES: {
            int ce = (inp.enter && !prev_enter);
            int ce2 = (inp.escape && !prev_esc);
            if (ce || ce2) g.state = STATE_MENU;
            break;
        }

        /* ---- SAISIE PSEUDO (nouvelle partie) ---- */
        case STATE_PSEUDO: {
            int done = ihm_text_input(&inp, pseudo_buf, PSEUDO_LEN, &pseudo_cursor);
            if (inp.escape && !prev_esc) { g.state = STATE_MENU; break; }
            if (done && strlen(pseudo_buf) > 0) {
                logic_init_game(&g, pseudo_buf);
                logic_init_level(&g, 1);
            }
            break;
        }

        /* ---- CHARGEMENT PARTIE ---- */
        case STATE_LOAD_GAME: {
            /* Saisie pseudo */
            ihm_text_input(&inp, load_buf, PSEUDO_LEN, &load_cursor);
            /* Navigation dans la liste */
            static int prev_up2 = 0, prev_dn2 = 0;
            if (inp.up && !prev_up2 && load_sel > 0) load_sel--;
            if (inp.down && !prev_dn2 && load_sel < g.nb_saves - 1) load_sel++;
            prev_up2 = inp.up; prev_dn2 = inp.down;

            if (inp.escape && !prev_esc) { g.state = STATE_MENU; break; }
            if (inp.enter && !prev_enter) {
                /* Cherche par pseudo tapé, sinon par sélection liste */
                int idx = -1;
                if (strlen(load_buf) > 0) idx = logic_find_save(&g, load_buf);
                if (idx == -1 && g.nb_saves > 0) idx = load_sel;
                if (idx >= 0) {
                    strncpy(g.player.pseudo, g.saves[idx].pseudo, PSEUDO_LEN-1);
                    g.score = g.saves[idx].score;
                    g.player.alive = 1;
                    logic_init_level(&g, g.saves[idx].level);
                }
                else g.state = STATE_MENU;
            }
            break;
        }

        /* ---- COMPTE À REBOURS ---- */
        case STATE_COUNTDOWN: {
            logic_update(&g, 0, 0, 0, 1.0f/TARGET_FPS);
            break;
        }

        /* ---- JEU EN COURS ---- */
        case STATE_PLAY: {
            logic_update(&g, inp.left, inp.right, inp.shoot,
                         1.0f / TARGET_FPS);
            /* Pause/menu */
            if (inp.escape && !prev_esc) {
                logic_save(&g);
                g.state = STATE_MENU;
            }
            break;
        }

        /* ---- VICTOIRE NIVEAU ---- */
        case STATE_LEVEL_WIN: {
            int ce = (inp.enter && !prev_enter);
            int cs = (key[KEY_S]   && !prev_s);
            int cq = (key[KEY_Q]   && !prev_q);
            int ce2= (inp.escape   && !prev_esc);
            if (cs) logic_save(&g);
            if (ce) {
                /* Niveau suivant */
                logic_init_level(&g, g.current_level + 1);
            }
            if (ce2) g.state = STATE_MENU;
            if (cq)  g.state = STATE_QUIT;
            break;
        }

        /* ---- ECHEC NIVEAU ---- */
        case STATE_LEVEL_LOSE: {
            int ce = (inp.enter && !prev_enter);
            int cs = (key[KEY_S]   && !prev_s);
            int cq = (key[KEY_Q]   && !prev_q);
            int ce2= (inp.escape   && !prev_esc);
            if (cs) logic_save(&g);
            if (ce) {
                /* Recommencer le même niveau */
                logic_init_level(&g, g.current_level);
            }
            if (ce2) g.state = STATE_MENU;
            if (cq)  g.state = STATE_QUIT;
            break;
        }

        /* ---- VICTOIRE FINALE ---- */
        case STATE_GAME_WIN: {
            int ce = (inp.enter && !prev_enter);
            if (ce) g.state = STATE_MENU;
            break;
        }

        /* ---- GAME OVER (alias lose au dernier niveau) ---- */
        case STATE_GAME_OVER: {
            int ce = (inp.enter && !prev_enter);
            if (ce) g.state = STATE_MENU;
            break;
        }

        default: break;
        }

        /* --- Mémorisation état précédent des touches spéciales --- */
        prev_enter = inp.enter;
        prev_esc   = inp.escape;
        prev_s     = key[KEY_S];
        prev_q     = key[KEY_Q];

        /* ============================================================
           RENDU
           ============================================================ */
        clear_to_color(buf, 0);

        switch (g.state) {
            case STATE_MENU:
                gfx_draw_menu(buf, menu_sel);
                break;
            case STATE_RULES:
                gfx_draw_rules(buf);
                break;
            case STATE_PSEUDO:
                gfx_draw_pseudo_input(buf, pseudo_buf, cursor_anim);
                break;
            case STATE_LOAD_GAME:
                gfx_draw_load_screen(buf, &g, load_sel, load_buf, cursor_anim);
                break;
            case STATE_COUNTDOWN:
                gfx_draw_countdown(buf, &g);
                break;
            case STATE_PLAY:
                gfx_draw_game(buf, &g);
                break;
            case STATE_LEVEL_WIN:
                gfx_draw_level_result(buf, &g, 1);
                break;
            case STATE_LEVEL_LOSE:
                gfx_draw_level_result(buf, &g, 0);
                break;
            case STATE_GAME_WIN:
                gfx_draw_game_win(buf, &g);
                break;
            default:
                break;
        }

        /* Flip buffer */
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }

    /* --- Nettoyage --- */
    destroy_bitmap(buf);
    gfx_destroy();
    allegro_exit();
    return 0;
}
END_OF_MAIN()
