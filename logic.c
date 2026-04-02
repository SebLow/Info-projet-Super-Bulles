/* ============================================================
   SUPER BULLES - logic.c
   Logique métier pure (sans aucune dépendance Allegro)
   ============================================================ */

#include "logic.h"
#include <math.h>
#include<main.h>

#define SAVE_FILE "saves.txt"

/* ------------------------------------------------------------------ */
/* Helpers internes                                                     */
/* ------------------------------------------------------------------ */

static FoodType food_for_level(int level, int idx)
{
    switch (level) {
        case 1: return FOOD_COOKIE;
        case 2: return FOOD_PIZZA;
        case 3: return FOOD_BURGER;
        case 4: return (FoodType)(idx % 3); /* cookie/pizza/burger */
        case 5: return FOOD_CAKE;
        default: return FOOD_COOKIE;
    }
}

static int radius_for_level(int lv)
{
    if (lv == 0) return BUBBLE_R_BIG;
    if (lv == 1) return BUBBLE_R_MED;
    return BUBBLE_R_SMALL;
}

static float rand_float(float lo, float hi)
{
    return lo + ((float)rand() / RAND_MAX) * (hi - lo);
}

/* Ajoute une bulle dans le tableau */
static void add_bubble(GameData *g, float x, float y,
                       float vx, float vy,
                       int lv, FoodType food)
{
    int i;
    if (g->nb_bubbles >= MAX_BUBBLES) return;
    for (i = 0; i < MAX_BUBBLES; i++) {
        if (!g->bubbles[i].alive) {
            Bubble *b = &g->bubbles[i];
            b->x = x; b->y = y;
            b->vx = vx; b->vy = vy;
            b->level = lv;
            b->radius = radius_for_level(lv);
            b->alive = 1;
            b->food = food;
            b->shoots_lightning = 0;
            b->lightning_timer = 0;
            b->hits_needed = (food == FOOD_CAKE) ? 1 : 1;
            g->nb_bubbles++;
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Initialisation                                                       */
/* ------------------------------------------------------------------ */

void logic_init_game(GameData *g, const char *pseudo)
{
    int i;
    memset(g, 0, sizeof(GameData));
    strncpy(g->player.pseudo, pseudo, PSEUDO_LEN - 1);
    g->player.alive = 1;
    g->player.weapon = WEAPON_BASIC;
    g->state = STATE_COUNTDOWN;
    g->current_level = 1;
    g->score = 0;
    for (i = 0; i < MAX_SAVES; i++) g->saves[i].level = 0;
}

void logic_init_level(GameData *g, int level)
{
    int i;
    float cx, cy;

    /* Reset bulles, projectiles, armes */
    memset(g->bubbles, 0, sizeof(g->bubbles));
    memset(g->projs,   0, sizeof(g->projs));
    memset(g->weapons, 0, sizeof(g->weapons));
    memset(g->lightnings, 0, sizeof(g->lightnings));
    g->nb_bubbles = 0;

    /* Joueur au centre bas */
    g->player.x = PLAY_W / 2.0f - PLAYER_W / 2.0f;
    g->player.y = PLAY_BOT - PLAYER_H - 2;
    g->player.alive = 1;
    g->player.weapon = WEAPON_BASIC;
    g->player.weapon_timer = 0;
    g->player.shoot_cooldown = 0;

    /* Timer du niveau */
    g->time_left = LEVEL_TIME;
    g->timer_ticks = 0;
    g->countdown = 3;
    g->countdown_ticks = 0;
    g->state = STATE_COUNTDOWN;
    g->current_level = level;

    cx = PLAY_W / 2.0f;
    cy = PLAY_TOP + 80.0f;

    if (level == 5) {
        /* ----- BOSS : un énorme gâteau ----- */
        g->boss_alive = 1;
        g->boss_hits  = 0;
        g->boss_max_hits = 10;
        Bubble *boss = &g->bubbles[0];
        boss->x = cx - 60;
        boss->y = (float)(PLAY_TOP + 40);
        boss->vx = 2.0f;
        boss->vy = 0.0f;
        boss->radius = 60;
        boss->level = 0;
        boss->alive = 1;
        boss->food = FOOD_CAKE;
        boss->hits_needed = g->boss_max_hits;
        boss->shoots_lightning = 1;
        boss->lightning_timer = 80;
        g->nb_bubbles = 1;
        /* Le boss génère aussi 2 petites bulles */
        add_bubble(g, cx - 120, cy, -2.5f, -4.0f, 0, FOOD_COOKIE);
        add_bubble(g, cx + 60,  cy,  2.5f, -4.0f, 0, FOOD_BURGER);
    } else {
        /* Niveaux 1-4 : bulles normales */
        int nb = 1 + level;      /* 2 à 5 grosses bulles */
        int shoot_lightning = (level >= 3);
        for (i = 0; i < nb; i++) {
            float vx = rand_float(-3.0f, 3.0f);
            if (fabsf(vx) < 1.0f) vx = 1.5f;
            float vy = -rand_float(3.0f, 6.0f);
            float x  = rand_float(60.0f, (float)(PLAY_W - 60));
            Bubble *b;
            add_bubble(g, x, cy, vx, vy, 0, food_for_level(level, i));
            b = &g->bubbles[g->nb_bubbles - 1];
            b->shoots_lightning = shoot_lightning && (i % 2 == 0);
            b->lightning_timer  = 120 + rand() % 80;
        }
        g->boss_alive = 0;
    }
}

/* ------------------------------------------------------------------ */
/* Mise à jour logique                                                  */
/* ------------------------------------------------------------------ */

void logic_update(GameData *g, int key_left, int key_right,
                  int key_shoot, float dt)
{
    int i, j;
    Player *pl = &g->player;
    (void)dt;

    g->frame++;

    /* === COUNTDOWN === */
    if (g->state == STATE_COUNTDOWN) {
        g->countdown_ticks++;
        if (g->countdown_ticks >= 60) {
            g->countdown_ticks = 0;
            g->countdown--;
            if (g->countdown <= 0) g->state = STATE_PLAY;
        }
        return;
    }

    if (g->state != STATE_PLAY) return;

    /* --- Timer du niveau --- */
    g->timer_ticks++;
    if (g->timer_ticks >= 60) {
        g->timer_ticks = 0;
        g->time_left--;
        if (g->time_left <= 0) {
            g->state = STATE_LEVEL_LOSE;
            return;
        }
    }

    /* --- Déplacement joueur --- */
    if (!pl->alive) return;
    if (key_left)  pl->x -= PLAYER_SPEED;
    if (key_right) pl->x += PLAYER_SPEED;
    if (pl->x < 0) pl->x = 0;
    if (pl->x + PLAYER_W > PLAY_W) pl->x = (float)(PLAY_W - PLAYER_W);

    /* Weapon timer */
    if (pl->weapon != WEAPON_BASIC && pl->weapon_timer > 0) {
        pl->weapon_timer--;
        if (pl->weapon_timer == 0) pl->weapon = WEAPON_BASIC;
    }
    if (pl->shoot_cooldown > 0) pl->shoot_cooldown--;

    /* --- Tir --- */
    if (key_shoot && pl->shoot_cooldown == 0) {
        int cooldown = (pl->weapon == WEAPON_DOUBLE) ? 8 : 14;
        /* Cherche un slot libre */
        int count = 0;
        for (i = 0; i < MAX_PROJ; i++) {
            if (g->projs[i].alive) count++;
        }
        /* Arme basique : 1 projectile max ; double : 2 max */
        int max_proj = (pl->weapon == WEAPON_BASIC) ? 1 :
                       (pl->weapon == WEAPON_DOUBLE) ? 2 : 3;
        if (count < max_proj) {
            /* tir central */
            for (i = 0; i < MAX_PROJ; i++) {
                if (!g->projs[i].alive) {
                    g->projs[i].x = pl->x + PLAYER_W / 2.0f - PROJ_W / 2.0f;
                    g->projs[i].y = pl->y - PROJ_H;
                    g->projs[i].alive = 1;
                    break;
                }
            }
            /* tir large : second projectile décalé */
            if (pl->weapon == WEAPON_WIDE) {
                for (i = 0; i < MAX_PROJ; i++) {
                    if (!g->projs[i].alive) {
                        g->projs[i].x = pl->x + PLAYER_W - PROJ_W;
                        g->projs[i].y = pl->y - PROJ_H;
                        g->projs[i].alive = 1;
                        break;
                    }
                }
            }
            pl->shoot_cooldown = cooldown;
        }
    }

    /* --- Déplacement projectiles --- */
    for (i = 0; i < MAX_PROJ; i++) {
        if (!g->projs[i].alive) continue;
        g->projs[i].y -= PROJ_SPEED;
        if (g->projs[i].y + PROJ_H < PLAY_TOP)
            g->projs[i].alive = 0;
    }

    /* --- Physique bulles --- */
    for (i = 0; i < MAX_BUBBLES; i++) {
        Bubble *b = &g->bubbles[i];
        if (!b->alive) continue;

        b->vy += GRAVITY;
        b->x  += b->vx;
        b->y  += b->vy;

        /* Rebond murs gauche/droite */
        if (b->x - b->radius < 0) {
            b->x = (float)b->radius;
            b->vx = fabsf(b->vx);
        }
        if (b->x + b->radius > PLAY_W) {
            b->x = (float)(PLAY_W - b->radius);
            b->vx = -fabsf(b->vx);
        }
        /* Rebond sol */
        if (b->y + b->radius >= PLAY_BOT) {
            b->y  = (float)(PLAY_BOT - b->radius);
            b->vy = -fabsf(b->vy) * BOUNCE_DAMP;
            /* Mini rebond minimal pour éviter que la bulle reste au sol */
            if (fabsf(b->vy) < 2.5f) b->vy = -4.5f;
        }
        /* Rebond plafond */
        if (b->y - b->radius < PLAY_TOP) {
            b->y  = (float)(PLAY_TOP + b->radius);
            b->vy = fabsf(b->vy) * 0.5f;
        }

        /* Boss : accélération progressive */
        if (b->food == FOOD_CAKE && g->boss_hits > 0) {
            float accel = 1.0f + 0.1f * g->boss_hits;
            if (fabsf(b->vx) < 2.0f * accel) b->vx *= 1.001f;
        }

        /* Éclairs (niveaux 3+) */
        if (b->shoots_lightning && b->alive) {
            b->lightning_timer--;
            if (b->lightning_timer <= 0) {
                b->lightning_timer = 100 + rand() % 100;
                /* Cherche slot éclair */
                for (j = 0; j < 4; j++) {
                    if (!g->lightnings[j].alive) {
                        g->lightnings[j].x = b->x;
                        g->lightnings[j].y = b->y + b->radius;
                        g->lightnings[j].alive = 1;
                        g->lightnings[j].timer = 60;
                        break;
                    }
                }
            }
        }
    }

    /* --- Déplacement éclairs --- */
    for (i = 0; i < 4; i++) {
        if (!g->lightnings[i].alive) continue;
        g->lightnings[i].y += 5.0f;
        g->lightnings[i].timer--;
        if (g->lightnings[i].y > PLAY_BOT || g->lightnings[i].timer <= 0)
            g->lightnings[i].alive = 0;

        /* Collision éclair / joueur */
        if (g->lightnings[i].alive && pl->alive) {
            float lx = g->lightnings[i].x;
            float ly = g->lightnings[i].y;
            if (lx > pl->x - 8 && lx < pl->x + PLAYER_W + 8 &&
                ly > pl->y && ly < pl->y + PLAYER_H) {
                pl->alive = 0;
                g->lightnings[i].alive = 0;
                g->state = STATE_LEVEL_LOSE;
                return;
            }
        }
    }

    /* --- Armes au sol --- */
    for (i = 0; i < MAX_WEAPONS; i++) {
        if (!g->weapons[i].alive) continue;
        g->weapons[i].timer--;
        if (g->weapons[i].timer <= 0) { g->weapons[i].alive = 0; continue; }
        /* Récupération joueur */
        if (pl->alive &&
            g->weapons[i].x < pl->x + PLAYER_W &&
            g->weapons[i].x + 20 > pl->x &&
            g->weapons[i].y < pl->y + PLAYER_H &&
            g->weapons[i].y + 20 > pl->y) {
            pl->weapon = g->weapons[i].type;
            pl->weapon_timer = 600; /* 10 secondes à 60fps */
            g->weapons[i].alive = 0;
        }
    }

    /* --- Collisions projectile / bulle --- */
    for (i = 0; i < MAX_PROJ; i++) {
        if (!g->projs[i].alive) continue;
        for (j = 0; j < MAX_BUBBLES; j++) {
            Bubble *b = &g->bubbles[j];
            if (!b->alive) continue;
            float dx = g->projs[i].x + PROJ_W / 2.0f - b->x;
            float dy = g->projs[i].y - b->y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < (float)b->radius + PROJ_W / 2.0f) {
                g->projs[i].alive = 0;

                /* Boss : compter les touches */
                if (b->food == FOOD_CAKE) {
                    g->boss_hits++;
                    g->score += 200;
                    if (g->boss_hits >= g->boss_max_hits) {
                        b->alive = 0;
                        g->boss_alive = 0;
                        g->nb_bubbles--;
                        /* Victoire */
                        int all_dead = 1;
                        int k;
                        for (k = 0; k < MAX_BUBBLES; k++)
                            if (g->bubbles[k].alive) { all_dead = 0; break; }
                        if (all_dead) { g->state = STATE_GAME_WIN; return; }
                    }
                    break;
                }

                /* Bulle normale : division */
                b->hits_needed--;
                if (b->hits_needed <= 0) {
                    b->alive = 0;
                    g->nb_bubbles--;
                    g->score += (3 - b->level) * 100;

                    /* Drop arme aléatoire (niveaux 2+, 20%) */
                    if (g->current_level >= 2 && rand() % 5 == 0) {
                        int k;
                        for (k = 0; k < MAX_WEAPONS; k++) {
                            if (!g->weapons[k].alive) {
                                g->weapons[k].x = b->x;
                                g->weapons[k].y = b->y;
                                g->weapons[k].type = (WeaponType)(1 + rand() % 2);
                                g->weapons[k].alive = 1;
                                g->weapons[k].timer = 300;
                                break;
                            }
                        }
                    }

                    /* Division en 2 bulles plus petites */
                    if (b->level < BUBBLE_LEVELS - 1) {
                        float spd = 3.0f + (float)(BUBBLE_LEVELS - b->level) * 0.5f;
                        add_bubble(g, b->x, b->y, -spd, -5.0f, b->level + 1, b->food);
                        add_bubble(g, b->x, b->y,  spd, -5.0f, b->level + 1, b->food);
                    }
                }
                break;
            }
        }
    }

    /* --- Collision bulle / joueur --- */
    for (i = 0; i < MAX_BUBBLES; i++) {
        Bubble *b = &g->bubbles[i];
        if (!b->alive || !pl->alive) continue;
        float dx = b->x - (pl->x + PLAYER_W / 2.0f);
        float dy = b->y - (pl->y + PLAYER_H / 2.0f);
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < (float)b->radius + PLAYER_W / 2.5f) {
            pl->alive = 0;
            g->state = STATE_LEVEL_LOSE;
            return;
        }
    }

    /* --- Victoire niveau : toutes les bulles mortes --- */
    if (logic_all_bubbles_dead(g)) {
        g->score += g->time_left * 10; /* bonus temps */
        if (g->current_level == 5)
            g->state = STATE_GAME_WIN;
        else
            g->state = STATE_LEVEL_WIN;
    }
}

/* ------------------------------------------------------------------ */
/* Requêtes                                                             */
/* ------------------------------------------------------------------ */

int logic_all_bubbles_dead(const GameData *g)
{
    int i;
    for (i = 0; i < MAX_BUBBLES; i++)
        if (g->bubbles[i].alive) return 0;
    return 1;
}

int logic_count_alive_bubbles(const GameData *g)
{
    int i, c = 0;
    for (i = 0; i < MAX_BUBBLES; i++)
        if (g->bubbles[i].alive) c++;
    return c;
}

/* ------------------------------------------------------------------ */
/* Sauvegarde                                                           */
/* ------------------------------------------------------------------ */

void logic_save(GameData *g)
{
    int i;
    /* Mise à jour ou ajout */
    int found = -1;
    for (i = 0; i < g->nb_saves; i++) {
        if (strcmp(g->saves[i].pseudo, g->player.pseudo) == 0) {
            found = i; break;
        }
    }
    if (found == -1 && g->nb_saves < MAX_SAVES) {
        found = g->nb_saves++;
        strncpy(g->saves[found].pseudo, g->player.pseudo, PSEUDO_LEN - 1);
    }
    if (found >= 0) {
        g->saves[found].level = g->current_level;
        g->saves[found].score = g->score;
    }

    FILE *f = fopen(SAVE_FILE, "w");
    if (!f) return;
    for (i = 0; i < g->nb_saves; i++) {
        fprintf(f, "%s %d %d\n",
                g->saves[i].pseudo,
                g->saves[i].level,
                g->saves[i].score);
    }
    fclose(f);
}

void logic_load_saves(GameData *g)
{
    FILE *f = fopen(SAVE_FILE, "r");
    g->nb_saves = 0;
    if (!f) return;
    while (g->nb_saves < MAX_SAVES) {
        SaveEntry *e = &g->saves[g->nb_saves];
        if (fscanf(f, "%19s %d %d", e->pseudo, &e->level, &e->score) != 3)
            break;
        g->nb_saves++;
    }
    fclose(f);
}

int logic_find_save(GameData *g, const char *pseudo)
{
    int i;
    for (i = 0; i < g->nb_saves; i++)
        if (strcmp(g->saves[i].pseudo, pseudo) == 0) return i;
    return -1;
}
