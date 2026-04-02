/* ============================================================
   SUPER BULLES - graphics.c
   Rendu graphique avec Allegro 4
   Sprites dessinés en formes géométriques colorées
   ============================================================ */

#include "graphics.h"
#include <math.h>
#include<main.h>

/* ------------------------------------------------------------------ */
/* Couleurs (initialisées dans gfx_init)                               */
/* ------------------------------------------------------------------ */

static int C_BLACK, C_WHITE, C_RED, C_GREEN, C_BLUE, C_YELLOW;
static int C_ORANGE, C_PINK, C_PURPLE, C_CYAN, C_BROWN, C_DARK;
static int C_GOLD, C_LTGRAY, C_DKGRAY, C_BG_SKY, C_SCORE_BG;
static int C_CREAM, C_CHOCO, C_STRAWB, C_OLIVE;

void gfx_init(void)
{
    C_BLACK  = makecol(0,   0,   0);
    C_WHITE  = makecol(255, 255, 255);
    C_RED    = makecol(220, 50,  50);
    C_GREEN  = makecol(50,  180, 80);
    C_BLUE   = makecol(60,  120, 220);
    C_YELLOW = makecol(255, 230, 0);
    C_ORANGE = makecol(255, 140, 0);
    C_PINK   = makecol(255, 150, 200);
    C_PURPLE = makecol(160, 60,  220);
    C_CYAN   = makecol(0,   210, 230);
    C_BROWN  = makecol(139, 90,  43);
    C_DARK   = makecol(30,  20,  60);
    C_GOLD   = makecol(255, 210, 50);
    C_LTGRAY = makecol(200, 200, 200);
    C_DKGRAY = makecol(80,  80,  80);
    C_BG_SKY = makecol(135, 206, 235);
    C_SCORE_BG = makecol(20, 20, 50);
    C_CREAM  = makecol(255, 240, 200);
    C_CHOCO  = makecol(101, 55,  0);
    C_STRAWB = makecol(230, 50,  100);
    C_OLIVE  = makecol(100, 140, 40);
}

void gfx_destroy(void) {}

/* ------------------------------------------------------------------ */
/* Dessin d'une bulle selon son type de nourriture                     */
/* ------------------------------------------------------------------ */

static void draw_cookie(BITMAP *buf, int cx, int cy, int r, int frame)
{
    int i;
    /* Corps biscuit */
    circlefill(buf, cx, cy, r, C_BROWN);
    circle(buf, cx, cy, r, C_CHOCO);
    /* Texture craquelée */
    int c2 = makecol(160, 110, 60);
    circle(buf, cx, cy, r - 4, c2);
    /* Pépites de chocolat */
    float angle;
    for (i = 0; i < 6; i++) {
        angle = (float)i * 3.14159f / 3.0f + frame * 0.01f;
        int px = cx + (int)(cosf(angle) * r * 0.55f);
        int py = cy + (int)(sinf(angle) * r * 0.55f);
        circlefill(buf, px, py, MAX(2, r/7), C_CHOCO);
    }
    /* Brillance */
    circlefill(buf, cx - r/4, cy - r/4, MAX(2, r/5), makecol(220,180,120));
}

static void draw_pizza(BITMAP *buf, int cx, int cy, int r, int frame)
{
    int i;
    float a0 = frame * 0.008f;
    /* Base triangle pizza */
    int pts[6];
    for (i = 0; i < 3; i++) {
        float a = a0 + i * 2.0f * 3.14159f / 3.0f;
        pts[i*2]   = cx + (int)(cosf(a) * r);
        pts[i*2+1] = cy + (int)(sinf(a) * r);
    }
    /* Fond jaune (fromage) */
    circlefill(buf, cx, cy, r, C_YELLOW);
    circle(buf, cx, cy, r, C_ORANGE);
    /* Triangle rouge (sauce) intérieur */
    triangle(buf,
        cx + (int)(cosf(a0)                    * r * 0.65f),
        cy + (int)(sinf(a0)                    * r * 0.65f),
        cx + (int)(cosf(a0 + 2.094f) * r * 0.65f),
        cy + (int)(sinf(a0 + 2.094f) * r * 0.65f),
        cx + (int)(cosf(a0 + 4.189f) * r * 0.65f),
        cy + (int)(sinf(a0 + 4.189f) * r * 0.65f),
        C_RED);
    /* Olives */
    for (i = 0; i < 4; i++) {
        float a = a0 + i * 1.57f;
        circlefill(buf,
            cx + (int)(cosf(a) * r * 0.4f),
            cy + (int)(sinf(a) * r * 0.4f),
            MAX(2, r/6), C_OLIVE);
    }
    /* Brillance */
    circlefill(buf, cx - r/4, cy - r/4, MAX(2, r/5), makecol(255,250,180));
}

