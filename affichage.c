#include <allegro.h>
#include <stdio.h>
#include <time.h>
#include "affichage.h"

BITMAP *buffer = NULL;

int COL_NOIR, COL_BLANC, COL_ROUGE, COL_JAUNE,
    COL_BLEU, COL_VERT, COL_GRIS, COL_ORANGE;

/* Couleurs par niveau pour le fond */
static int FONDS[5][3] = {
    {  5,  5, 20},   /* niveau 0 (inutilise) */
    {  5,  5, 20},   /* niveau 1 : espace profond */
    { 10,  0, 30},   /* niveau 2 : nebuleuse violette */
    {  0, 10, 25},   /* niveau 3 : secteur vert */
    { 20,  0,  0},   /* niveau 4 : zone rouge boss */
};

void couleurs_init(void) {
    COL_NOIR   = makecol(0,   0,   0);
    COL_BLANC  = makecol(255, 255, 255);
    COL_ROUGE  = makecol(220,  40,  40);
    COL_JAUNE  = makecol(255, 230,  50);
    COL_BLEU   = makecol( 60, 140, 255);
    COL_VERT   = makecol( 50, 220,  80);
    COL_GRIS   = makecol(150, 150, 150);
    COL_ORANGE = makecol(255, 140,  20);
}

int affichage_init(void) {
    allegro_init();
    install_keyboard();
    install_timer();
    set_color_depth(32);
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, JEU_LARG, JEU_HAUT, 0, 0) != 0)
        return 0;
    set_window_title("Super Bulles — Star Wars Edition");
    couleurs_init();
    buffer = create_bitmap(JEU_LARG, JEU_HAUT);
    if (!buffer) return 0;
    return 1;
}

void affichage_quitter(void) {
    if (buffer) { destroy_bitmap(buffer); buffer = NULL; }
    allegro_exit();
}

void affichage_flip(void) {
    blit(buffer, screen, 0, 0, 0, 0, JEU_LARG, JEU_HAUT);
}

/* ---- Fond etoile ---- */
static void dessiner_etoiles(BITMAP *buf) {
    /* Etoiles fixes generees avec un seed stable */
    srand(42);
    int i;
    for (i = 0; i < 120; i++) {
        int x  = rand() % JEU_LARG;
        int y  = rand() % ZONE_JEU_H;
        int br = 100 + rand() % 156;
        putpixel(buf, x, y, makecol(br, br, br));
    }
    srand((unsigned)time(NULL));
}

void dessiner_fond(int niveau_numero) {
    int n = (niveau_numero >= 1 && niveau_numero <= 4) ? niveau_numero : 1;
    int r = FONDS[n][0], g = FONDS[n][1], b = FONDS[n][2];
    clear_to_color(buffer, makecol(r, g, b));
    dessiner_etoiles(buffer);
    /* Ligne de separation HUD */
    hline(buffer, 0, ZONE_JEU_H, JEU_LARG - 1,
          makecol(60, 60, 100));
    rectfill(buffer, 0, ZONE_JEU_H+1, JEU_LARG-1, JEU_HAUT-1,
             makecol(10, 10, 30));
}

/* ---- Joueur (Falcon simplifie) ---- */
void dessiner_joueur(BITMAP *buf, const Joueur *j) {
    if (!j->vivant) return;
    int x = (int)j->x, y = (int)j->y;
    int w = j->largeur,  h = j->hauteur;
    int cx = x + w/2;

    /* Corps principal : ellipse grise */
    ellipse(buf, cx, y + h/2, w/2, h/3, COL_GRIS);
    ellipsefill(buf, cx, y + h/2, w/2 - 2, h/3 - 2,
                makecol(80, 80, 90));

    /* Cockpit : petit cercle avant */
    circlefill(buf, cx, y + 4, 5, makecol(60, 160, 220));

    /* Antenne / canon */
    line(buf, cx - 3, y, cx - 3, y - 8, COL_BLANC);
    line(buf, cx + 5, y, cx + 5, y - 6, COL_BLANC);

    /* Moteurs : deux petits rectangles arriere */
    rectfill(buf, cx - 14, y + h - 6, cx - 8, y + h + 2,
             makecol(255, 160, 40));
    rectfill(buf, cx +  8, y + h - 6, cx + 14, y + h + 2,
             makecol(255, 160, 40));

    /* Bouclier */
    if (j->bouclier > 0)
        ellipse(buf, cx, y + h/2, w/2 + 6, h/2 + 6,
                makecol(60, 200, 255));
}

