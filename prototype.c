/* ============================================================
   FICHIER : game.c
   ============================================================ */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "entities.h"
#include "game.h"

int circle_rect_collision(float cx, float cy, int r,
                          float rx, float ry, int rw, int rh) {
    float nearX = cx < rx ? rx : (cx > rx+rw ? rx+rw : cx);
    float nearY = cy < ry ? ry : (cy > ry+rh ? ry+rh : cy);
    float ddx = cx - nearX, ddy = cy - nearY;
    return (ddx*ddx + ddy*ddy) <= (float)(r*r);
}

int enemies_remaining(GameState *gs) {
    int n = 0;
    for (int i = 0; i < gs->enemy_count; i++)
        if (gs->enemies[i].alive) n++;
    return n;
}

void init_stars(GameState *gs) {
    for (int i = 0; i < MAX_STARS; i++) {
        gs->stars[i].x          = rand() % WIN_W;
        gs->stars[i].y          = rand() % GAME_ZONE_H;
        gs->stars[i].brightness = 100 + rand() % 156;
    }
}

void init_game_state(GameState *gs, int level, const char *pseudo) {
    memset(gs, 0, sizeof(GameState));
    gs->level     = level;
    gs->time_left = 90 + level * 10;
    gs->countdown = 3 * 60;
    gs->has_boss  = (level == 4);
    strncpy(gs->pseudo, pseudo, 31);

    gs->player.x           = 60;
    gs->player.y           = GAME_ZONE_H / 2 - PLAYER_H / 2;
    gs->player.alive        = 1;
    gs->player.weapon_timer = 0;
    gs->player.weapon_type  = WEAPON_NONE;
    gs->player.dir          = DIR_RIGHT;

    init_stars(gs);
    spawn_level_enemies(gs);
}

void spawn_level_enemies(GameState *gs) {
    gs->enemy_count = 0;
    int n = gs->level + 1;
    if (n > 5) n = 5;

    for (int i = 0; i < n; i++) {
        Enemy *e  = &gs->enemies[gs->enemy_count++];
        e->x      = WIN_W - 100 - (i % 3) * 150;
        e->y      = 80 + i * 90;
        e->radius = TIE_LARGE;
        e->alive  = 1;
        e->dx     = (i % 2 == 0) ? -1.5f : 1.5f;
        e->dy     = (i % 3 == 0) ?  1.0f : -1.0f;
        e->can_lightning = (gs->level >= 3 && i == 0) ? 1 : 0;
    }

    if (gs->has_boss) {
        gs->boss.x         = WIN_W - 120;
        gs->boss.y         = GAME_ZONE_H / 2;
        gs->boss.dx        = -1.5f;
        gs->boss.dy        = 1.0f;
        gs->boss.alive     = 1;
        gs->boss.hits_left = 10;
    }
}

/* tir directionnel selon gs->player.dir */
void shoot(GameState *gs) {
    float sx = 0, sy = 0;
    switch (gs->player.dir) {
        case DIR_RIGHT:      sx =  PROJ_SPEED; sy =  0;            break;
        case DIR_LEFT:       sx = -PROJ_SPEED; sy =  0;            break;
        case DIR_UP:         sx =  0;           sy = -PROJ_SPEED;  break;
        case DIR_DOWN:       sx =  0;           sy =  PROJ_SPEED;  break;
        case DIR_UP_RIGHT:   sx =  PROJ_SPEED*0.7f; sy = -PROJ_SPEED*0.7f; break;
        case DIR_DOWN_RIGHT: sx =  PROJ_SPEED*0.7f; sy =  PROJ_SPEED*0.7f; break;
        case DIR_UP_LEFT:    sx = -PROJ_SPEED*0.7f; sy = -PROJ_SPEED*0.7f; break;
        case DIR_DOWN_LEFT:  sx = -PROJ_SPEED*0.7f; sy =  PROJ_SPEED*0.7f; break;
    }

    int slots = (gs->player.weapon_type == WEAPON_DOUBLE) ? 2 : 1;
    int placed = 0;
    for (int i = 0; i < MAX_PROJECTILES && placed < slots; i++) {
        if (!gs->projectiles[i].active) {
            gs->projectiles[i].active = 1;
            gs->projectiles[i].dx     = sx;
            gs->projectiles[i].dy     = sy;
            gs->projectiles[i].x      = gs->player.x + PLAYER_W / 2;
            gs->projectiles[i].y      = gs->player.y + PLAYER_H / 2 + placed * 8;
            placed++;
        }
    }
}