static void draw_burger(BITMAP *buf, int cx, int cy, int r, int frame)
{
    (void)frame;
    /* Pain haut */
    int bun_c = makecol(210, 150, 60);
    ellipsefill(buf, cx, cy - r/3, r, r*2/5, bun_c);
    /* Salade */
    ellipsefill(buf, cx, cy - r/8, r+2, r/5, C_GREEN);
    /* Steak */
    ellipsefill(buf, cx, cy + r/8, r, r/5, C_BROWN);
    /* Fromage */
    ellipsefill(buf, cx, cy + r/4, r+1, r/6, C_YELLOW);
    /* Pain bas */
    ellipsefill(buf, cx, cy + r/2, r, r/4, bun_c);
    /* Contour */
    ellipse(buf, cx, cy, r, r*4/5, C_CHOCO);
    /* Graines sur le pain */
    int i;
    for (i = 0; i < 4; i++) {
        float a = i * 0.8f;
        circlefill(buf,
            cx + (int)(cosf(a) * r * 0.55f),
            cy - r/3 + (int)(sinf(a) * r * 0.18f),
            MAX(1, r/9),
            C_CREAM);
    }
}

static void draw_cake_boss(BITMAP *buf, int cx, int cy, int r, int frame)
{
    int i;
    /* Corps gâteau (ellipse) */
    ellipsefill(buf, cx, cy + r/4, r, r*3/4, C_STRAWB);
    /* Étages */
    ellipsefill(buf, cx, cy - r/4, r*7/8, r/2, C_PINK);
    ellipsefill(buf, cx, cy - r*3/4, r*6/8, r/3, C_WHITE);
    /* Glaçage dégoulinant */
    int glac = makecol(255, 100, 150);
    for (i = 0; i < 6; i++) {
        int gx = cx - r + i * r/3;
        vline(buf, gx, cy - r/4, cy - r/4 + r/3 + (i%2)*r/6, glac);
    }
    /* Bougies */
    for (i = 0; i < 4; i++) {
        int candx = cx - r/2 + i * r/3;
        int candy = cy - r - 4;
        rectfill(buf, candx - 2, candy, candx + 2, candy + r/4, C_CYAN);
        /* Flamme animée */
        int foff = (int)(sinf(frame * 0.2f + i) * 3);
        circlefill(buf, candx + foff, candy - 4, 4, C_YELLOW);
        circlefill(buf, candx + foff, candy - 6, 2, C_ORANGE);
    }
    /* Yeux de boss méchant */
    circlefill(buf, cx - r/3, cy - r/2, r/6, C_WHITE);
    circlefill(buf, cx + r/3, cy - r/2, r/6, C_WHITE);
    int eyeoff = (int)(sinf(frame * 0.15f) * 2);
    circlefill(buf, cx - r/3 + eyeoff, cy - r/2, r/9, C_RED);
    circlefill(buf, cx + r/3 + eyeoff, cy - r/2, r/9, C_RED);
    /* Bouche méchante */
    arc(buf, cx, cy - r/3, itofix(-20), itofix(20), r/3, C_DARK);
    /* Contour */
    ellipse(buf, cx, cy + r/4, r, r*3/4, C_CHOCO);
}