/* ---- Asteroide ---- */
void dessiner_asteroide(BITMAP *buf, const Asteroide *a) {
    int x = (int)a->x, y = (int)a->y, r = a->rayon;
    int col_bord, col_interieur;
    switch (a->niveau_div) {
        case 0: col_bord = makecol(180,120, 60);
                col_interieur = makecol(120, 80, 40); break;
        case 1: col_bord = makecol(160,100, 50);
                col_interieur = makecol(100, 65, 30); break;
        case 2: col_bord = makecol(140, 80, 40);
                col_interieur = makecol( 80, 50, 20); break;
        default:col_bord = makecol(120, 60, 30);
                col_interieur = makecol( 60, 30, 10); break;
    }
    circlefill(buf, x, y, r,     col_interieur);
    circle    (buf, x, y, r,     col_bord);
    circle    (buf, x, y, r - 2, makecol(80, 55, 25));
    /* Crateres */
    circlefill(buf, x - r/3, y - r/4, r/4, makecol(60, 40, 20));
    circlefill(buf, x + r/4, y + r/3, r/5, makecol(60, 40, 20));
}

void dessiner_asteroides(BITMAP *buf, const Asteroide *liste) {
    const Asteroide *a = liste;
    while (a) { dessiner_asteroide(buf, a); a = a->suivant; }
}

/* ---- Projectile ---- */
void dessiner_projectile(BITMAP *buf, const Projectile *p) {
    int x = (int)p->x, y = (int)p->y;
    if (p->type == ARME_BASE) {
        rectfill(buf, x, y, x+3, y+12, COL_JAUNE);
    } else if (p->type == ARME_LASER) {
        rectfill(buf, x-1, y, x+4, y+16, COL_ROUGE);
        line(buf, x+1, y, x+1, y+16, COL_ORANGE);
    } else { /* bombe */
        circlefill(buf, x+4, y+4, 5, COL_ORANGE);
        circle    (buf, x+4, y+4, 5, COL_ROUGE);
    }
}

void dessiner_projectiles(BITMAP *buf, const Projectile *liste) {
    const Projectile *p = liste;
    while (p) { if (p->actif) dessiner_projectile(buf, p); p = p->suivant; }
}

/* ---- Bonus ---- */
void dessiner_bonus(BITMAP *buf, const Bonus *liste) {
    const Bonus *b = liste;
    while (b) {
        if (b->actif) {
            int x = (int)b->x, y = (int)b->y;
            int col;
            switch (b->type) {
                case BONUS_LASER:   col = COL_ROUGE; break;
                case BONUS_BOMBE:   col = COL_ORANGE; break;
                default:            col = COL_BLEU;   break;
            }
            rectfill(buf, x-10, y-10, x+10, y+10, col);
            rect    (buf, x-10, y-10, x+10, y+10, COL_BLANC);
            /* Lettre indicatrice */
            char lettre[2] = {'L', '\0'};
            if (b->type == BONUS_BOMBE)    lettre[0] = 'B';
            if (b->type == BONUS_BOUCLIER) lettre[0] = 'S';
            textout_centre_ex(buf, font, lettre, x, y-4, COL_BLANC, -1);
        }
        b = b->suivant;
    }
}

/* ---- Eclairs ---- */
void dessiner_eclairs(BITMAP *buf, const Eclair *liste) {
    const Eclair *e = liste;
    while (e) {
        if (e->actif) {
            int x = (int)e->x, y = (int)e->y;
            /* Ligne brisee imitant un eclair */
            line(buf, x,   y,      x+4, y+8,  COL_JAUNE);
            line(buf, x+4, y+8,    x-2, y+14, COL_JAUNE);
            line(buf, x-2, y+14,   x+2, y+20, COL_BLANC);
        }
        e = e->suivant;
    }
}

