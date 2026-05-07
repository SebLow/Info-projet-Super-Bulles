#include <allegro.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "entities.h"
#include "logique.h"
#include "affichage.h"
#include "ihm.h"

/* ============================================================
   ETATS DU JEU
   ============================================================ */
typedef enum {
    ETAT_MENU = 0,
    ETAT_SAISIE_PSEUDO,
    ETAT_COUNTDOWN,
    ETAT_JEU,
    ETAT_VICTOIRE_NIVEAU,
    ETAT_DEFAITE_NIVEAU,
    ETAT_VICTOIRE_PARTIE,
    ETAT_GAME_OVER,
    ETAT_QUITTER
} EtatJeu;

/* ============================================================
   TIMER FPS (callback Allegro)
   ============================================================ */
static volatile int tick = 0;
static void incrementer_tick(void) { tick++; }
END_OF_FUNCTION(incrementer_tick)

/* ============================================================
   REGLES DU JEU
   ============================================================ */
static void afficher_regles(void) {
    clear_to_color(buffer, makecol(5, 5, 20));
    int y = 30;
    textout_centre_ex(buffer, font, "=== REGLES DU JEU ===",
                      JEU_LARG/2, y, makecol(255,230,50), -1); y+=30;
    textout_ex(buffer, font,
        "- Detruisez tous les asteroides de chaque niveau.",
        20, y, makecol(255,255,255), -1); y+=20;
    textout_ex(buffer, font,
        "- Fleches : deplacer le Faucon Millenium.",
        20, y, makecol(255,255,255), -1); y+=20;
    textout_ex(buffer, font,
        "- Espace : tirer.",
        20, y, makecol(255,255,255), -1); y+=20;
    textout_ex(buffer, font,
        "- Une bulle touchee se divise en bulles plus petites.",
        20, y, makecol(255,255,255), -1); y+=20;
    textout_ex(buffer, font,
        "- Ne pas toucher les asteroides ni les eclairs !",
        20, y, makecol(220,40,40), -1); y+=20;
    textout_ex(buffer, font,
        "- Niveau 3 : des eclairs tombent des asteroides.",
        20, y, makecol(255,230,50), -1); y+=20;
    textout_ex(buffer, font,
        "- Niveau 4 : affrontez le Destroyer Stellaire !",
        20, y, makecol(220,40,40), -1); y+=20;
    textout_ex(buffer, font,
        "- Recuperez les bonus (L=laser, B=bombe, S=bouclier).",
        20, y, makecol(60,220,80), -1); y+=30;
    textout_centre_ex(buffer, font, "Appuyez sur une touche pour revenir...",
                      JEU_LARG/2, y+20, makecol(150,150,150), -1);
    affichage_flip();
    ihm_attendre_touche();
}

/* ============================================================
   BOUCLE PRINCIPALE
   ============================================================ */