void gfx_draw_bubble(BITMAP *buf, const Bubble *b, int frame)
{
    if (!b->alive) return;
    int cx = (int)b->x;
    int cy = (int)b->y;
    int r  = b->radius;

    switch (b->food) {
        case FOOD_COOKIE: draw_cookie(buf, cx, cy, r, frame); break;
        case FOOD_PIZZA:  draw_pizza(buf, cx, cy, r, frame);  break;
        case FOOD_BURGER: draw_burger(buf, cx, cy, r, frame); break;
        case FOOD_CAKE:   draw_cake_boss(buf, cx, cy, r, frame); break;
        case FOOD_MIX:    /* mélange : utilise level%3 */
            switch ((cx + cy) % 3) {
                case 0: draw_cookie(buf, cx, cy, r, frame); break;
                case 1: draw_pizza(buf, cx, cy, r, frame);  break;
                default: draw_burger(buf, cx, cy, r, frame); break;
            }
            break;
    }

    /* Indicateur boss HP */
    if (b->food == FOOD_CAKE && b->hits_needed > 1) {
        int bar_w = r * 2;
        int bar_x = cx - r;
        int bar_y = cy + r + 8;
        rectfill(buf, bar_x, bar_y, bar_x + bar_w, bar_y + 6, C_DKGRAY);
        /* On ne connaît pas boss_max_hits ici, on affiche juste la jauge */
        int fill = bar_w * b->hits_needed / 10;
        rectfill(buf, bar_x, bar_y, bar_x + fill, bar_y + 6, C_RED);
        rect(buf, bar_x, bar_y, bar_x + bar_w, bar_y + 6, C_WHITE);
    }
}

/* ------------------------------------------------------------------ */
/* Joueur                                                              */
/* ------------------------------------------------------------------ */

void gfx_draw_player(BITMAP *buf, const Player *p, int frame)
{
    if (!p->alive) return;
    int x = (int)p->x;
    int y = (int)p->y;
    int w = PLAYER_W;
    int h = PLAYER_H;

    /* Corps */
    int body_c = makecol(60, 120, 220);
    rectfill(buf, x + 4, y + h/2, x + w - 4, y + h, body_c);
    /* Tête */
    circlefill(buf, x + w/2, y + h/4, h/4, makecol(255, 210, 160));
    /* Casque/chapeau */
    rectfill(buf, x + 3, y, x + w - 3, y + h/4, makecol(200, 50, 50));
    /* Bras animés */
    int arm_off = (int)(sinf(frame * 0.2f) * 3);
    line(buf, x + 4, y + h*2/3, x - 4, y + h/2 + arm_off, body_c);
    line(buf, x + w - 4, y + h*2/3, x + w + 4, y + h/2 + arm_off, body_c);
    /* Jambes */
    int leg_off = (int)(sinf(frame * 0.25f) * 4);
    line(buf, x + w/3, y + h, x + w/3 - leg_off, y + h + 6, body_c);
    line(buf, x + 2*w/3, y + h, x + 2*w/3 + leg_off, y + h + 6, body_c);
    /* Arme actuelle */
    if (p->weapon != WEAPON_BASIC) {
        int wc = (p->weapon == WEAPON_DOUBLE) ? C_CYAN : C_PURPLE;
        rectfill(buf, x + w/2 - 3, y + h/2 - 6, x + w/2 + 3, y + h/2 + 4, wc);
        /* Minuterie arme */
        int bar_w = w;
        int pct = p->weapon_timer * bar_w / 600;
        rectfill(buf, x, y + h + 8, x + pct, y + h + 12, wc);
        rect(buf, x, y + h + 8, x + bar_w, y + h + 12, C_WHITE);
    }
}

/* ------------------------------------------------------------------ */
/* HUD                                                                 */
/* ------------------------------------------------------------------ */