void update_game(GameState *gs, int move_left, int move_right,
                 int move_up, int move_down, int fire) {
    if (gs->game_over || gs->level_won) return;
    if (gs->countdown > 0) { gs->countdown--; return; }

    gs->frame++;

    if (gs->frame % 60 == 0 && gs->time_left > 0) gs->time_left--;
    if (gs->time_left == 0) { gs->game_over = 1; return; }

    /* déplacement + mise à jour direction */
    int any_move = 0;
    if (move_left  && gs->player.x > 0)               { gs->player.x -= PLAYER_SPEED; any_move |= 1; }
    if (move_right && gs->player.x < WIN_W - PLAYER_W) { gs->player.x += PLAYER_SPEED; any_move |= 2; }
    if (move_up    && gs->player.y > 0)               { gs->player.y -= PLAYER_SPEED; any_move |= 4; }
    if (move_down  && gs->player.y < GAME_ZONE_H - PLAYER_H) { gs->player.y += PLAYER_SPEED; any_move |= 8; }

    /* direction du tir = combinaison des touches */
    if      ((any_move & 2) && (any_move & 4)) gs->player.dir = DIR_UP_RIGHT;
    else if ((any_move & 2) && (any_move & 8)) gs->player.dir = DIR_DOWN_RIGHT;
    else if ((any_move & 1) && (any_move & 4)) gs->player.dir = DIR_UP_LEFT;
    else if ((any_move & 1) && (any_move & 8)) gs->player.dir = DIR_DOWN_LEFT;
    else if (any_move & 2)                     gs->player.dir = DIR_RIGHT;
    else if (any_move & 1)                     gs->player.dir = DIR_LEFT;
    else if (any_move & 4)                     gs->player.dir = DIR_UP;
    else if (any_move & 8)                     gs->player.dir = DIR_DOWN;
    /* sinon on garde la dernière direction */

    /* tir continu */
    if (fire) shoot(gs);

    if (gs->player.weapon_timer > 0) {
        gs->player.weapon_timer--;
        if (gs->player.weapon_timer == 0)
            gs->player.weapon_type = WEAPON_NONE;
    }

    update_enemies(gs);
    update_projectiles(gs);
    update_weapons(gs);
    update_explosions(gs);
    if (gs->level >= 3) update_lightning(gs);
    if (gs->has_boss)   update_boss(gs);
    check_collisions(gs);

    if (enemies_remaining(gs) == 0) {
        if (gs->has_boss && gs->boss.alive) return;
        gs->level_won = 1;
    }
}

void update_enemies(GameState *gs) {
    for (int i = 0; i < gs->enemy_count; i++) {
        Enemy *e = &gs->enemies[i];
        if (!e->alive) continue;

        e->dy += GRAVITY * 0.3f;
        e->x  += e->dx;
        e->y  += e->dy;

        if (e->x - e->radius < 0)           { e->x = e->radius;           e->dx =  fabsf(e->dx); }
        if (e->x + e->radius > WIN_W)        { e->x = WIN_W - e->radius;   e->dx = -fabsf(e->dx); }
        if (e->y + e->radius > GAME_ZONE_H)  {
            e->y  = GAME_ZONE_H - e->radius;
            e->dy = -fabsf(e->dy) * 0.85f;
            if (fabsf(e->dy) < 3) e->dy = -3;
        }
        if (e->y - e->radius < 0) { e->y = e->radius; e->dy = fabsf(e->dy); }

        if (e->can_lightning && gs->frame % 150 == 0) {
            for (int j = 0; j < 8; j++) {
                if (!gs->lightnings[j].active) {
                    gs->lightnings[j].active = 1;
                    gs->lightnings[j].x      = e->x - e->radius;
                    gs->lightnings[j].y      = e->y;
                    gs->lightnings[j].timer  = 80;
                    break;
                }
            }
        }
    }
}

void update_projectiles(GameState *gs) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!gs->projectiles[i].active) continue;
        gs->projectiles[i].x += gs->projectiles[i].dx;
        gs->projectiles[i].y += gs->projectiles[i].dy;
        /* désactiver si hors écran */
        if (gs->projectiles[i].x > WIN_W || gs->projectiles[i].x < 0 ||
            gs->projectiles[i].y > GAME_ZONE_H || gs->projectiles[i].y < 0)
            gs->projectiles[i].active = 0;
    }
}

void update_weapons(GameState *gs) {
    for (int i = 0; i < 4; i++) {
        if (!gs->weapons[i].active) continue;
        gs->weapons[i].timer--;
        if (gs->weapons[i].timer <= 0) gs->weapons[i].active = 0;
    }
}

void update_lightning(GameState *gs) {
    for (int i = 0; i < 8; i++) {
        if (!gs->lightnings[i].active) continue;
        gs->lightnings[i].x -= 8;
        gs->lightnings[i].timer--;
        if (gs->lightnings[i].timer <= 0 || gs->lightnings[i].x < 0)
            gs->lightnings[i].active = 0;
    }
}

