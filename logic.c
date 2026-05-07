#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "logique.h"

/* ============================================================
   JOUEUR
   ============================================================ */
void joueur_init(Joueur *j, const char *pseudo) {
    j->x          = JEU_LARG / 2 - 20;
    j->y          = ZONE_JEU_H - 60;
    j->vx         = 0;
    j->largeur    = 40;
    j->hauteur    = 30;
    j->arme_active = ARME_BASE;
    j->duree_arme  = 0;
    j->vivant      = 1;
    j->bouclier    = 0;
    strncpy(j->pseudo, pseudo, PSEUDO_MAX - 1);
    j->pseudo[PSEUDO_MAX - 1] = '\0';
}

void joueur_deplacer(Joueur *j, int dir, int zone_w) {
    int vitesse = 5;
    j->x += dir * vitesse;
    if (j->x < 0)              j->x = 0;
    if (j->x + j->largeur > zone_w) j->x = zone_w - j->largeur;
}

/* ============================================================
   PROJECTILES
   ============================================================ */
Projectile* projectile_creer(float x, float y, int type) {
    Projectile *p = (Projectile*)malloc(sizeof(Projectile));
    if (!p) return NULL;
    p->x      = x;
    p->y      = y;
    p->vy     = -12;
    p->actif  = 1;
    p->type   = type;
    p->suivant = NULL;
    return p;
}

void projectiles_update(Projectile **liste) {
    Projectile *p = *liste, *prev = NULL;
    while (p) {
        p->y += p->vy;
        if (p->y < 0 || !p->actif) {
            /* Supprimer ce noeud */
            Projectile *suivant = p->suivant;
            if (prev) prev->suivant = suivant;
            else       *liste       = suivant;
            free(p);
            p = suivant;
        } else {
            prev = p;
            p    = p->suivant;
        }
    }
}

void projectiles_vider(Projectile **liste) {
    Projectile *p = *liste;
    while (p) {
        Projectile *tmp = p->suivant;
        free(p);
        p = tmp;
    }
    *liste = NULL;
}

Projectile* joueur_tirer(Joueur *j) {
    float cx = j->x + j->largeur / 2.0f - 2;
    float cy = j->y;
    return projectile_creer(cx, cy, j->arme_active);
}

/* ============================================================
   ASTEROIDES
   ============================================================ */
Asteroide* asteroide_creer(float x, float y, float vx, float vy,
                            int rayon, int niveau_div,
                            int lance_eclair, int bonus_cache) {
    Asteroide *a = (Asteroide*)malloc(sizeof(Asteroide));
    if (!a) return NULL;
    a->x           = x;
    a->y           = y;
    a->vx          = vx;
    a->vy          = vy;
    a->rayon       = rayon;
    a->niveau_div  = niveau_div;
    a->lance_eclair = lance_eclair;
    a->timer_eclair = 120 + rand() % 180;
    a->bonus_cache  = bonus_cache;
    a->suivant      = NULL;
    return a;
}

void asteroides_update(Asteroide **liste, int zone_w, int zone_h) {
    Asteroide *a = *liste;
    /* Gravite simulee */
    float gravite = 0.15f;
    while (a) {
        a->vy += gravite;
        a->x  += a->vx;
        a->y  += a->vy;
        /* Rebond horizontal */
        if (a->x - a->rayon < 0) {
            a->x  = a->rayon;
            a->vx = fabsf(a->vx);
        }
        if (a->x + a->rayon > zone_w) {
            a->x  = zone_w - a->rayon;
            a->vx = -fabsf(a->vx);
        }
        /* Rebond sol */
        if (a->y + a->rayon > zone_h) {
            a->y  = zone_h - a->rayon;
            a->vy = -fabsf(a->vy) * 0.9f;
        }
        /* Plafond */
        if (a->y - a->rayon < 0) {
            a->y  = a->rayon;
            a->vy = fabsf(a->vy);
        }
        a = a->suivant;
    }
}