void gfx_draw_hud(BITMAP *buf, const GameData *g)
{
    char txt[80];
    /* Barre supérieure */
    rectfill(buf, 0, 0, SCREEN_W, ZONE_TOP - 1, C_SCORE_BG);
    rect(buf, 0, 0, SCREEN_W - 1, ZONE_TOP - 1, C_GOLD);

    sprintf(txt, "JOUEUR: %s", g->player.pseudo);
    textout_ex(buf, font, txt, 8, 4, C_WHITE, -1);
    sprintf(txt, "SCORE: %d", g->score);
    textout_ex(buf, font, txt, 8, 18, C_GOLD, -1);

    /* Niveau */
    sprintf(txt, "NIVEAU: %d", g->current_level);
    textout_ex(buf, font, txt, SCREEN_W/2 - 50, 10, C_CYAN, -1);

    /* Timer (rouge si urgent) */
    int tc = (g->time_left <= 10) ? C_RED : C_WHITE;
    sprintf(txt, "TEMPS: %d", g->time_left);
    textout_ex(buf, font, txt, SCREEN_W - 120, 10, tc, -1);

    /* Arme active */
    if (g->player.weapon != WEAPON_BASIC) {
        const char *wname = (g->player.weapon == WEAPON_DOUBLE) ?
                            "DOUBLE" : "LARGE";
        sprintf(txt, "[%s]", wname);
        textout_ex(buf, font, txt, SCREEN_W/2 - 30, 30, C_CYAN, -1);
    }

    /* Barre inférieure */
    rectfill(buf, 0, PLAY_BOT, SCREEN_W, SCREEN_H, C_SCORE_BG);
    rect(buf, 0, PLAY_BOT, SCREEN_W - 1, SCREEN_H - 1, C_GOLD);
    sprintf(txt, "ESPACE=Tirer  FLECHES=Bouger  ECHAP=Pause");
    textout_ex(buf, font, txt, 8, PLAY_BOT + 16, C_LTGRAY, -1);

    /* Boss HP */
    if (g->boss_alive) {
        sprintf(txt, "BOSS HP: %d/%d", g->boss_max_hits - g->boss_hits, g->boss_max_hits);
        textout_ex(buf, font, txt, SCREEN_W - 140, PLAY_BOT + 16, C_RED, -1);
    }
}

/* ------------------------------------------------------------------ */
/* Fond de décor selon le niveau                                        */
/* ------------------------------------------------------------------ */

static void draw_background(BITMAP *buf, int level, int frame)
{
    int i;
    /* Ciel */
    int sky_top, sky_bot;
    switch (level) {
        case 1: sky_top = makecol(100,180,255); sky_bot = makecol(200,230,255); break;
        case 2: sky_top = makecol(255,160,80);  sky_bot = makecol(255,220,160); break;
        case 3: sky_top = makecol(60, 80, 140); sky_bot = makecol(130,170,220); break;
        case 4: sky_top = makecol(140,60, 140); sky_bot = makecol(220,160,220); break;
        default:sky_top = makecol(20, 10, 40);  sky_bot = makecol(80, 40, 100); break;
    }
    /* Dégradé vertical simple */
    int play_h = PLAY_BOT - PLAY_TOP;
    for (i = 0; i < play_h; i++) {
        int r1 = getr(sky_top) + (getr(sky_bot)-getr(sky_top)) * i / play_h;
        int g1 = getg(sky_top) + (getg(sky_bot)-getg(sky_top)) * i / play_h;
        int b1 = getb(sky_top) + (getb(sky_bot)-getb(sky_top)) * i / play_h;
        hline(buf, 0, PLAY_TOP + i, SCREEN_W - 1, makecol(r1,g1,b1));
    }

    /* Nuages animés (niveaux 1-4) */
    if (level <= 4) {
        int cloud_c = makecol(255, 255, 255);
        for (i = 0; i < 3; i++) {
            int cx = (i * 250 + frame / 2) % (SCREEN_W + 120) - 60;
            int cy = PLAY_TOP + 40 + i * 30;
            circlefill(buf, cx,      cy,     20, cloud_c);
            circlefill(buf, cx + 22, cy - 8, 25, cloud_c);
            circlefill(buf, cx + 48, cy,     18, cloud_c);
        }
    }

    /* Sol */
    int ground_c = makecol(80, 55, 30);
    rectfill(buf, 0, PLAY_BOT - 14, SCREEN_W, PLAY_BOT, ground_c);
    /* Herbe */
    int grass_c = (level == 5) ? makecol(40, 10, 60) : makecol(50, 160, 60);
    rectfill(buf, 0, PLAY_BOT - 18, SCREEN_W, PLAY_BOT - 14, grass_c);
    for (i = 0; i < SCREEN_W; i += 12) {
        int gh = 4 + (i % 3) * 2;
        triangle(buf, i, PLAY_BOT - 18, i + 6, PLAY_BOT - 18 - gh, i + 12, PLAY_BOT - 18, grass_c);
    }
}