void update_boss(GameState *gs) {
    Boss *b = &gs->boss;
    if (!b->alive) return;
    float speed = 1.5f + (10 - b->hits_left) * 0.35f;
    b->x += b->dx * speed; b->y += b->dy * speed;
    if (b->x < WIN_W/2)        b->dx =  fabsf(b->dx);
    if (b->x > WIN_W - 80)    b->dx = -fabsf(b->dx);
    if (b->y < 60)             b->dy =  fabsf(b->dy);
    if (b->y > GAME_ZONE_H-60) b->dy = -fabsf(b->dy);

    if (gs->frame % 180 == 0 && gs->enemy_count < MAX_ENEMIES-1) {
        Enemy *ne = &gs->enemies[gs->enemy_count++];
        ne->x = b->x - 60; ne->y = b->y;
        ne->radius = TIE_SMALL; ne->alive = 1;
        ne->dx = -2.0f; ne->dy = (gs->frame%360==0) ? 1.5f : -1.5f;
        ne->can_lightning = 0;
    }
}

void update_explosions(GameState *gs) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!gs->explosions[i].active) continue;
        gs->explosions[i].timer--;
        gs->explosions[i].radius += 2;
        if (gs->explosions[i].timer <= 0) gs->explosions[i].active = 0;
    }
}

static void add_explosion(GameState *gs, float x, float y) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!gs->explosions[i].active) {
            gs->explosions[i].active = 1; gs->explosions[i].x = x;
            gs->explosions[i].y = y; gs->explosions[i].timer = 20;
            gs->explosions[i].radius = 5; break;
        }
    }
}

void check_collisions(GameState *gs) {
    Player *p = &gs->player;

    for (int pi = 0; pi < MAX_PROJECTILES; pi++) {
        Projectile *pr = &gs->projectiles[pi];
        if (!pr->active) continue;
        for (int ei = 0; ei < gs->enemy_count; ei++) {
            Enemy *e = &gs->enemies[ei];
            if (!e->alive) continue;
            if (circle_rect_collision(e->x, e->y, e->radius,
                                      pr->x, pr->y, PROJ_W, PROJ_H)) {
                pr->active = 0;
                add_explosion(gs, e->x, e->y);
                split_enemy(gs, ei);
                gs->score += 100 * (TIE_LARGE / e->radius);
                break;
            }
        }
    }

    for (int ei = 0; ei < gs->enemy_count; ei++) {
        Enemy *e = &gs->enemies[ei];
        if (!e->alive) continue;
        if (circle_rect_collision(e->x, e->y, e->radius,
                                  p->x, p->y, PLAYER_W, PLAYER_H)) {
            add_explosion(gs, p->x+PLAYER_W/2, p->y+PLAYER_H/2);
            p->alive = 0; gs->game_over = 1; return;
        }
    }

    for (int i = 0; i < 8; i++) {
        if (!gs->lightnings[i].active) continue;
        if (gs->lightnings[i].x >= p->x && gs->lightnings[i].x <= p->x+PLAYER_W &&
            gs->lightnings[i].y >= p->y && gs->lightnings[i].y <= p->y+PLAYER_H) {
            add_explosion(gs, p->x+PLAYER_W/2, p->y+PLAYER_H/2);
            p->alive = 0; gs->game_over = 1; return;
        }
    }

    for (int i = 0; i < 4; i++) {
        WeaponPickup *w = &gs->weapons[i];
        if (!w->active) continue;
        if (w->x >= p->x && w->x <= p->x+PLAYER_W &&
            w->y >= p->y && w->y <= p->y+PLAYER_H) {
            p->weapon_type = w->type; p->weapon_timer = 600; w->active = 0;
        }
    }

    if (gs->has_boss && gs->boss.alive) {
        if (circle_rect_collision(gs->boss.x, gs->boss.y, 70,
                                  p->x, p->y, PLAYER_W, PLAYER_H)) {
            add_explosion(gs, p->x+PLAYER_W/2, p->y+PLAYER_H/2);
            p->alive = 0; gs->game_over = 1; return;
        }
        for (int pi = 0; pi < MAX_PROJECTILES; pi++) {
            Projectile *pr = &gs->projectiles[pi];
            if (!pr->active) continue;
            if (circle_rect_collision(gs->boss.x, gs->boss.y, 70,
                                      pr->x, pr->y, PROJ_W, PROJ_H)) {
                pr->active = 0; gs->boss.hits_left--; gs->score += 500;
                add_explosion(gs, gs->boss.x, gs->boss.y);
                if (gs->boss.hits_left <= 0) gs->boss.alive = 0;
            }
        }
    }
}