/* Insere un asteroide en tete de liste */
static void liste_inserer(Asteroide **liste, Asteroide *a) {
    a->suivant = *liste;
    *liste = a;
}

void asteroide_diviser(Asteroide **liste, Asteroide *a,
                        Bonus **liste_bonus) {
    if (a->niveau_div >= 3) return; /* taille minimum : disparait */

    int nouveau_rayon = a->rayon / 2;
    int nouveau_div   = a->niveau_div + 1;
    float speed_boost = 1.3f;

    /* Fils gauche */
    Asteroide *fils1 = asteroide_creer(
        a->x, a->y,
        -fabsf(a->vx) * speed_boost,
        -fabsf(a->vy) * speed_boost,
        nouveau_rayon, nouveau_div, a->lance_eclair, -1);
    /* Fils droit */
    Asteroide *fils2 = asteroide_creer(
        a->x, a->y,
        fabsf(a->vx) * speed_boost,
        -fabsf(a->vy) * speed_boost,
        nouveau_rayon, nouveau_div, a->lance_eclair, -1);

    if (fils1) liste_inserer(liste, fils1);
    if (fils2) liste_inserer(liste, fils2);

    /* Faire tomber le bonus cache */
    if (a->bonus_cache >= 0) {
        Bonus *b = (Bonus*)malloc(sizeof(Bonus));
        if (b) {
            b->x     = a->x;
            b->y     = a->y;
            b->vy    = 2.0f;
            b->type  = a->bonus_cache;
            b->actif = 1;
            b->suivant = *liste_bonus;
            *liste_bonus = b;
        }
    }
}

void asteroides_vider(Asteroide **liste) {
    Asteroide *a = *liste;
    while (a) {
        Asteroide *tmp = a->suivant;
        free(a);
        a = tmp;
    }
    *liste = NULL;
}

/* ============================================================
   ECLAIRS
   ============================================================ */
Eclair* eclair_creer(float x, float y) {
    Eclair *e = (Eclair*)malloc(sizeof(Eclair));
    if (!e) return NULL;
    e->x      = x;
    e->y      = y;
    e->vy     = 6;
    e->actif  = 1;
    e->suivant = NULL;
    return e;
}

void eclairs_update(Eclair **liste, int zone_h) {
    Eclair *e = *liste, *prev = NULL;
    while (e) {
        e->y += e->vy;
        if (e->y > zone_h || !e->actif) {
            Eclair *suivant = e->suivant;
            if (prev) prev->suivant = suivant;
            else       *liste       = suivant;
            free(e);
            e = suivant;
        } else {
            prev = e;
            e    = e->suivant;
        }
    }
}

void eclairs_vider(Eclair **liste) {
    Eclair *e = *liste;
    while (e) {
        Eclair *tmp = e->suivant;
        free(e);
        e = tmp;
    }
    *liste = NULL;
}

void asteroides_lancer_eclairs(Asteroide *liste, Eclair **eclairs) {
    Asteroide *a = liste;
    while (a) {
        if (a->lance_eclair) {
            a->timer_eclair--;
            if (a->timer_eclair <= 0) {
                Eclair *e = eclair_creer(a->x, a->y + a->rayon);
                if (e) { e->suivant = *eclairs; *eclairs = e; }
                a->timer_eclair = 120 + rand() % 180;
            }
        }
        a = a->suivant;
    }
}

/* ============================================================
   BONUS
   ============================================================ */
void bonus_update(Bonus **liste, int zone_h) {
    Bonus *b = *liste, *prev = NULL;
    while (b) {
        b->y += b->vy;
        if (b->y > zone_h || !b->actif) {
            Bonus *suivant = b->suivant;
            if (prev) prev->suivant = suivant;
            else       *liste       = suivant;
            free(b);
            b = suivant;
        } else {
            prev = b;
            b    = b->suivant;
        }
    }
}