/* ------------------------------------------------------------------ */
/* Écran de jeu principal                                              */
/* ------------------------------------------------------------------ */

void gfx_draw_game(BITMAP *buf, const GameData *g)
{
    int i;
    draw_background(buf, g->current_level, g->frame);

    /* Armes au sol */
    for (i = 0; i < MAX_WEAPONS; i++) {
        if (!g->weapons[i].alive) continue;
        int wx = (int)g->weapons[i].x;
        int wy = (int)g->weapons[i].y;
        int wc = (g->weapons[i].type == WEAPON_DOUBLE) ? C_CYAN : C_PURPLE;
        /* Icône arme : étoile */
        circlefill(buf, wx + 10, wy + 10, 10, wc);
        /* Clignotement */
        if ((g->frame / 10) % 2 == 0)
            circle(buf, wx + 10, wy + 10, 12, C_WHITE);
        /* Lettre */
        const char *wl = (g->weapons[i].type == WEAPON_DOUBLE) ? "D" : "W";
        textout_ex(buf, font, wl, wx + 6, wy + 5, C_WHITE, -1);
    }

    /* Éclairs */
    for (i = 0; i < 4; i++) {
        if (!g->lightnings[i].alive) continue;
        int lx = (int)g->lightnings[i].x;
        int ly = (int)g->lightnings[i].y;
        /* Zigzag éclair */
        int j;
        for (j = 0; j < 5; j++) {
            int off = (j % 2 == 0) ? -6 : 6;
            line(buf, lx + off, ly + j*8, lx - off, ly + j*8 + 8, C_YELLOW);
            line(buf, lx + off, ly + j*8, lx - off, ly + j*8 + 8, C_WHITE);
        }
        circlefill(buf, lx, ly + 40, 5, C_YELLOW);
    }

    /* Bulles */
    for (i = 0; i < MAX_BUBBLES; i++)
        gfx_draw_bubble(buf, &g->bubbles[i], g->frame);

    /* Projectiles */
    for (i = 0; i < MAX_PROJ; i++) {
        if (!g->projs[i].alive) continue;
        int px = (int)g->projs[i].x;
        int py = (int)g->projs[i].y;
        /* Crochet vertical style Pang */
        rectfill(buf, px, py, px + PROJ_W, py + PROJ_H, C_WHITE);
        rectfill(buf, px + 1, py + 1, px + PROJ_W - 1, py + PROJ_H - 1,
                 makecol(100, 200, 255));
        /* Pointe */
        triangle(buf, px, py, px + PROJ_W, py, px + PROJ_W/2, py - 8, C_CYAN);
    }

    /* Joueur */
    gfx_draw_player(buf, &g->player, g->frame);

    /* HUD */
    gfx_draw_hud(buf, g);
}

/* ------------------------------------------------------------------ */
/* Menu principal                                                       */
/* ------------------------------------------------------------------ */