void split_enemy(GameState *gs, int idx) {
    Enemy *e = &gs->enemies[idx];
    e->alive = 0;
    int new_r = 0;
    if      (e->radius == TIE_LARGE)  new_r = TIE_MEDIUM;
    else if (e->radius == TIE_MEDIUM) new_r = TIE_SMALL;
    else if (e->radius == TIE_SMALL)  new_r = TIE_TINY;
    else return;

    if (gs->level >= 2 && gs->frame % 4 == 0) {
        for (int i = 0; i < 4; i++)
            if (!gs->weapons[i].active) {
                gs->weapons[i].active = 1; gs->weapons[i].x = e->x;
                gs->weapons[i].y = e->y; gs->weapons[i].type = WEAPON_FAST;
                gs->weapons[i].timer = 300; break;
            }
    }

    for (int side = -1; side <= 1; side += 2) {
        if (gs->enemy_count >= MAX_ENEMIES) break;
        Enemy *ne = &gs->enemies[gs->enemy_count++];
        ne->x = e->x; ne->y = e->y; ne->radius = new_r; ne->alive = 1;
        ne->dx = side * (2.5f + (float)(TIE_LARGE - new_r) / 20.0f);
        ne->dy = -3.5f; ne->can_lightning = 0;
    }
}


/* ============================================================
   FICHIER : display.c
   ============================================================ */
#include <allegro.h>
#include <stdio.h>
#include <math.h>
#include "entities.h"
#include "display.h"

BITMAP *falcon_sprite = NULL;

static void draw_stars(BITMAP *buf, Star *stars) {
    for (int i = 0; i < MAX_STARS; i++) {
        int b = stars[i].brightness;
        putpixel(buf,(int)stars[i].x,(int)stars[i].y,makecol(b,b,b));
        if (b > 200) putpixel(buf,(int)stars[i].x+1,(int)stars[i].y,makecol(b,b,b));
    }
}

/* Faucon : utilise le sprite si chargé, sinon dessin géométrique amélioré */
static void draw_falcon(BITMAP *buf, Player *p) {
    int x=(int)p->x, y=(int)p->y;

    if (falcon_sprite) {
        /* dessine le sprite avec transparence (couleur de fond = gris clair 200,200,200) */
        draw_sprite(buf, falcon_sprite, x, y);
    } else {
        /* fallback géométrique */
        int cx = x+PLAYER_W/2, cy = y+PLAYER_H/2;
        ellipsefill(buf,cx,cy,PLAYER_W/2,PLAYER_H/2-5,makecol(180,180,180));
        ellipse    (buf,cx,cy,PLAYER_W/2,PLAYER_H/2-5,makecol(80,80,80));
        circlefill (buf,cx-PLAYER_W/4,cy,8,makecol(100,200,255));
        rectfill   (buf,cx+PLAYER_W/2-5,cy-5,cx+PLAYER_W/2+8,cy+5,makecol(0,150,255));
    }

    /* indicateur direction tir */
    int cx = x+PLAYER_W/2, cy = y+PLAYER_H/2;
    int ax=cx, ay=cy;
    switch (p->dir) {
        case DIR_RIGHT:      ax+=18; break;
        case DIR_LEFT:       ax-=18; break;
        case DIR_UP:         ay-=18; break;
        case DIR_DOWN:       ay+=18; break;
        case DIR_UP_RIGHT:   ax+=13; ay-=13; break;
        case DIR_DOWN_RIGHT: ax+=13; ay+=13; break;
        case DIR_UP_LEFT:    ax-=13; ay-=13; break;
        case DIR_DOWN_LEFT:  ax-=13; ay+=13; break;
    }
    /* petite flèche verte indiquant la direction */
    line(buf, cx, cy, ax, ay, makecol(0,255,100));
    circlefill(buf, ax, ay, 3, makecol(0,255,100));
}

static void draw_tie(BITMAP *buf, Enemy *e) {
    int x=(int)e->x, y=(int)e->y, r=e->radius;
    int hull, wing, detail;
    if      (r==TIE_LARGE)  { hull=makecol(70,70,80);  wing=makecol(0,70,150);  detail=makecol(130,130,180); }
    else if (r==TIE_MEDIUM) { hull=makecol(80,80,90);  wing=makecol(0,90,170);  detail=makecol(140,140,190); }
    else if (r==TIE_SMALL)  { hull=makecol(90,90,100); wing=makecol(0,110,190); detail=makecol(150,150,200); }
    else                     { hull=makecol(100,100,110);wing=makecol(0,130,210);detail=makecol(160,160,210); }

    int wr = r + r/2;
    rectfill(buf,x-wr,y-r,  x-r/2,y+r,  wing);
    rectfill(buf,x+r/2,y-r, x+wr, y+r,  wing);
    rect    (buf,x-wr,y-r,  x-r/2,y+r,  detail);
    rect    (buf,x+r/2,y-r, x+wr, y+r,  detail);
    hline(buf,x-wr,y,x-r/2,detail); hline(buf,x+r/2,y,x+wr,detail);
    vline(buf,x-wr+wr/3,y-r,y+r,detail); vline(buf,x+r/2+wr/3,y-r,y+r,detail);
    circlefill(buf,x,y,r/2,hull); circle(buf,x,y,r/2,detail);
    circlefill(buf,x,y,r/4,makecol(180,40,40)); circle(buf,x,y,r/4,makecol(230,80,80));
}