void bonus_vider(Bonus **liste) {
    Bonus *b = *liste;
    while (b) {
        Bonus *tmp = b->suivant;
        free(b);
        b = tmp;
    }
    *liste = NULL;
}

/* ============================================================
   BOSS
   ============================================================ */
void boss_init(Boss *b, int zone_w) {
    b->x           = zone_w / 2 - 80;
    b->y           = 40;
    b->largeur     = 160;
    b->hauteur     = 80;
    b->hp_max      = 15;
    b->hp_actuel   = 15;
    b->vitesse_base = 1.5f;
    b->vx          = b->vitesse_base;
    b->vivant      = 1;
    b->timer_spawn = 0;
    b->frame_touche = 0;
}

void boss_update(Boss *b, int zone_w, Asteroide **asteroides) {
    if (!b->vivant) return;

    /* Vitesse augmente a chaque degat */
    float vitesse = b->vitesse_base +
                    (b->hp_max - b->hp_actuel) * 0.5f;
    b->x += (b->vx > 0 ? vitesse : -vitesse);

    if (b->x < 0)                       { b->x = 0;                b->vx = vitesse; }
    if (b->x + b->largeur > zone_w)     { b->x = zone_w-b->largeur; b->vx = -vitesse; }

    /* Flash degat */
    if (b->frame_touche > 0) b->frame_touche--;

    /* Spawn d'asteroides */
    b->timer_spawn++;
    if (b->timer_spawn >= 180) {
        b->timer_spawn = 0;
        float spawn_vx = (rand() % 2 == 0) ? 2.5f : -2.5f;
        Asteroide *a = asteroide_creer(
            b->x + b->largeur / 2, b->y + b->hauteur,
            spawn_vx, 2.0f, RAYON_MOYEN, 1, 0, -1);
        if (a) { a->suivant = *asteroides; *asteroides = a; }
    }
}

/* ============================================================
   COLLISIONS
   ============================================================ */
int collision_cercle_rect(float cx, float cy, int r,
                           float rx, float ry, int rw, int rh) {
    float closest_x = cx < rx ? rx : (cx > rx+rw ? rx+rw : cx);
    float closest_y = cy < ry ? ry : (cy > ry+rh ? ry+rh : cy);
    float dx = cx - closest_x;
    float dy = cy - closest_y;
    return (dx*dx + dy*dy) < (float)(r*r);
}

int collision_rect_rect(float x1, float y1, int w1, int h1,
                         float x2, float y2, int w2, int h2) {
    return !(x1+w1 < x2 || x2+w2 < x1 ||
             y1+h1 < y2 || y2+h2 < y1);
}

void collisions_projectiles_asteroides(Projectile **projs,
                                        Asteroide **asts,
                                        Bonus **bonus,
                                        int *score) {
    Projectile *p = *projs;
    while (p) {
        if (!p->actif) { p = p->suivant; continue; }
        Asteroide *a = *asts, *prev_a = NULL;
        while (a) {
            if (collision_cercle_rect(a->x, a->y, a->rayon,
                                       p->x, p->y, 4, 12)) {
                /* Supprimer l'asteroide */
                if (prev_a) prev_a->suivant = a->suivant;
                else         *asts          = a->suivant;
                /* Points selon la taille */
                *score += (4 - a->niveau_div) * 100;
                /* Diviser si possible */
                if (a->niveau_div < 3)
                    asteroide_diviser(asts, a, bonus);
                free(a);
                a = (prev_a ? prev_a->suivant : *asts);
                /* Desactiver le projectile */
                p->actif = 0;
                break;
            } else {
                prev_a = a;
                a      = a->suivant;
            }
        }
        p = p->suivant;
    }
}

void collisions_projectiles_boss(Projectile **projs, Boss *b, int *score) {
    if (!b->vivant) return;
    Projectile *p = *projs;
    while (p) {
        if (p->actif && collision_rect_rect(p->x, p->y, 4, 12,
                                             b->x, b->y,
                                             b->largeur, b->hauteur)) {
            p->actif = 0;
            b->hp_actuel--;
            b->frame_touche = 15;
            *score += 500;
            if (b->hp_actuel <= 0) { b->vivant = 0; *score += 5000; }
        }
        p = p->suivant;
    }
}