void gfx_draw_menu(BITMAP *buf, int selection)
{
    int i;
    /* Fond */
    clear_to_color(buf, C_DARK);
    /* Titre */
    int tc = makecol(255, 200, 0);
    textout_centre_ex(buf, font, "** SUPER BULLES **", SCREEN_W/2, 60, tc, -1);
    textout_centre_ex(buf, font, "Projet ECE 2025-2026", SCREEN_W/2, 80, C_LTGRAY, -1);

    /* Déco : quelques bulles/cookies */
    draw_cookie(buf, 80,  160, 35, 0);
    draw_pizza(buf,  720, 200, 35, 0);
    draw_burger(buf, 80,  350, 30, 0);

    /* Items de menu */
    const char *items[] = {
        "1. Lire les regles du jeu",
        "2. Nouvelle partie",
        "3. Reprendre une partie",
        "4. Quitter"
    };
    int nb = 4;
    int base_y = 180;
    for (i = 0; i < nb; i++) {
        int color = (i == selection) ? C_YELLOW : C_WHITE;
        int y = base_y + i * 40;
        if (i == selection) {
            rectfill(buf, SCREEN_W/2 - 180, y - 4,
                          SCREEN_W/2 + 180, y + 20,
                          makecol(60, 60, 120));
            textout_centre_ex(buf, font, ">", SCREEN_W/2 - 175, y, C_GOLD, -1);
        }
        textout_centre_ex(buf, font, items[i], SCREEN_W/2, y, color, -1);
    }
    textout_centre_ex(buf, font,
        "Haut/Bas pour naviguer, Entree pour valider",
        SCREEN_W/2, SCREEN_H - 40, C_DKGRAY, -1);
}

/* ------------------------------------------------------------------ */
/* Règles du jeu                                                        */
/* ------------------------------------------------------------------ */

void gfx_draw_rules(BITMAP *buf)
{
    clear_to_color(buf, C_DARK);
    int y = 30;
    int lh = 18;
    int c = C_WHITE;
    textout_centre_ex(buf, font, "== REGLES DU JEU ==", SCREEN_W/2, y, C_GOLD, -1); y += lh*2;
    textout_ex(buf, font, "- Eliminez toutes les bulles de nourriture !", 30, y, c, -1); y += lh;
    textout_ex(buf, font, "- FLECHES : deplacer le personnage",            30, y, c, -1); y += lh;
    textout_ex(buf, font, "- ESPACE  : tirer un crochet vers le haut",     30, y, c, -1); y += lh;
    textout_ex(buf, font, "- Une bulle touchee se divise en 2 plus petites", 30, y, c, -1); y += lh;
    textout_ex(buf, font, "- Niveau 1 : cookies    Niveau 2 : pizzas",     30, y, C_YELLOW, -1); y += lh;
    textout_ex(buf, font, "- Niveau 3 : burgers    Niveau 4 : tout !",     30, y, C_YELLOW, -1); y += lh;
    textout_ex(buf, font, "- Niveau 5 : BOSS = enorme gateau d'anniv !",   30, y, C_RED,    -1); y += lh;
    y += lh/2;
    textout_ex(buf, font, "Ramassez les armes qui tombent des bulles :", 30, y, C_CYAN, -1); y += lh;
    textout_ex(buf, font, "  [D] Double tir  [W] Tir large",              30, y, C_CYAN, -1); y += lh;
    y += lh/2;
    textout_ex(buf, font, "Niveau 3+ : ATTENTION aux eclairs !",          30, y, C_GOLD, -1); y += lh;
    textout_ex(buf, font, "Si le temps est ecoule ou si vous etes touche => ECHEC", 30, y, C_RED, -1);
    y += lh * 2;
    textout_centre_ex(buf, font, "Appuyez sur ECHAP ou ENTREE pour revenir",
                      SCREEN_W/2, y, C_LTGRAY, -1);
}

/* ------------------------------------------------------------------ */
/* Saisie pseudo                                                        */
/* ------------------------------------------------------------------ */