static void draw_deathstar(BITMAP *buf, Boss *boss) {
    int x=(int)boss->x,y=(int)boss->y,r=70;
    circlefill(buf,x,y,r,makecol(120,120,120));
    circle    (buf,x,y,r,makecol(70,70,70));
    hline(buf,x-r,y,x+r,makecol(70,70,70));
    rect (buf,x-r,y-5,x+r,y+5,makecol(70,70,70));
    circlefill(buf,x-25,y-20,18,makecol(50,50,50));
    circlefill(buf,x-25,y-20,10,makecol(20,20,20));
    circlefill(buf,x-25,y-20,5, makecol(0,200,100));
    for (int i=-2;i<=2;i++) hline(buf,x-r+5,y+i*11,x+r-5,makecol(80,80,80));
    int bar_w=boss->hits_left*14;
    rectfill(buf,x-70,y-r-18,x-70+bar_w,y-r-8,makecol(0,220,0));
    rect    (buf,x-70,y-r-18,x+70,      y-r-8,makecol(200,200,200));
    textout_ex(buf,font,"ETOILE DE LA MORT",x-68,y-r-17,makecol(0,0,0),-1);
}

static void draw_explosions(BITMAP *buf, Explosion *expl) {
    for (int i=0;i<MAX_EXPLOSIONS;i++) {
        if (!expl[i].active) continue;
        int r=expl[i].radius, t=expl[i].timer;
        int bright = t*10; if(bright>200) bright=200;
        circlefill(buf,(int)expl[i].x,(int)expl[i].y,r,   makecol(255,bright,0));
        circlefill(buf,(int)expl[i].x,(int)expl[i].y,r/2, makecol(255,80,0));
        circlefill(buf,(int)expl[i].x,(int)expl[i].y,r/4, makecol(255,255,200));
    }
}

/* dessin laser selon direction */
static void draw_laser(BITMAP *buf, Projectile *pr) {
    int x=(int)pr->x, y=(int)pr->y;
    float len = 18.0f;
    float nx = pr->dx / PROJ_SPEED, ny = pr->dy / PROJ_SPEED;
    /* normalise */
    float mag = sqrtf(nx*nx+ny*ny); if(mag<0.01f) mag=0.01f;
    nx/=mag; ny/=mag;
    int x2 = x + (int)(nx*len), y2 = y + (int)(ny*len);
    line(buf, x, y, x2, y2, makecol(0,255,80));
    line(buf, x+1, y, x2+1, y2, makecol(150,255,150));
}

void display_game(BITMAP *buf, GameState *gs) {
    rectfill(buf,0,0,WIN_W,GAME_ZONE_H,makecol(0,0,0));
    /* nébuleuse */
    for (int py=0;py<GAME_ZONE_H;py+=6)
        hline(buf,0,py,WIN_W,makecol(0,gs->level*2,gs->level*5));
    draw_stars(buf,gs->stars);

    for (int i=0;i<gs->enemy_count;i++)
        if (gs->enemies[i].alive) draw_tie(buf,&gs->enemies[i]);

    for (int i=0;i<MAX_PROJECTILES;i++)
        if (gs->projectiles[i].active) draw_laser(buf,&gs->projectiles[i]);

    for (int i=0;i<4;i++)
        if (gs->weapons[i].active) {
            circlefill(buf,(int)gs->weapons[i].x,(int)gs->weapons[i].y,
                       10,makecol(255,200,0));
            circle    (buf,(int)gs->weapons[i].x,(int)gs->weapons[i].y,
                       10,makecol(255,255,0));
        }

    for (int i=0;i<8;i++)
        if (gs->lightnings[i].active) {
            int lx=(int)gs->lightnings[i].x,ly=(int)gs->lightnings[i].y;
            rectfill(buf,lx,ly-2,lx+28,ly+2,makecol(255,30,30));
            hline(buf,lx,ly,lx+28,makecol(255,150,150));
        }

    draw_explosions(buf,gs->explosions);
    if (gs->has_boss && gs->boss.alive) draw_deathstar(buf,&gs->boss);
    if (gs->player.alive) draw_falcon(buf,&gs->player);

    /* barre info */
    rectfill(buf,0,INFO_ZONE_Y,WIN_W,WIN_H,makecol(5,5,20));
    hline(buf,0,INFO_ZONE_Y,WIN_W,makecol(0,150,255));

    char txt[64];
    sprintf(txt,"Pilote: %s",gs->pseudo);
    textout_ex(buf,font,txt,8,INFO_ZONE_Y+8,makecol(0,200,255),-1);
    sprintf(txt,"Score: %d",gs->score);
    textout_ex(buf,font,txt,230,INFO_ZONE_Y+8,makecol(255,220,0),-1);
    sprintf(txt,"Temps: %d",gs->time_left);
    textout_ex(buf,font,txt,420,INFO_ZONE_Y+8,
               gs->time_left<20?makecol(255,80,80):makecol(100,255,100),-1);
    sprintf(txt,"Secteur: %d",gs->level);
    textout_ex(buf,font,txt,610,INFO_ZONE_Y+8,makecol(180,180,255),-1);

    /* direction tir */
    const char *dir_names[]={"->","<-","^","v","^->","v->","^<-","v<-"};
    sprintf(txt,"Tir: %s",dir_names[gs->player.dir]);
    textout_ex(buf,font,txt,8,INFO_ZONE_Y+24,makecol(0,255,100),-1);

    if (gs->player.weapon_type!=WEAPON_NONE) {
        sprintf(txt,"TURBO: %ds",gs->player.weapon_timer/60);
        textout_ex(buf,font,txt,200,INFO_ZONE_Y+24,makecol(255,200,0),-1);
    }
    textout_ex(buf,font,"Fleches:deplacer  ESPACE:tirer",
               WIN_W-260,INFO_ZONE_Y+24,makecol(60,60,100),-1);
}