/* ---- Boss (Destroyer Stellaire) ---- */
void dessiner_boss(BITMAP *buf, const Boss *b) {
    if (!b->vivant) return;
    int x = (int)b->x, y = (int)b->y;
    int w = b->largeur, h = b->hauteur;
    int cx = x + w/2;

    int col_coque = (b->frame_touche > 0)
                    ? makecol(255, 100, 100)
                    : makecol(160, 165, 170);
    /* Corps triangulaire */
    triangle(buf, cx, y, x, y+h, x+w, y+h, makecol(80, 85, 90));
    /* Dessus de la coque */
    triangle(buf, cx, y+5, x+10, y+h-5, x+w-10, y+h-5, col_coque);
    /* Tour de commandement */
    rectfill(buf, cx-12, y+10, cx+12, y+35, makecol(120,125,130));
    rect    (buf, cx-12, y+10, cx+12, y+35, makecol(200,205,210));
    /* Canons */
    rectfill(buf, cx-30, y+8,  cx-20, y+18, makecol(80,85,90));
    rectfill(buf, cx+20, y+8,  cx+30, y+18, makecol(80,85,90));
    line(buf, cx-25, y+8,  cx-25, y,    makecol(100,105,110));
    line(buf, cx+25, y+8,  cx+25, y,    makecol(100,105,110));

    /* Barre de vie */
    int barre_w = w;
    int hp_w = barre_w * b->hp_actuel / b->hp_max;
    rect    (buf, x, y-14, x+barre_w, y-8, COL_BLANC);
    rectfill(buf, x+1, y-13, x+hp_w, y-9,  COL_ROUGE);
    textout_ex(buf, font, "BOSS HP", x, y-24, COL_BLANC, -1);
}

/* ---- HUD ---- */
void dessiner_hud(BITMAP *buf, const Joueur *j,
                   int temps, int score, int niveau_num) {
    char txt[64];
    int y0 = ZONE_JEU_H + 10;

    sprintf(txt, "PILOTE: %s", j->pseudo);
    textout_ex(buf, font, txt, 10, y0, COL_BLANC, -1);

    sprintf(txt, "SCORE: %d", score);
    textout_ex(buf, font, txt, 10, y0 + 16, COL_JAUNE, -1);

    sprintf(txt, "TEMPS: %d", temps);
    int col_temps = (temps <= 10) ? COL_ROUGE : COL_BLANC;
    textout_centre_ex(buf, font, txt, JEU_LARG/2, y0 + 8, col_temps, -1);

    sprintf(txt, "NIVEAU: %d", niveau_num);
    textout_ex(buf, font, txt, JEU_LARG - 100, y0, COL_VERT, -1);

    /* Affichage arme active */
    if (j->arme_active != ARME_BASE) {
        const char *nom = (j->arme_active == ARME_LASER) ? "LASER" : "BOMBE";
        sprintf(txt, "ARME: %s (%d)", nom, j->duree_arme / FPS + 1);
        textout_ex(buf, font, txt, JEU_LARG - 120, y0 + 16, COL_ORANGE, -1);
    }
    if (j->bouclier > 0) {
        sprintf(txt, "BOUCLIER: %d", j->bouclier / FPS + 1);
        textout_ex(buf, font, txt, JEU_LARG - 150, y0 + 32, COL_BLEU, -1);
    }
}