void gfx_draw_pseudo_input(BITMAP *buf, const char *pseudo, int cursor)
{
    char txt[64];
    clear_to_color(buf, C_DARK);
    textout_centre_ex(buf, font, "ENTREZ VOTRE PSEUDO:", SCREEN_W/2, 200, C_GOLD, -1);
    /* Champ de saisie */
    rectfill(buf, SCREEN_W/2 - 150, 240, SCREEN_W/2 + 150, 265, makecol(20,20,60));
    rect(buf, SCREEN_W/2 - 150, 240, SCREEN_W/2 + 150, 265, C_WHITE);
    sprintf(txt, "%s", pseudo);
    textout_ex(buf, font, txt, SCREEN_W/2 - 140, 248, C_WHITE, -1);
    /* Curseur clignotant */
    if ((cursor / 30) % 2 == 0) {
        int cx2 = SCREEN_W/2 - 140 + (int)strlen(pseudo) * 8;
        vline(buf, cx2, 244, 260, C_WHITE);
    }
    textout_centre_ex(buf, font, "ENTREE pour valider", SCREEN_W/2, 300, C_LTGRAY, -1);
}

/* ------------------------------------------------------------------ */
/* Compte à rebours                                                     */
/* ------------------------------------------------------------------ */

void gfx_draw_countdown(BITMAP *buf, const GameData *g)
{
    /* Dessine le jeu en arrière-plan */
    draw_background(buf, g->current_level, g->frame);
    gfx_draw_hud(buf, g);

    /* Panneau central */
    rectfill(buf, SCREEN_W/2 - 120, SCREEN_H/2 - 60,
                  SCREEN_W/2 + 120, SCREEN_H/2 + 60, makecol(0,0,80));
    rect(buf,     SCREEN_W/2 - 120, SCREEN_H/2 - 60,
                  SCREEN_W/2 + 120, SCREEN_H/2 + 60, C_GOLD);

    char txt[16];
    if (g->countdown > 0) {
        sprintf(txt, "%d", g->countdown);
        textout_centre_ex(buf, font, txt, SCREEN_W/2, SCREEN_H/2 - 10,
                          C_YELLOW, -1);
    } else {
        textout_centre_ex(buf, font, "PRET!", SCREEN_W/2, SCREEN_H/2 - 10,
                          C_GREEN, -1);
    }

    /* Nom du niveau */
    const char *lvl_names[] = {"", "NIVEAU 1: Cookies !",
                                    "NIVEAU 2: Pizzas !",
                                    "NIVEAU 3: Burgers !",
                                    "NIVEAU 4: Le Grand Melange !",
                                    "NIVEAU 5: BOSS - Le Gateau !"};
    if (g->current_level >= 1 && g->current_level <= 5)
        textout_centre_ex(buf, font, lvl_names[g->current_level],
                          SCREEN_W/2, SCREEN_H/2 + 20, C_CYAN, -1);
}

/* ------------------------------------------------------------------ */
/* Résultat de niveau                                                   */
/* ------------------------------------------------------------------ */

void gfx_draw_level_result(BITMAP *buf, const GameData *g, int win)
{
    char txt[80];
    clear_to_color(buf, win ? makecol(0,30,0) : makecol(40,0,0));
    int c = win ? C_GREEN : C_RED;
    const char *msg = win ? "NIVEAU GAGNE !" : "NIVEAU PERDU !";
    textout_centre_ex(buf, font, msg, SCREEN_W/2, 120, c, -1);
    sprintf(txt, "Score: %d", g->score);
    textout_centre_ex(buf, font, txt, SCREEN_W/2, 160, C_GOLD, -1);

    textout_centre_ex(buf, font, "Que voulez-vous faire ?",     SCREEN_W/2, 220, C_WHITE, -1);
    textout_centre_ex(buf, font, "ENTREE : Continuer",           SCREEN_W/2, 260, C_YELLOW, -1);
    textout_centre_ex(buf, font, "S      : Sauvegarder",         SCREEN_W/2, 280, C_CYAN, -1);
    textout_centre_ex(buf, font, "ECHAP  : Revenir au menu",     SCREEN_W/2, 300, C_LTGRAY, -1);
    textout_centre_ex(buf, font, "Q      : Quitter le jeu",      SCREEN_W/2, 320, C_LTGRAY, -1);
}