void display_countdown(BITMAP *buf, GameState *gs) {
    display_game(buf,gs);
    int sec=gs->countdown/60+1; char txt[8]; sprintf(txt,"%d",sec);
    textout_centre_ex(buf,font,txt,WIN_W/2,GAME_ZONE_H/2-20,makecol(255,220,0),-1);
    textout_centre_ex(buf,font,"QUE LA FORCE SOIT AVEC VOUS",
                      WIN_W/2,GAME_ZONE_H/2+15,makecol(200,200,200),-1);
}

void display_menu(BITMAP *buf, int selected, Star *stars) {
    rectfill(buf,0,0,WIN_W,WIN_H,makecol(0,0,0));
    if (stars) draw_stars(buf,stars);
    textout_centre_ex(buf,font,"STAR  WARS",WIN_W/2,55,makecol(255,220,0),-1);
    textout_centre_ex(buf,font,"FAUCON MILLENIUM VS EMPIRE",WIN_W/2,85,makecol(180,180,180),-1);
    textout_centre_ex(buf,font,"ECE Paris - 2025/2026",WIN_W/2,110,makecol(80,80,80),-1);
    const char *items[]={"1. Regles du jeu","2. Nouvelle partie","3. Reprendre une partie","4. Quitter"};
    for (int i=0;i<4;i++)
        textout_centre_ex(buf,font,items[i],WIN_W/2,230+i*55,
                          (i==selected)?makecol(255,220,0):makecol(170,170,170),-1);
    textout_centre_ex(buf,font,"Fleches + Entree",WIN_W/2,475,makecol(70,70,70),-1);
}

void display_game_over(BITMAP *buf, GameState *gs) {
    rectfill(buf,0,0,WIN_W,WIN_H,makecol(8,0,0));
    textout_centre_ex(buf,font,"L EMPIRE A GAGNE...",WIN_W/2,175,makecol(255,50,50),-1);
    textout_centre_ex(buf,font,"LE FAUCON MILLENIUM EST DETRUIT",WIN_W/2,215,makecol(200,100,100),-1);
    char txt[64]; sprintf(txt,"Score final : %d",gs->score);
    textout_centre_ex(buf,font,txt,WIN_W/2,275,makecol(255,255,255),-1);
    textout_centre_ex(buf,font,"ENTREE = Recommencer   ECHAP = Menu",WIN_W/2,355,makecol(170,170,170),-1);
}

void display_level_won(BITMAP *buf, GameState *gs) {
    rectfill(buf,0,0,WIN_W,WIN_H,makecol(0,8,0));
    textout_centre_ex(buf,font,"SECTEUR LIBERE !",WIN_W/2,175,makecol(100,255,100),-1);
    char txt[64]; sprintf(txt,"Score : %d",gs->score);
    textout_centre_ex(buf,font,txt,WIN_W/2,235,makecol(255,255,255),-1);
    textout_centre_ex(buf,font,"ENTREE = Secteur suivant   ECHAP = Menu",WIN_W/2,355,makecol(170,170,170),-1);
}