int main(void) {
    srand((unsigned)time(NULL));

    if (!affichage_init()) {
        allegro_message("Erreur initialisation graphique !");
        return 1;
    }

    /* Timer FPS */
    LOCK_FUNCTION(incrementer_tick);
    LOCK_VARIABLE(tick);
    install_int_ex(incrementer_tick, BPS_TO_TIMER(FPS));

    /* Variables de jeu */
    EtatJeu  etat        = ETAT_MENU;
    int      selection   = 1;   /* 0=regles,1=nouvelle,2=reprendre,3=quitter */
    int      score_total = 0;
    int      niveau_num  = 1;
    int      countdown   = 3 * FPS;  /* 3 secondes en ticks */
    int      delay_menu  = 0;   /* anti-rebond apres ecran */
    int      score_sauve = 0;

    Joueur j;
    memset(&j, 0, sizeof(j));
    strcpy(j.pseudo, "PILOTE");

    Niveau n;
    memset(&n, 0, sizeof(n));

    /* Pseudo buffer */
    char   pseudo_buf[PSEUDO_MAX] = {0};
    int    pseudo_len = 0;
    int    reprendre  = 0;  /* 1 si mode reprise */

    while (etat != ETAT_QUITTER) {
        /* Attendre le prochain tick */
        while (!tick) { /* busy-wait leger */ }
        tick = 0;

        switch (etat) {

        /* -------------------------------------------------- */
        case ETAT_MENU:
            ecran_menu(buffer, selection);
            affichage_flip();

            if (delay_menu > 0) { delay_menu--; break; }

            if (ihm_menu_haut() && selection > 0) { selection--; rest(120); }
            if (ihm_menu_bas()  && selection < 3) { selection++; rest(120); }
            if (ihm_valider()) {
                switch (selection) {
                    case 0: afficher_regles(); break;
                    case 1:
                        reprendre = 0;
                        memset(pseudo_buf, 0, PSEUDO_MAX);
                        pseudo_len = 0;
                        etat = ETAT_SAISIE_PSEUDO;
                        break;
                    case 2:
                        reprendre = 1;
                        memset(pseudo_buf, 0, PSEUDO_MAX);
                        pseudo_len = 0;
                        etat = ETAT_SAISIE_PSEUDO;
                        break;
                    case 3:
                        etat = ETAT_QUITTER;
                        break;
                }
                rest(200);
            }
            break;

        /* -------------------------------------------------- */
        case ETAT_SAISIE_PSEUDO:
            ecran_saisie_pseudo(buffer, pseudo_buf, pseudo_len);
            affichage_flip();

            if (ihm_saisie_pseudo(pseudo_buf, &pseudo_len, PSEUDO_MAX)) {
                strncpy(j.pseudo, pseudo_buf, PSEUDO_MAX-1);
                if (reprendre) {
                    Sauvegarde sv;
                    if (sauvegarde_lire(j.pseudo, &sv)) {
                        niveau_num  = sv.dernier_niveau;
                        score_total = sv.score;
                    } else {
                        niveau_num  = 1;
                        score_total = 0;
                    }
                } else {
                    niveau_num  = 1;
                    score_total = 0;
                }
                niveau_init(&n, niveau_num, &j);
                countdown = 3 * FPS;
                etat = ETAT_COUNTDOWN;
            }
            break;

        /* -------------------------------------------------- */
        case ETAT_COUNTDOWN:
            dessiner_fond(niveau_num);
            dessiner_joueur(buffer, &j);
            ecran_countdown(buffer, countdown / FPS + 1);
            affichage_flip();
            countdown--;
            if (countdown <= 0) etat = ETAT_JEU;
            break;

        /* -------------------------------------------------- */
        case ETAT_JEU: {
            /* Entrees */
            int dir = ihm_direction();
            joueur_deplacer(&j, dir, JEU_LARG);

            if (ihm_tir()) {
                /* Arme de base : un seul projectile a la fois */
                if (j.arme_active == ARME_BASE) {
                    if (n.liste_projectiles == NULL) {
                        Projectile *p = joueur_tirer(&j);
                        if (p) { p->suivant = n.liste_projectiles;
                                 n.liste_projectiles = p; }
                    }
                } else {
                    /* Armes evoluees : tir libre */
                    Projectile *p = joueur_tirer(&j);
                    if (p) { p->suivant = n.liste_projectiles;
                             n.liste_projectiles = p; }
                }
            }

            /* Logique */
            niveau_update(&n, &j);

            /* Dessin */
            dessiner_fond(niveau_num);
            dessiner_asteroides(buffer, n.liste_asteroides);
            dessiner_projectiles(buffer, n.liste_projectiles);
            dessiner_bonus(buffer, n.liste_bonus);
            dessiner_eclairs(buffer, n.liste_eclairs);
            if (n.boss_actif) dessiner_boss(buffer, &n.boss);
            dessiner_joueur(buffer, &j);
            dessiner_hud(buffer, &j, n.temps_restant,
                         score_total + n.score_courant, niveau_num);
            affichage_flip();

            /* Transitions */
            if (niveau_gagne(&n)) {
                score_total += n.score_courant;
                niveau_vider(&n);
                etat = ETAT_VICTOIRE_NIVEAU;
                delay_menu = FPS;
            } else if (niveau_perdu(&n, &j)) {
                niveau_vider(&n);
                etat = ETAT_DEFAITE_NIVEAU;
                delay_menu = FPS;
            }
            if (ihm_quitter_jeu()) {
                niveau_vider(&n);
                etat = ETAT_MENU;
                delay_menu = FPS / 2;
            }
            break;
        }

        /* -------------------------------------------------- */
        case ETAT_VICTOIRE_NIVEAU:
            if (delay_menu > 0) { delay_menu--; break; }
            ecran_victoire(buffer, score_total);
            affichage_flip();
            ihm_attendre_touche();

            if (niveau_num < 4) {
                niveau_num++;
                niveau_init(&n, niveau_num, &j);
                countdown = 3 * FPS;
                etat = ETAT_COUNTDOWN;
            } else {
                /* Partie gagnee ! */
                etat = ETAT_VICTOIRE_PARTIE;
            }
            break;

        /* -------------------------------------------------- */
        case ETAT_DEFAITE_NIVEAU:
            if (delay_menu > 0) { delay_menu--; break; }
            ecran_defaite(buffer);
            affichage_flip();
            ihm_attendre_touche();
            /* Sauvegarder et proposer menu */
            sauvegarde_ecrire(j.pseudo, niveau_num, score_total);
            etat = ETAT_MENU;
            delay_menu = FPS / 2;
            break;

        /* -------------------------------------------------- */
        case ETAT_VICTOIRE_PARTIE:
            clear_to_color(buffer, makecol(0, 0, 0));
            textout_centre_ex(buffer, font,
                "*** VICTOIRE ! LA GALAXIE EST SAUVEE ! ***",
                JEU_LARG/2, 180, makecol(255,230,50), -1);
            {
                char txt[64];
                sprintf(txt, "Score final : %d", score_total);
                textout_centre_ex(buffer, font, txt,
                    JEU_LARG/2, 220, makecol(60,220,80), -1);
            }
            textout_centre_ex(buffer, font,
                "Appuyez sur une touche...",
                JEU_LARG/2, 270, makecol(200,200,200), -1);
            affichage_flip();
            ihm_attendre_touche();
            sauvegarde_ecrire(j.pseudo, 4, score_total);
            score_sauve = score_total;
            (void)score_sauve;
            etat = ETAT_MENU;
            delay_menu = FPS / 2;
            niveau_num = 1;
            score_total = 0;
            break;

        /* -------------------------------------------------- */
        default:
            break;
        }
    }

    affichage_quitter();
    return 0;
}
END_OF_MAIN()