void collisions_asteroide_joueur(Asteroide *asts, Joueur *j) {
    if (!j->vivant || j->bouclier) return;
    Asteroide *a = asts;
    while (a) {
        if (collision_cercle_rect(a->x, a->y, a->rayon,
                                   j->x, j->y, j->largeur, j->hauteur))
            j->vivant = 0;
        a = a->suivant;
    }
}

void collisions_eclair_joueur(Eclair *eclairs, Joueur *j) {
    if (!j->vivant || j->bouclier) return;
    Eclair *e = eclairs;
    while (e) {
        if (e->actif &&
            collision_rect_rect(e->x-2, e->y, 4, 20,
                                 j->x, j->y, j->largeur, j->hauteur))
            j->vivant = 0;
        e = e->suivant;
    }
}

void collisions_boss_joueur(Boss *b, Joueur *j) {
    if (!b->vivant || !j->vivant || j->bouclier) return;
    if (collision_rect_rect(b->x, b->y, b->largeur, b->hauteur,
                             j->x, j->y, j->largeur, j->hauteur))
        j->vivant = 0;
}

void collisions_bonus_joueur(Bonus **bonus, Joueur *j) {
    Bonus *b = *bonus;
    while (b) {
        if (b->actif &&
            collision_rect_rect(b->x-10, b->y-10, 20, 20,
                                 j->x, j->y, j->largeur, j->hauteur)) {
            switch (b->type) {
                case BONUS_LASER:
                    j->arme_active = ARME_LASER;
                    j->duree_arme  = 600;
                    break;
                case BONUS_BOMBE:
                    j->arme_active = ARME_BOMBE;
                    j->duree_arme  = 300;
                    break;
                case BONUS_BOUCLIER:
                    j->bouclier = 300;
                    break;
            }
            b->actif = 0;
        }
        b = b->suivant;
    }
}

/* ============================================================
   NIVEAU
   ============================================================ */
/* Placement initial des asteroides selon le numero de niveau */
static void niveau_placer_asteroides(Niveau *n) {
    int nb_gros   = 1 + n->numero;
    int eclair    = (n->numero >= 3) ? 1 : 0;
    int bonus_idx = 0;
    int i;
    for (i = 0; i < nb_gros; i++) {
        float x  = 80 + (i * (JEU_LARG - 160) / nb_gros);
        float y  = 80 + rand() % 100;
        float vx = (rand() % 2 == 0) ? 2.0f + n->numero*0.3f
                                     : -(2.0f + n->numero*0.3f);
        float vy = 1.5f;
        int   bc = (bonus_idx < 2) ? bonus_idx++ : -1;
        Asteroide *a = asteroide_creer(x, y, vx, vy,
                                        RAYON_GROS, 0, eclair, bc);
        if (a) { a->suivant = n->liste_asteroides;
                 n->liste_asteroides = a; }
    }
}

void niveau_init(Niveau *n, int numero, Joueur *j) {
    n->numero          = numero;
    n->temps_restant   = 90 - (numero - 1) * 10;
    n->timer_ticks     = 0;
    n->score_courant   = 0;
    n->boss_actif      = (numero == 4) ? 1 : 0;
    n->liste_asteroides  = NULL;
    n->liste_projectiles = NULL;
    n->liste_bonus       = NULL;
    n->liste_eclairs     = NULL;

    joueur_init(j, j->pseudo);

    if (!n->boss_actif)
        niveau_placer_asteroides(n);
    else
        boss_init(&n->boss, JEU_LARG);
}