void display_victory(BITMAP *buf, GameState *gs) {
    rectfill(buf,0,0,WIN_W,WIN_H,makecol(0,0,8));
    textout_centre_ex(buf,font,"L ETOILE DE LA MORT EST DETRUITE !",WIN_W/2,145,makecol(255,220,0),-1);
    textout_centre_ex(buf,font,"QUE LA FORCE SOIT AVEC VOUS",WIN_W/2,195,makecol(255,255,255),-1);
    char txt[64]; sprintf(txt,"Score final : %d",gs->score);
    textout_centre_ex(buf,font,txt,WIN_W/2,265,makecol(100,255,100),-1);
    textout_centre_ex(buf,font,"ENTREE = Menu",WIN_W/2,355,makecol(170,170,170),-1);
}

void display_rules(BITMAP *buf) {
    rectfill(buf,0,0,WIN_W,WIN_H,makecol(0,0,0));
    const char *lines[]={
        "REGLES - STAR WARS","",
        "- Pilotez le Faucon Millenium",
        "- Detruisez tous les TIE Fighters",
        "- Les TIE se divisent quand touches",
        "- Le tir suit votre direction de deplacement",
        "- Secteur 3+ : evitez les lasers rouges",
        "- Secteur 4  : detruisez l Etoile de la Mort",
        "- Ramassez les bonus pour le turbo laser","",
        "Fleches : deplacement + direction du tir",
        "ESPACE  : tirer (continu)","ECHAP : menu","",
        "ECHAP pour revenir"
    };
    for (int i=0;i<15;i++)
        textout_centre_ex(buf,font,lines[i],WIN_W/2,50+i*27,makecol(190,190,190),-1);
}

void display_pseudo_input(BITMAP *buf, const char *pseudo) {
    rectfill(buf,0,0,WIN_W,WIN_H,makecol(0,0,0));
    textout_centre_ex(buf,font,"IDENTIFICATION DU PILOTE",WIN_W/2,195,makecol(255,220,0),-1);
    rect(buf,WIN_W/2-130,252,WIN_W/2+130,282,makecol(0,150,255));
    char txt[64]; sprintf(txt,"%s_",pseudo);
    textout_centre_ex(buf,font,txt,WIN_W/2,260,makecol(255,220,0),-1);
    textout_centre_ex(buf,font,"ENTREE pour valider",WIN_W/2,312,makecol(110,110,110),-1);
}


/* ============================================================
   FICHIER : input.c
   ============================================================ */
#include <allegro.h>
#include "input.h"

void input_update(InputState *inp) {
    inp->move_left  = key[KEY_LEFT];
    inp->move_right = key[KEY_RIGHT];
    inp->move_up    = key[KEY_UP];
    inp->move_down  = key[KEY_DOWN];
    inp->fire       = key[KEY_SPACE];
    inp->escape     = key[KEY_ESC];
    inp->enter      = key[KEY_ENTER];
    inp->up         = key[KEY_UP];
    inp->down       = key[KEY_DOWN];
}


/* ============================================================
   FICHIER : save.c
   ============================================================ */
#include <stdio.h>
#include <string.h>
#include "save.h"

void list_saves(SaveEntry *entries, int *count) {
    *count=0;
    FILE *f=fopen(SAVE_FILE,"r");
    if(!f) return;
    while(*count<MAX_SAVES &&
          fscanf(f,"%31s %d %d",
                 entries[*count].pseudo,
                 &entries[*count].level,
                 &entries[*count].score)==3) (*count)++;
    fclose(f);
}

int save_game(const char *pseudo, int level, int score) {
    SaveEntry entries[MAX_SAVES]; int count=0;
    list_saves(entries,&count);
    for(int i=0;i<count;i++)
        if(strcmp(entries[i].pseudo,pseudo)==0){
            entries[i].level=level; entries[i].score=score; goto write;
        }
    if(count<MAX_SAVES){
        strncpy(entries[count].pseudo,pseudo,31);
        entries[count].level=level; entries[count].score=score; count++;
    }
write:;
    FILE *f=fopen(SAVE_FILE,"w"); if(!f) return 0;
    for(int i=0;i<count;i++)
        fprintf(f,"%s %d %d\n",entries[i].pseudo,entries[i].level,entries[i].score);
    fclose(f); return 1;
}

int load_game(const char *pseudo, int *level, int *score) {
    SaveEntry entries[MAX_SAVES]; int count=0;
    list_saves(entries,&count);
    for(int i=0;i<count;i++)
        if(strcmp(entries[i].pseudo,pseudo)==0){
            *level=entries[i].level; *score=entries[i].score; return 1;
        }
    return 0;
}


/* ============================================================
   FICHIER : main.c
   ============================================================ */
#include <allegro.h>
#include <stdio.h>
#include <string.h>
#include "entities.h"
#include "game.h"
#include "display.h"
#include "input.h"
#include "save.h"

typedef enum {
    STATE_MENU, STATE_RULES, STATE_PSEUDO_INPUT, STATE_PSEUDO_LOAD,
    STATE_COUNTDOWN, STATE_PLAYING, STATE_GAME_OVER,
    STATE_LEVEL_WON, STATE_VICTORY, STATE_QUIT
} AppState;