/* ------------------------------------------------------------------ */
/* Victoire finale                                                      */
/* ------------------------------------------------------------------ */

void gfx_draw_game_win(BITMAP *buf, const GameData *g)
{
    int i, frame = g->frame;
    char txt[80];
    /* Fond festif */
    clear_to_color(buf, makecol(10,10,50));
    /* Confettis animés */
    int cols[] = {C_RED, C_YELLOW, C_GREEN, C_BLUE, C_PINK, C_ORANGE};
    srand(42);
    for (i = 0; i < 60; i++) {
        int cx2 = (rand() % SCREEN_W + frame * (i%3+1)) % SCREEN_W;
        int cy2 = (rand() % SCREEN_H + frame * (i%4+1)) % SCREEN_H;
        circlefill(buf, cx2, cy2, 4, cols[i % 6]);
    }
    textout_centre_ex(buf, font, "** FELICITATIONS ! **",
                      SCREEN_W/2, 100, C_GOLD, -1);
    textout_centre_ex(buf, font, "Vous avez vaincu le gateau geant !",
                      SCREEN_W/2, 140, C_WHITE, -1);
    sprintf(txt, "Score final: %d", g->score);
    textout_centre_ex(buf, font, txt, SCREEN_W/2, 180, C_YELLOW, -1);
    /* Dessine le gâteau vaincu (petit) */
    draw_cake_boss(buf, SCREEN_W/2, 300, 50, frame);
    /* Croix de mort */
    line(buf, SCREEN_W/2 - 60, 250, SCREEN_W/2 + 60, 350, C_RED);
    line(buf, SCREEN_W/2 + 60, 250, SCREEN_W/2 - 60, 350, C_RED);

    textout_centre_ex(buf, font, "ENTREE : Retour au menu",
                      SCREEN_W/2, SCREEN_H - 60, C_LTGRAY, -1);
}

/* ------------------------------------------------------------------ */
/* Écran de chargement (reprendre une partie)                           */
/* ------------------------------------------------------------------ */

void gfx_draw_load_screen(BITMAP *buf, const GameData *g,
                           int selection, const char *input, int cursor)
{
    int i;
    char txt[80];
    clear_to_color(buf, C_DARK);
    textout_centre_ex(buf, font, "== REPRENDRE UNE PARTIE ==",
                      SCREEN_W/2, 30, C_GOLD, -1);
    textout_ex(buf, font, "Entrez votre pseudo:", 40, 70, C_WHITE, -1);
    /* Champ saisie */
    rectfill(buf, 40, 90, 400, 110, makecol(20,20,60));
    rect(buf, 40, 90, 400, 110, C_WHITE);
    textout_ex(buf, font, input, 48, 96, C_WHITE, -1);
    if ((cursor / 30) % 2 == 0)
        vline(buf, 48 + (int)strlen(input) * 8, 93, 107, C_WHITE);

    textout_ex(buf, font, "Sauvegardes disponibles:", 40, 130, C_CYAN, -1);
    if (g->nb_saves == 0) {
        textout_ex(buf, font, "(aucune sauvegarde)", 40, 150, C_DKGRAY, -1);
    }
    for (i = 0; i < g->nb_saves; i++) {
        int col = (i == selection) ? C_YELLOW : C_WHITE;
        sprintf(txt, "%s   Niveau: %d   Score: %d",
                g->saves[i].pseudo, g->saves[i].level, g->saves[i].score);
        if (i == selection)
            rectfill(buf, 36, 148 + i*20, 500, 164 + i*20, makecol(60,60,120));
        textout_ex(buf, font, txt, 40, 150 + i * 20, col, -1);
    }
    textout_centre_ex(buf, font,
        "Haut/Bas pour choisir, ENTREE pour charger, ECHAP pour revenir",
        SCREEN_W/2, SCREEN_H - 40, C_LTGRAY, -1);
}