/* ---- Ecrans ---- */
void ecran_menu(BITMAP *buf, int selection) {
    clear_to_color(buf, makecol(5, 5, 20));
    dessiner_etoiles(buf);

    textout_centre_ex(buf, font, "=== SUPER BULLES : STAR WARS ===",
                      JEU_LARG/2, 80, COL_JAUNE, -1);
    textout_centre_ex(buf, font, "Protegez la galaxie !",
                      JEU_LARG/2, 100, COL_GRIS, -1);

    const char *items[] = {
        "1. Regles du jeu",
        "2. Nouvelle partie",
        "3. Reprendre une partie",
        "4. Quitter"
    };
    int i;
    for (i = 0; i < 4; i++) {
        int col = (i == selection) ? COL_JAUNE : COL_BLANC;
        if (i == selection)
            rectfill(buf, JEU_LARG/2 - 120, 180 + i*40 - 2,
                          JEU_LARG/2 + 120, 180 + i*40 + 14,
                          makecol(30, 30, 60));
        textout_centre_ex(buf, font, items[i],
                          JEU_LARG/2, 180 + i*40, col, -1);
    }
    textout_centre_ex(buf, font, "Fleches : naviguer | Entree : valider",
                      JEU_LARG/2, JEU_HAUT - 30, COL_GRIS, -1);
}

void ecran_countdown(BITMAP *buf, int compte) {
    char txt[8];
    sprintf(txt, "%d", compte);
    textout_centre_ex(buf, font, txt,
                      JEU_LARG/2, JEU_HAUT/2 - 20,
                      COL_JAUNE, -1);
    if (compte == 0)
        textout_centre_ex(buf, font, "GO !",
                          JEU_LARG/2, JEU_HAUT/2 + 10,
                          COL_VERT, -1);
}

void ecran_victoire(BITMAP *buf, int score) {
    char txt[64];
    rectfill(buf, 0, 0, JEU_LARG-1, JEU_HAUT-1, makecol(0,0,0));
    textout_centre_ex(buf, font, "NIVEAU REMPORTE !",
                      JEU_LARG/2, 200, COL_VERT, -1);
    sprintf(txt, "Score : %d", score);
    textout_centre_ex(buf, font, txt, JEU_LARG/2, 230, COL_JAUNE, -1);
    textout_centre_ex(buf, font, "Appuyez sur une touche...",
                      JEU_LARG/2, 280, COL_BLANC, -1);
}

void ecran_defaite(BITMAP *buf) {
    rectfill(buf, 0, 0, JEU_LARG-1, JEU_HAUT-1, makecol(0,0,0));
    textout_centre_ex(buf, font, "LEVEL FAILED",
                      JEU_LARG/2, 200, COL_ROUGE, -1);
    textout_centre_ex(buf, font, "Appuyez sur une touche...",
                      JEU_LARG/2, 250, COL_BLANC, -1);
}

void ecran_game_over(BITMAP *buf, int score) {
    char txt[64];
    rectfill(buf, 0, 0, JEU_LARG-1, JEU_HAUT-1, makecol(0,0,0));
    textout_centre_ex(buf, font, "GAME OVER",
                      JEU_LARG/2, 180, COL_ROUGE, -1);
    sprintf(txt, "Score final : %d", score);
    textout_centre_ex(buf, font, txt, JEU_LARG/2, 220, COL_JAUNE, -1);
    textout_centre_ex(buf, font, "Appuyez sur une touche...",
                      JEU_LARG/2, 270, COL_BLANC, -1);
}

void ecran_saisie_pseudo(BITMAP *buf, const char *pseudo, int len) {
    (void)len;
    clear_to_color(buf, makecol(5, 5, 20));
    dessiner_etoiles(buf);
    textout_centre_ex(buf, font, "ENTREZ VOTRE PSEUDO :",
                      JEU_LARG/2, 200, COL_BLANC, -1);
    char affiche[PSEUDO_MAX + 2];
    snprintf(affiche, sizeof(affiche), "%s_", pseudo);
    rectfill(buf, JEU_LARG/2 - 100, 225, JEU_LARG/2 + 100, 245,
             makecol(30, 30, 60));
    textout_centre_ex(buf, font, affiche,
                      JEU_LARG/2, 230, COL_JAUNE, -1);
    textout_centre_ex(buf, font, "Entree pour valider",
                      JEU_LARG/2, 280, COL_GRIS, -1);
}