int main(void) {
    allegro_init();
    install_keyboard();
    set_color_depth(24);
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, WIN_W, WIN_H, 0, 0);

    /* charge le sprite du Faucon */
    falcon_sprite = load_bitmap("falcon.bmp", NULL);
    /* si NULL : le fallback géométrique sera utilisé */

    BITMAP *buffer = create_bitmap(WIN_W, WIN_H);
    GameState  gs;
    InputState inp;
    AppState   state    = STATE_MENU;
    int        menu_sel = 1;
    char       pseudo[32] = "";
    int        pseudo_len = 0;

    GameState menu_gs;
    memset(&menu_gs,0,sizeof(menu_gs));
    init_stars(&menu_gs);

    while (state != STATE_QUIT) {
        input_update(&inp);

        switch (state) {

        case STATE_MENU:
            if (inp.up)   { menu_sel=(menu_sel+3)%4; rest(120); }
            if (inp.down) { menu_sel=(menu_sel+1)%4; rest(120); }
            if (inp.enter) {
                rest(150);
                if      (menu_sel==0) state=STATE_RULES;
                else if (menu_sel==1) { pseudo[0]='\0'; pseudo_len=0; state=STATE_PSEUDO_INPUT; }
                else if (menu_sel==2) { pseudo[0]='\0'; pseudo_len=0; state=STATE_PSEUDO_LOAD; }
                else                  state=STATE_QUIT;
            }
            for (int i=0;i<MAX_STARS;i+=8)
                menu_gs.stars[i].brightness=100+rand()%156;
            display_menu(buffer, menu_sel, menu_gs.stars);
            break;

        case STATE_RULES:
            if (inp.escape){ rest(150); state=STATE_MENU; }
            display_rules(buffer);
            break;

        case STATE_PSEUDO_INPUT:
        case STATE_PSEUDO_LOAD:
            if (inp.escape){ rest(150); state=STATE_MENU; break; }
            for (int k=KEY_A;k<=KEY_Z;k++)
                if (key[k] && pseudo_len<15) {
                    pseudo[pseudo_len++]='A'+(k-KEY_A);
                    pseudo[pseudo_len]='\0';
                    while(key[k]) rest(10);
                }
            if (key[KEY_BACKSPACE] && pseudo_len>0){
                pseudo[--pseudo_len]='\0';
                while(key[KEY_BACKSPACE]) rest(10);
            }
            if (inp.enter && pseudo_len>0) {
                rest(150);
                if (state==STATE_PSEUDO_LOAD) {
                    int lv=1,sc=0; load_game(pseudo,&lv,&sc);
                    init_game_state(&gs,lv,pseudo); gs.score=sc;
                } else {
                    init_game_state(&gs,1,pseudo);
                }
                state=STATE_COUNTDOWN;
            }
            display_pseudo_input(buffer,pseudo);
            break;

        case STATE_COUNTDOWN:
            display_countdown(buffer,&gs);
            if (gs.countdown>0) gs.countdown--;
            else state=STATE_PLAYING;
            rest(16);
            break;

        case STATE_PLAYING:
            update_game(&gs,inp.move_left,inp.move_right,
                        inp.move_up,inp.move_down,inp.fire);
            display_game(buffer,&gs);
            if      (gs.game_over) state=STATE_GAME_OVER;
            else if (gs.level_won) state=(gs.level>=4)?STATE_VICTORY:STATE_LEVEL_WON;
            rest(16);
            break;

        case STATE_GAME_OVER:
            display_game_over(buffer,&gs);
            if (inp.enter){ rest(150); init_game_state(&gs,gs.level,gs.pseudo); state=STATE_COUNTDOWN; }
            if (inp.escape){ rest(150); state=STATE_MENU; }
            break;

        case STATE_LEVEL_WON:
            display_level_won(buffer,&gs);
            if (inp.enter){
                rest(150);
                save_game(gs.pseudo,gs.level+1,gs.score);
                int next=gs.level+1,sc=gs.score;
                init_game_state(&gs,next,gs.pseudo); gs.score=sc;
                state=STATE_COUNTDOWN;
            }
            if (inp.escape){ rest(150); state=STATE_MENU; }
            break;

        case STATE_VICTORY:
            display_victory(buffer,&gs);
            if (inp.enter||inp.escape){ rest(150); state=STATE_MENU; }
            break;

        default: break;
        }

        blit(buffer,screen,0,0,0,0,WIN_W,WIN_H);
        rest(16);
    }

    if (falcon_sprite) destroy_bitmap(falcon_sprite);
    destroy_bitmap(buffer);
    allegro_exit();
    return 0;
}
END_OF_MAIN()