void niveau_update(Niveau *n, Joueur *j) {
    /* Decompte temps */
    n->timer_ticks++;
    if (n->timer_ticks >= FPS) {
        n->timer_ticks = 0;
        n->temps_restant--;
    }

    /* Expiration arme */
    if (j->duree_arme > 0) {
        j->duree_arme--;
        if (j->duree_arme == 0) j->arme_active = ARME_BASE;
    }
    /* Expiration bouclier */
    if (j->bouclier > 0) j->bouclier--;

    asteroides_update(&n->liste_asteroides, JEU_LARG, ZONE_JEU_H);
    projectiles_update(&n->liste_projectiles);
    bonus_update(&n->liste_bonus, ZONE_JEU_H);

    if (n->numero >= 3)
        asteroides_lancer_eclairs(n->liste_asteroides, &n->liste_eclairs);
    eclairs_update(&n->liste_eclairs, ZONE_JEU_H);

    if (n->boss_actif)
        boss_update(&n->boss, JEU_LARG, &n->liste_asteroides);

    /* Collisions */
    collisions_projectiles_asteroides(&n->liste_projectiles,
                                       &n->liste_asteroides,
                                       &n->liste_bonus,
                                       &n->score_courant);
    if (n->boss_actif)
        collisions_projectiles_boss(&n->liste_projectiles,
                                     &n->boss, &n->score_courant);
    collisions_asteroide_joueur(n->liste_asteroides, j);
    collisions_eclair_joueur(n->liste_eclairs, j);
    if (n->boss_actif) collisions_boss_joueur(&n->boss, j);
    collisions_bonus_joueur(&n->liste_bonus, j);
}

void niveau_vider(Niveau *n) {
    asteroides_vider(&n->liste_asteroides);
    projectiles_vider(&n->liste_projectiles);
    bonus_vider(&n->liste_bonus);
    eclairs_vider(&n->liste_eclairs);
}

int niveau_gagne(Niveau *n) {
    if (n->boss_actif) return !n->boss.vivant;
    return (n->liste_asteroides == NULL);
}

int niveau_perdu(Niveau *n, Joueur *j) {
    return (!j->vivant || n->temps_restant <= 0);
}

/* ============================================================
   SAUVEGARDE
   ============================================================ */
int sauvegarde_ecrire(const char *pseudo, int niveau, int score) {
    /* Lit toutes les sauvegardes, met a jour ou ajoute */
    Sauvegarde sauves[64];
    int nb = 0;
    FILE *f = fopen(SAVE_FILE, "r");
    if (f) {
        while (nb < 64 &&
               fscanf(f, "%31[^;];%d;%d\n",
                      sauves[nb].pseudo,
                      &sauves[nb].dernier_niveau,
                      &sauves[nb].score) == 3)
            nb++;
        fclose(f);
    }
    /* Cherche si pseudo existe */
    int idx = -1, i;
    for (i = 0; i < nb; i++)
        if (strcmp(sauves[i].pseudo, pseudo) == 0) { idx = i; break; }
    if (idx < 0 && nb < 64) idx = nb++;
    if (idx < 0) return 0;
    strncpy(sauves[idx].pseudo, pseudo, PSEUDO_MAX-1);
    sauves[idx].dernier_niveau = niveau;
    sauves[idx].score          = score;
    f = fopen(SAVE_FILE, "w");
    if (!f) return 0;
    for (i = 0; i < nb; i++)
        fprintf(f, "%s;%d;%d\n", sauves[i].pseudo,
                sauves[i].dernier_niveau, sauves[i].score);
    fclose(f);
    return 1;
}

int sauvegarde_lire(const char *pseudo, Sauvegarde *s) {
    FILE *f = fopen(SAVE_FILE, "r");
    if (!f) return 0;
    char buf_pseudo[PSEUDO_MAX];
    int  niv, sc;
    while (fscanf(f, "%31[^;];%d;%d\n", buf_pseudo, &niv, &sc) == 3) {
        if (strcmp(buf_pseudo, pseudo) == 0) {
            strncpy(s->pseudo, pseudo, PSEUDO_MAX-1);
            s->dernier_niveau = niv;
            s->score          = sc;
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}
