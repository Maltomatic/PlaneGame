#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <math.h>

#define LOG_ENABLED

// Frame rate (frame per second)
const int FPS = 60;
// Display (screen) width.
const int SCREEN_W = 1200;
// Display (screen) height.
const int SCREEN_H = 900;
// At most 10 audios can be played at a time.
const int RESERVE_SAMPLES = 100;

int score = 100;
int lives;
int lives2;
int highscore;
int highscore2;
int prev_high;
int prev_high2;
int spawn = 0;
int thicc = 0;
int flip = 1;
bool p2 = false;
float boss_mhp;

enum {
    SCENE_MENU = 1,
    SCENE_START = 2,
    SCENE_SETTINGS = 3,
    SCENE_OVER = 4
};

int active_scene;
// Keyboard state, whether the key is down or not.
bool key_state[ALLEGRO_KEY_MAX];
// Mouse state, whether the key is down or not.
// 1 is for left, 2 is for right, 3 is for middle.
bool *mouse_state;
// Mouse position.
int mouse_x, mouse_y;

ALLEGRO_DISPLAY* game_display;
ALLEGRO_EVENT_QUEUE* game_event_queue;
ALLEGRO_TIMER* game_update_timer;

/* Shared resources*/

ALLEGRO_FONT* font_pirulen_32;
ALLEGRO_FONT* font_pirulen_24;
ALLEGRO_FONT* font_pirulen_16;
// TODO: More shared resources or data that needed to be accessed
// across different scenes.

/* Menu Scene resources*/
ALLEGRO_BITMAP* main_img_background;
ALLEGRO_BITMAP* img_settings;
ALLEGRO_BITMAP* img_settings2;
ALLEGRO_BITMAP* startbtn;
ALLEGRO_BITMAP* _startbtn_;
ALLEGRO_BITMAP* startbtn2;
ALLEGRO_SAMPLE* main_bgm;
ALLEGRO_SAMPLE_ID main_bgm_id;

ALLEGRO_SAMPLE* wine;
ALLEGRO_SAMPLE_ID wine_id;

/* Start Scene resources*/
ALLEGRO_BITMAP* start_img_background;
ALLEGRO_BITMAP* start_img_plane;
ALLEGRO_BITMAP* player_2;
ALLEGRO_BITMAP* start_img_enemy;
ALLEGRO_SAMPLE* start_bgm;
ALLEGRO_SAMPLE_ID start_bgm_id;
ALLEGRO_SAMPLE* pew;
ALLEGRO_SAMPLE_ID pew_id;
ALLEGRO_SAMPLE* scratch;
ALLEGRO_SAMPLE_ID scratch_id;
ALLEGRO_SAMPLE* die;
ALLEGRO_SAMPLE_ID die_id;
ALLEGRO_SAMPLE* boom;
ALLEGRO_SAMPLE_ID boom_id;
ALLEGRO_SAMPLE* oof;
ALLEGRO_SAMPLE_ID oof_id;
ALLEGRO_BITMAP* img_bullet;
ALLEGRO_BITMAP* img_wbullet;
ALLEGRO_BITMAP* mega;
ALLEGRO_BITMAP* test;
ALLEGRO_BITMAP* mdf;

ALLEGRO_SAMPLE* over;
ALLEGRO_SAMPLE_ID over_id;
ALLEGRO_SAMPLE* over_bgm;
ALLEGRO_SAMPLE_ID over_bgm_id;

typedef struct {
    float x, y;  // center
    float w, h;
    float vx, vy;
    bool hidden;
    float hp;
    ALLEGRO_BITMAP* img;
} MovableObject;
void draw_movable_object(MovableObject obj);
void draw_movable_object_rotated(MovableObject obj);
#define MAX_ENEMY 6
#define MAX_BULLET 20
#define MAX_WBULLETS 12
#define MAX_MFG 10

MovableObject plane;
MovableObject plane2;
MovableObject enemies[MAX_ENEMY];
MovableObject bullets[MAX_BULLET];
MovableObject pbullets[MAX_BULLET];
MovableObject wbullets[MAX_WBULLETS];
MovableObject hypno;
MovableObject wingman;
MovableObject boss;
MovableObject mfg[MAX_MFG];
const float MAX_COOLDOWN = 0.02f;
double last_shoot_timestamp;
double last_wshoot_timestamp;
double last_mega_timestamp;
double last_shoot_timestamp2;
double last_mega_timestamp2;
int plane2_mega;

double last_boss_timestamp;
double last_teleport_timestamp;
double last_mfg_timestamp;

void allegro5_init(void);
// Initialize variables and resources.
// Allows the game to perform any initialization it needs before starting to run.
void game_init(void);
// Process events inside the event queue using an infinity loop.
void game_start_event_loop(void);
// Run game logic such as updating the world, checking for collision, switching scenes and so on.
// This is called when the game should update its logic.
void game_update(void);
// Draw to display.
// This is called when the game should draw itself.
void game_draw(void);
// Release resources.
// Free the pointers we allocated.
void game_destroy(void);
// Function to change from one scene to another.
void game_change_scene(int next_scene);
// Load resized bitmap and check if failed.
ALLEGRO_BITMAP *load_bitmap_resized(const char *filename, int w, int h);

bool pnt_in_rect(int px, int py, int x, int y, int w, int h);

/* Event callbacks. */
void on_key_down(int keycode);
void on_mouse_down(int btn, int x, int y);
void game_abort(const char* format, ...);
void game_log(const char* format, ...);
void game_vlog(const char* format, va_list arg);

int main(int argc, char** argv) {
    srand(time(NULL));
    allegro5_init();
    FILE *fptr;
    if((fptr = fopen("scoring.txt", "rb")) == NULL) game_abort("error opening score file");
    fscanf(fptr, "%d%d", &highscore, &highscore2);
    prev_high = highscore;
    prev_high2 = highscore2;
    fclose(fptr);
    game_log("highscore: %d    duo-highscore: %d", highscore, highscore2);
    game_log("Allegro5 initialized");
    game_init();
    game_log("Game initialized");
    // Draw the first frame.
    game_draw();
    game_log("Game start event loop");
    // This call blocks until the game is finished.
    game_start_event_loop();
    game_log("Game end");
    if(highscore > prev_high || highscore2 > prev_high2){
        if((fptr = fopen("scoring.txt", "wb")) == NULL) game_abort("error opening score file");
        fprintf(fptr, "%d\n%d", highscore, highscore2);
        fclose(fptr);
    } else{
        highscore = prev_high;
        highscore2 = prev_high2;
    }

    game_log("Final highscore: %d     Final duo-highscore: %d", highscore, highscore2);
    game_destroy();
    return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

void allegro5_init(void) {
    if (!al_init()) game_abort("failed to initialize allegro");
    if (!al_init_primitives_addon()) game_abort("failed to initialize primitives add-on");
    if (!al_init_font_addon()) game_abort("failed to initialize font add-on");
    if (!al_init_ttf_addon()) game_abort("failed to initialize ttf add-on");
    if (!al_init_image_addon()) game_abort("failed to initialize image add-on");
    if (!al_install_audio()) game_abort("failed to initialize audio add-on");
    if (!al_init_acodec_addon()) game_abort("failed to initialize audio codec add-on");
    if (!al_reserve_samples(RESERVE_SAMPLES)) game_abort("failed to reserve samples");
    if (!al_install_keyboard()) game_abort("failed to install keyboard");
    if (!al_install_mouse()) game_abort("failed to install mouse");

    game_display = al_create_display(SCREEN_W, SCREEN_H);
    if (!game_display) game_abort("failed to create display");
    al_set_window_title(game_display, "I2P(I)_2020 Final Project <109062104>");

    game_update_timer = al_create_timer(1.0f / FPS);
    if (!game_update_timer) game_abort("failed to create timer");

    game_event_queue = al_create_event_queue();
    if (!game_event_queue) game_abort("failed to create event queue");

    const unsigned m_buttons = al_get_mouse_num_buttons();
    game_log("There are total %u supported mouse buttons", m_buttons);

    mouse_state = malloc((m_buttons + 1) * sizeof(bool));
    memset(mouse_state, false, (m_buttons + 1) * sizeof(bool));

    al_register_event_source(game_event_queue, al_get_display_event_source(game_display));
    al_register_event_source(game_event_queue, al_get_timer_event_source(game_update_timer));
    al_register_event_source(game_event_queue, al_get_keyboard_event_source());
    al_register_event_source(game_event_queue, al_get_mouse_event_source());

    al_start_timer(game_update_timer);
}

void game_init(void) {
    font_pirulen_32 = al_load_font("pirulen.ttf", 32, 0);
    if (!font_pirulen_32) game_abort("failed to load font: pirulen.ttf with size 32");

    font_pirulen_24 = al_load_font("pirulen.ttf", 24, 0);
    if (!font_pirulen_24) game_abort("failed to load font: pirulen.ttf with size 24");

    font_pirulen_16 = al_load_font("pirulen.ttf", 16, 0);
    if (!font_pirulen_24) game_abort("failed to load font: pirulen.ttf with size 16");

    main_img_background = load_bitmap_resized("main-bg.png", SCREEN_W, SCREEN_H);

    main_bgm = al_load_sample("pvz.ogg");
    if (!main_bgm) game_abort("failed to load audio: pvz.ogg");

    wine = al_load_sample("wine.ogg");
    if (!wine) game_abort("failed to load audio: wine.ogg");

    img_settings = al_load_bitmap("settings.png");
    if (!img_settings) game_abort("failed to load image: settings.png");
    img_settings2 = al_load_bitmap("settings2.png");
    if (!img_settings2) game_abort("failed to load image: settings2.png");

    startbtn = al_load_bitmap("startBtn_re.png");
    if (!startbtn) game_abort("failed to load image: startBtn_re.png");
    _startbtn_ = al_load_bitmap("startBtn2_re.png");
    if (!_startbtn_) game_abort("failed to load image: startBtn2_re.png");
    startbtn2 = al_load_bitmap("startPress_re.png");
    if (!startbtn2) game_abort("failed to load image: startPress_re.png");

    start_img_background = load_bitmap_resized("start-bg.jpg", SCREEN_W, SCREEN_H);

    start_img_plane = al_load_bitmap("rocket-4.png");
    if (!start_img_plane) game_abort("failed to load image: rocket-4.png");

    player_2 = al_load_bitmap("tank-2.png");
    if (!player_2) game_abort("failed to load image: tank-2.png");

    start_img_enemy = al_load_bitmap("tie_i.png");
    if (!start_img_enemy) game_abort("failed to load image: tie_i.png");

    start_bgm = al_load_sample("shooting-stars.ogg");
    if (!start_bgm) game_abort("failed to load audio: shooting-stars.ogg");

    pew = al_load_sample("pewpew.ogg");
    if (!pew) game_abort("failed to load audio: pewpew.ogg");

    scratch = al_load_sample("scratch.ogg");
    if (!scratch) game_abort("failed to load audio: scratch.ogg");

    die = al_load_sample("die.ogg");
    if (!die) game_abort("failed to load audio: die.ogg");

    boom = al_load_sample("boom.ogg");
    if (!boom) game_abort("failed to load audio: boom.ogg");

    oof = al_load_sample("oof.ogg");
    if (!oof) game_abort("failed to load audio: oof.ogg");

    over = al_load_sample("over.ogg");
    if (!over) game_abort("failed to load audio: over.ogg");

    over_bgm = al_load_sample("thug.ogg");
    if (!over_bgm) game_abort("failed to load audio: thug.ogg");

    img_bullet = al_load_bitmap("image12.png");
    if (!img_bullet) game_abort("failed to load image: image12.png");

    img_wbullet = al_load_bitmap("rocket.png");
    if (!img_wbullet) game_abort("failed to load image: rocket.png");

    mega = al_load_bitmap("hypno_small.png");
    if (!mega) game_abort("failed to load image: hypno_small.png");

    test = al_load_bitmap("test.png");
    if (!test) game_abort("failed to load image: test.png");

    mdf = al_load_bitmap("mdf.png");
    if (!mdf) game_abort("failed to load image: mdf.png");

    // Change to first scene.
    game_change_scene(SCENE_MENU);
}

void game_start_event_loop(void) {
    bool done = false;
    ALLEGRO_EVENT event;
    int redraws = 0;
    while (!done) {
        al_wait_for_event(game_event_queue, &event);
        if(wingman.hidden == false) wingman.hp -= 1;
        if(boss.hp > 0 && boss.hp <= 8000 && spawn > 3) boss.hp += 2;
        if(plane2_mega >= 0) plane2_mega--;

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            // Event for clicking the window close button.
            game_log("Window close button clicked");
            done = true;
        } else if (event.type == ALLEGRO_EVENT_TIMER) {
            // Event for redrawing the display.
            if (event.timer.source == game_update_timer){
                // The redraw timer has ticked.
                redraws++;
            }
        } else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            //game_log("Key with keycode %d down", event.keyboard.keycode);
            key_state[event.keyboard.keycode] = true;
            on_key_down(event.keyboard.keycode);
        } else if (event.type == ALLEGRO_EVENT_KEY_UP) {
            //game_log("Key with keycode %d up", event.keyboard.keycode);
            key_state[event.keyboard.keycode] = false;
        } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            game_log("Mouse button %d down at (%d, %d)", event.mouse.button, event.mouse.x, event.mouse.y);
            mouse_state[event.mouse.button] = true;
            on_mouse_down(event.mouse.button, event.mouse.x, event.mouse.y);
        } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
            game_log("Mouse button %d up at (%d, %d)", event.mouse.button, event.mouse.x, event.mouse.y);
            mouse_state[event.mouse.button] = false;
        } else if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
            if (event.mouse.dx != 0 || event.mouse.dy != 0) {
                mouse_x = event.mouse.x;
                mouse_y = event.mouse.y;
            } /*else if (event.mouse.dz != 0) {
                game_log("Mouse scroll at (%d, %d) with delta %d", event.mouse.x, event.mouse.y, event.mouse.dz);
            }*/
        }

        // Redraw
        if (redraws > 0 && al_is_event_queue_empty(game_event_queue)) {
            game_update();
            game_draw();
            redraws = 0;
        }
    }
}

void game_update(void) {
    if (active_scene == SCENE_START) {
        // game over if score too low
        if(score < 0){
            score = 0;
            if (!al_play_sample(over, 1, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &over_id)) game_abort("failed to play audio (over)");
            game_change_scene(SCENE_OVER);
        }

        // plane move
        if(plane.hidden == false){
            plane.vx = plane.vy = 0;
            if (key_state[ALLEGRO_KEY_UP]) plane.vy -= 1;
            if (key_state[ALLEGRO_KEY_DOWN]) plane.vy += 1;
            if (key_state[ALLEGRO_KEY_LEFT]) plane.vx -= 1;
            if (key_state[ALLEGRO_KEY_RIGHT]) plane.vx += 1;
            plane.y += plane.vy * 6 * (plane.vx ? 0.71f : 1);
            plane.x += plane.vx * 6 * (plane.vy ? 0.71f : 1);

            if(plane.x > SCREEN_W-plane.w/2) plane.x = SCREEN_W-plane.w/2;
            if(plane.x < plane.w/2) plane.x = plane.w/2;
            if(plane.y > SCREEN_H-plane.h/2) plane.y = SCREEN_H-plane.h/2;
            if(plane.y < plane.h/2) plane.y = plane.h/2;
        }

        // p2 move
        if(plane2.hidden == false){
            plane2.vx = plane2.vy = 0;
            if (key_state[ALLEGRO_KEY_W]) plane2.vy -= 1;
            if (key_state[ALLEGRO_KEY_S]) plane2.vy += 1;
            if (key_state[ALLEGRO_KEY_A]) plane2.vx -= 1;
            if (key_state[ALLEGRO_KEY_D]) plane2.vx += 1;
            plane2.y += plane2.vy * 6 * (plane2.vx ? 0.71f : 1);
            plane2.x += plane2.vx * 6 * (plane2.vy ? 0.71f : 1);

            if(plane2.x > SCREEN_W-plane2.w/2) plane2.x = SCREEN_W-plane2.w/2;
            if(plane2.x < plane2.w/2) plane2.x = plane2.w/2;
            if(plane2.y > SCREEN_H-plane2.h/2) plane2.y = SCREEN_H-plane2.h/2;
            if(plane2.y < plane2.h/2) plane2.y = plane2.h/2;
        }

        double now = al_get_time();

        int i, j;
        // bullet move
        for (i = 0; i < MAX_BULLET; i++) {
            if (bullets[i].hidden) continue;
            bullets[i].x += 0;
            bullets[i].y -= 10;
            if (bullets[i].y < -bullets[i].h/2) bullets[i].hidden = true;
        }
        // pbullets move
        for (i = 0; i < MAX_BULLET; i++) {
            if (pbullets[i].hidden) continue;
            pbullets[i].x += 0;
            pbullets[i].y -= 10;
            if (pbullets[i].y < -pbullets[i].h/2) pbullets[i].hidden = true;
        }

        //wbullet move
        for(i = 0; i < MAX_WBULLETS; i++) {
            if (wbullets[i].hidden) continue;
            wbullets[i].x += 0;
            wbullets[i].y -= 9;
            if (wbullets[i].y < -wbullets[i].h/2) wbullets[i].hidden = true;
        }
        // hypno move
        if(hypno.hidden == false){
            hypno.x += 0;
            hypno.y -= 3;
            if(hypno.y < -hypno.w/2) hypno.hidden = true;
        }

        //mfg move
        for (i = 0; i < MAX_MFG; i++) {
            if (mfg[i].hidden) continue;
            mfg[i].x += mfg[i].vx;
            mfg[i].y += mfg[i].vy;
            if (mfg[i].y > SCREEN_H+mfg[i].h/2) mfg[i].hidden = true;
            else if(mfg[i].y < 0) mfg[i].hidden = true;
            else if(mfg[i].x > SCREEN_W) mfg[i].hidden = true;
            else if(mfg[i].x < 0) mfg[i].hidden = true;
        }

        // wingman move
        if(wingman.hp > 0){
            wingman.x = plane.x - 30 + cos(wingman.hp/30)*30;
            wingman.y = plane.y - 10;
            if(wingman.y < wingman.h/2) wingman.y = wingman.h/2;
            if(wingman.x < wingman.w/2) wingman.x = wingman.w/2;
        }
        else wingman.hidden = true;

        // enemy move
        for (i = 0; i < MAX_ENEMY; i++) {
            if(enemies[i].x > SCREEN_W-enemies[i].w/2) enemies[i].x = SCREEN_W-enemies[i].w/2;
            if(enemies[i].x < enemies[i].w/2) enemies[i].x = enemies[i].w/2;
            enemies[i].y += 1.2;
            enemies[i].x += cos(enemies[i].y/30)*3;
            if(enemies[i].y > SCREEN_H+enemies[i].h/2){
                enemies[i].y = -15 - enemies[i].h - (i*10);
                enemies[i].x = enemies[i].w/2+rand()%(int)(SCREEN_W-enemies[i].w);
                score -= 20;
                if (!al_play_sample(oof, 1.5, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &oof_id)) game_abort("failed to play audio (oof)");
            }
        }
        // boss move
        if(boss.hidden == false){
            bool teleport_b = false;
            double teleport = al_get_time();
            if(teleport - last_teleport_timestamp >= 6){
                int shift = (int)rand()%4;
                if(shift != 0) teleport_b = true;
            }
            last_teleport_timestamp = teleport;
            if(boss.x > SCREEN_W-boss.w/2){
                boss.x = SCREEN_W-boss.w/2;
                teleport_b = true;
            }
            if(boss.x < boss.w/2){
                boss.x = boss.w/2;
                teleport_b = true;
            }
            if(teleport_b){
                boss.x = boss.w/2+rand()%(int)(SCREEN_W-boss.w);
                boss.y -= 3;
                flip *= -1;
            }
            boss.y += 0.3;
            if(flip == 1) boss.x += sin((al_get_time()-last_boss_timestamp)/20)*2;
            else boss.x -= cos((al_get_time()-last_boss_timestamp)/20)*2;

            if(boss.y > SCREEN_H+boss.h/2){
                lives = 0;
                lives2 = 0;
                score -= 100;
            }
        }

        // bullet hit
        for(i = 0; i < MAX_BULLET; i++){
            if(bullets[i].hidden) continue;
            for(j = 0; j < MAX_ENEMY; j++){
                if(bullets[i].x <= enemies[j].x+15 && bullets[i].x >= enemies[j].x-15 && bullets[i].y-bullets[i].h/2+10 >= enemies[j].y && bullets[i].y-bullets[i].h/2-4 <= enemies[j].y){
                    enemies[j].hp -= bullets[i].hp;
                    bullets[i].hidden = true;
                }
            }
            if(boss.hidden == false){
                if(bullets[i].x <= boss.x+65 && bullets[i].x >= boss.x-65 && bullets[i].y-bullets[i].h/2+15 >= boss.y+boss.h/2 && bullets[i].y-bullets[i].h/2-4 <= boss.y+boss.h/2){
                    boss.hp -= 80;
                    bullets[i].hidden = true;
                    //game_log("bullet[%d] hit boss", i);
                }
            }
        }
        // pbullets hit
        for(i = 0; i < MAX_BULLET; i++){
            if(pbullets[i].hidden) continue;
            for(j = 0; j < MAX_ENEMY; j++){
                if(pbullets[i].x <= enemies[j].x+15 && pbullets[i].x >= enemies[j].x-15 && pbullets[i].y-pbullets[i].h/2+10 >= enemies[j].y && pbullets[i].y-pbullets[i].h/2-4 <= enemies[j].y){
                    enemies[j].hp -= pbullets[i].hp;
                    pbullets[i].hidden = true;
                }
            }
            if(boss.hidden == false){
                if(pbullets[i].x <= boss.x+65 && pbullets[i].x >= boss.x-65 && pbullets[i].y-pbullets[i].h/2+15 >= boss.y+boss.h/2 && pbullets[i].y-pbullets[i].h/2-4 <= boss.y+boss.h/2){
                    boss.hp -= 80;
                    pbullets[i].hidden = true;
                    //game_log("pbullet[%d] hit boss", i);
                }
            }
        }

        // wbullet hit
        for(i = 0; i < MAX_WBULLETS; i++){
            if(wbullets[i].hidden) continue;
            for(j = 0; j < MAX_ENEMY; j++){
                if(wbullets[i].x <= enemies[j].x+20 && wbullets[i].x >= enemies[j].x-20 && wbullets[i].y >= enemies[j].y+enemies[j].h/2 && wbullets[i].y-wbullets[i].h/2-8 <= enemies[j].y+enemies[j].h/2){
                    enemies[j].hp = 0;
                    if (!al_play_sample(die, 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &die_id)) game_abort("failed to play audio (die)");
                }
            }
            if(boss.hidden == false){
                if(wbullets[i].x <= boss.x+75 && wbullets[i].x >= boss.x-75 && wbullets[i].y-wbullets[i].h/2+15 >= boss.y+boss.h/2 && wbullets[i].y-wbullets[i].h/2-4 <= boss.y+boss.h/2){
                    boss.hp -= 200;
                    wbullets[i].hidden = true;
                    //game_log("wbullet[%d] hit boss", i);
                }
            }
        }
        // hypno hit
        if(hypno.hidden == false){
            for(j = 0; j < MAX_ENEMY; j++){
                if(hypno.x <= enemies[j].x+30 && hypno.x >= enemies[j].x-30 && hypno.y-hypno.h/2+25 >= enemies[j].y+enemies[j].h/2 && hypno.y-hypno.h/2-10 <= enemies[j].y+enemies[j].h/2){
                    enemies[j].hp = 0;
                    if(lives < 4) lives++;
                    game_log("Add life");
                    hypno.hp = 0;
                }
            }
            if(boss.hidden == false){
                if(hypno.x <= boss.x+85 && hypno.x >= boss.x-85 && hypno.y-hypno.h/2+20 >= boss.y+boss.h/2 && hypno.y-hypno.h/2-4 <= boss.y+boss.h/2){
                    boss.hp -= 800;
                    hypno.hidden = true;
                    game_log("hypno hit boss");
                }
            }
            if(hypno.hp == 0){
                if (!al_play_sample(scratch, 1.2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &over_id)) game_abort("failed to play audio (scratch)");
                wingman.hidden = false;
                wingman.hp = 900;
                hypno.hp = 800;
                for(i = 0; i < MAX_WBULLETS; i++){
                    wbullets[i].hidden = false;
                }
            }
        }

        // enemy hit player or wingman
        for(j = 0; j < MAX_ENEMY; j++){
            if(plane.hidden == false && plane.x <= enemies[j].x+25 && plane.x >= enemies[j].x-25 && plane.y+35 >= enemies[j].y+enemies[j].h/2 && plane.y-plane.h/2-7 <= enemies[j].y+enemies[j].h/2){
                lives--;
                enemies[j].hp = 0;
                score -= 30;
                game_log("hit by enemy, lives left %d", lives);
                plane.x = SCREEN_W/2;
                plane.y = SCREEN_H/2;
                for(i = 0; i < MAX_ENEMY; i++) enemies[i].y = -enemies[i].h/2-15;
                if(boss.hidden == false) boss.y = -boss.h/2-15;
                if(lives <= 0){
                    plane.hidden = true;
                }
            }
            if(plane2.hidden == false && plane2.x <= enemies[j].x+25 && plane2.x >= enemies[j].x-25 && plane2.y+35 >= enemies[j].y+enemies[j].h/2 && plane2.y-plane2.h/2-7 <= enemies[j].y+enemies[j].h/2){
                score -= 30;
                if(plane2_mega <= 0){
                    lives2--;
                    enemies[j].hp = 0;
                    game_log("hit by enemy, p2 lives left %d", lives2);
                    plane2.x = SCREEN_W/2;
                    plane2.y = SCREEN_H/2;
                    for(i = 0; i < MAX_ENEMY; i++) enemies[i].y = -enemies[i].h/2-15;
                    if(boss.hidden == false) boss.y = -boss.h/2-15;
                    if(lives2 <= 0){
                        plane2.hidden = true;
                    }
                }else{
                    enemies[j].hp = 0;
                    game_log("ram enemy");
                }
            }
            if(wingman.x <= enemies[j].x+20 && wingman.x >= enemies[j].x-20 && wingman.y-wingman.h/2+8 >= enemies[j].y+enemies[j].h/2 && wingman.y-wingman.h/2-5 <= enemies[j].y+enemies[j].h/2) wingman.hp = 0;
        }
        //mfg hit
        for(i = 0; i < MAX_MFG; i++){
            if(mfg[i].hidden) continue;
            if(plane.hidden == false && mfg[i].x <= plane.x+25 && mfg[i].x >= plane.x-25 && mfg[i].y+mfg[i].h/2 >= plane.y+plane.h/2 && mfg[i].y-mfg[i].h/2-8 <= plane.y+plane.h/2){
                lives--;
                game_log("hit by mfg, lives left %d", lives);
                if (!al_play_sample(boom, 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &boom_id)) game_abort("failed to play audio (boom)");
                boss.y = -boss.h/2;
                for(j = 0; j < MAX_ENEMY; j++) enemies[j].y = -enemies[j].h/2;
                mfg[i].hidden = true;
                if(lives <= 0){
                    plane.hidden = true;
                }
            } else if(plane2.hidden == false && mfg[i].x <= plane2.x+25 && mfg[i].x >= plane2.x-25 && mfg[i].y+mfg[i].h/2 >= plane2.y+plane2.h/2 && mfg[i].y-mfg[i].h/2-8 <= plane2.y+plane2.h/2){
                if(plane2_mega <= 0){
                    lives2--;
                    game_log("hit by mfg, p2 lives left %d", lives2);
                    if (!al_play_sample(boom, 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &boom_id)) game_abort("failed to play audio (boom)");
                    boss.y = -boss.h/2;
                    for(j = 0; j < MAX_ENEMY; j++) enemies[j].y = -enemies[j].h/2;
                    mfg[i].hidden = true;
                    if(lives2 <= 0){
                        plane2.hidden = true;
                    }
                }
            }
            if(mfg[i].x <= wingman.x+20 && mfg[i].x >= wingman.x-20 && mfg[i].y+mfg[i].h/2 >= wingman.y+wingman.h/2 && mfg[i].y-mfg[i].h/2-8 <= wingman.y+wingman.h/2) wingman.hp = 0;

        }

        // enemy die
        for(i = 0; i < MAX_ENEMY; i++){
            if(enemies[i].hp <= 0){
                if (!al_play_sample(boom, 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &boom_id)) game_abort("failed to play audio (boom)");
                score += 30;
                enemies[i].y = -15 - enemies[i].h - (i*10);
                enemies[i].x = enemies[i].w/2+rand()%(int)(SCREEN_W-enemies[i].w);
                enemies[i].hp = 800;
            }
        }
        // boss die
        if(boss.hp <= 0){
            if (!al_play_sample(boom, 2, 0.0, 0.5, ALLEGRO_PLAYMODE_ONCE, &boom_id)) game_abort("failed to play audio (boom)");
            score += 200;
            game_log("boss dead");
            boss.hidden = true;
            boss.y = -10;
            boss.hp = 4000;
            if(now - last_boss_timestamp >= 60-spawn*5 - 10) last_boss_timestamp = now+10-(60-spawn*5);
        }

        // both players dead
        if(lives <= 0 && lives2 <= 0){
            if (!al_play_sample(over, 1, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &over_id)) game_abort("failed to play audio (over)");
            if(p2){
                if(score > highscore2) highscore2 = score;
            }else{
                if(score > highscore) highscore = score;
            }
            game_change_scene(SCENE_OVER);
        }

        // [HACKATHON 2-8]
        // TODO: Shoot if key is down and cool-down is over.
        // 1) Get the time now using 'al_get_time'.
        // 2) If Space key is down in 'key_state' and the time
        //    between now and last shoot is not less that cool
        //    down time.
        // 3) Loop through the bullet array and find one that is hidden.
        //    (This part can be optimized.)
        // 4) The bullet will be found if your array is large enough.
        // 5) Set the last shoot time to now.
        // 6) Set hidden to false (recycle the bullet) and set its x, y to the
        //    front part of your plane.
        // Uncomment and fill in the code below.

        // player shoot
        if (plane.hidden == false && key_state[ALLEGRO_KEY_SPACE] && now - last_shoot_timestamp >= MAX_COOLDOWN) {
            for (i = 0; i < MAX_BULLET; i++) {
                if (bullets[i].hidden) {
                    if(i%5 == 0){
                        if (!al_play_sample(pew, 1.2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &pew_id)) game_abort("failed to play audio (pew)");
                    }
                    last_shoot_timestamp = now;
                    bullets[i].hidden = false;
                    bullets[i].x = plane.x;
                    bullets[i].y = plane.y;
                    break;
                }
            }
        }
        // plane2 shoot
        if (plane2.hidden == false && key_state[ALLEGRO_KEY_Q] && now - last_shoot_timestamp2 >= MAX_COOLDOWN) {
            for (i = 0; i < MAX_BULLET; i++) {
                if (pbullets[i].hidden) {
                    if(i%5 == 0){
                        if (!al_play_sample(pew, 1.2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &pew_id)) game_abort("failed to play audio (pew)");
                    }
                    last_shoot_timestamp2 = now;
                    pbullets[i].hidden = false;
                    pbullets[i].x = plane2.x;
                    pbullets[i].y = plane2.y;
                    break;
                }
            }
        }

        // mega: hypno
        if (plane.hidden == false && key_state[ALLEGRO_KEY_M] && now - last_mega_timestamp >= 30) {
            if(hypno.hidden){
                last_mega_timestamp = now;
                hypno.hidden = false;
                hypno.x = plane.x;
                hypno.y = plane.y;
                hypno.hp = 800;
            }
        }
        // plane 2 mega: invincibility ram
        if (plane2.hidden == false && key_state[ALLEGRO_KEY_1] && now - last_mega_timestamp2 >= 20){
            plane2_mega = 480;
            if(lives2 < 3) lives2++;
            last_mega_timestamp2 = now;
        }

        // wingman shoot
        if (wingman.hidden == false && now - last_wshoot_timestamp >= 0.2f) {
            for (i = 0; i < MAX_WBULLETS; i++) {
                if (wbullets[i].hidden) {
                    last_wshoot_timestamp = now;
                    wbullets[i].hidden = false;
                    wbullets[i].x = wingman.x;
                    wbullets[i].y = wingman.y;
                    break;
                }
            }
        }

        // boss spawn
        if(boss.hidden == true && now - last_boss_timestamp >= 60-spawn*5){
            game_log("boss spawn");
            last_boss_timestamp = now;
            spawn++;
            if(spawn > 4){
                thicc++;
                spawn = 4;
            }
            boss.hidden = false;
            boss.hp = 4000 + thicc*200;
            boss_mhp = boss.hp;
            boss.y = -boss.h/2;
            boss.x = boss.w/2+rand()%(int)(SCREEN_W-boss.w);
        }
        // boss shoot
        if (boss.hidden == false && boss.y >= 10 && now - last_mfg_timestamp >= 0.01f) {
            for (i = 0; i < MAX_MFG; i++) {
                if (mfg[i].hidden) {
                    last_mfg_timestamp = now;
                    mfg[i].hidden = false;
                    mfg[i].x = boss.x;
                    mfg[i].y = boss.y;
                    mfg[i].vx = (int)rand()%16-9;
                    mfg[i].vy = (int)rand()%8;
                    if(mfg[i].vy == 0) mfg[i].vy = 8;
                    break;
                }
            }
        }
    }
}

void game_draw(void) {
    if (active_scene == SCENE_MENU) {
        al_draw_bitmap(main_img_background, 0, 0, 0);
        al_draw_text(font_pirulen_32, al_map_rgb(255, 30, 40), SCREEN_W / 2, 250, ALLEGRO_ALIGN_CENTER, "Pew Pew Spaceshi*s");
        al_draw_text(font_pirulen_16, al_map_rgb(255, 255, 255), 20, SCREEN_H - 50, 0, "Press enter key or click to start");

        if (pnt_in_rect(mouse_x, mouse_y, SCREEN_W-29, 29, 19, 19)) al_draw_bitmap(img_settings2, SCREEN_W-48, 10, 0);
        else al_draw_bitmap(img_settings, SCREEN_W-48, 10, 0);

        if (pnt_in_rect(mouse_x, mouse_y, SCREEN_W/2, 470, 240, 120)) al_draw_bitmap(startbtn2, SCREEN_W/2-240, 230, 0); ////
        else{
            if((int)al_get_time()%2) al_draw_bitmap(startbtn, SCREEN_W/2-240, 230, 0);
            else al_draw_bitmap(_startbtn_, SCREEN_W/2-240, 230, 0);
        }
    } else if (active_scene == SCENE_START) {
        int i;
        al_draw_bitmap(start_img_background, 0, 0, 0);
        for (i = 0; i < MAX_BULLET; i++) draw_movable_object(bullets[i]);
        for (i = 0; i < MAX_BULLET; i++) draw_movable_object(pbullets[i]);
        for (i = 0; i < MAX_WBULLETS; i++) draw_movable_object(wbullets[i]);
        draw_movable_object(plane);
        draw_movable_object(plane2);
        draw_movable_object(hypno);
        draw_movable_object_rotated(wingman);
        draw_movable_object(boss);
        al_draw_filled_rounded_rectangle(boss.x-35, boss.y-87, boss.x+35, boss.y-79, 4, 4, al_map_rgb(255, 0, 0));
        al_draw_filled_rounded_rectangle(boss.x-35, boss.y-87, boss.x-35+(boss.hp/boss_mhp)*70, boss.y-79, 4, 4, al_map_rgb(0, 255, 0));
        for(i = 0; i < MAX_MFG; i++) draw_movable_object(mfg[i]);
        for (i = 0; i < MAX_ENEMY; i++){
            draw_movable_object(enemies[i]);
            al_draw_filled_rounded_rectangle(enemies[i].x-15, enemies[i].y-enemies[i].h+8, enemies[i].x+15, enemies[i].y-enemies[i].h+12, 2, 2, al_map_rgb(255, 0, 0));
            al_draw_filled_rounded_rectangle(enemies[i].x-15, enemies[i].y-enemies[i].h+8, enemies[i].x-15+(enemies[i].hp/800)*30, enemies[i].y-enemies[i].h+12, 2, 2, al_map_rgb(0, 255, 0));
        }
        al_draw_textf(font_pirulen_16, al_map_rgb(137, 29, 191), SCREEN_W/20, SCREEN_H/30, ALLEGRO_ALIGN_LEFT, "score: %d", score);
        double now = al_get_time();
        double cd_time = now-last_mega_timestamp;
        if(cd_time > 30.0) cd_time = 30.0;
        double cd_time2 = now-last_mega_timestamp2;
        if(cd_time2 > 20.0) cd_time2 = 20.0;
        al_draw_textf(font_pirulen_16, al_map_rgb(137, 29, 191), SCREEN_W/20, SCREEN_H/18, ALLEGRO_ALIGN_LEFT, "P1 lives left: %d", lives);
        al_draw_textf(font_pirulen_16, al_map_rgb(137, 29, 191), SCREEN_W/20, SCREEN_H/12, ALLEGRO_ALIGN_LEFT, "P1 CD time: %.2lf", plane.hidden? 0.00:(30.0-cd_time));
        if(p2){
            al_draw_textf(font_pirulen_16, al_map_rgb(137, 29, 191), SCREEN_W/16+200, SCREEN_H/18, ALLEGRO_ALIGN_LEFT, "P2 lives left: %d", lives2);
            al_draw_textf(font_pirulen_16, al_map_rgb(137, 29, 191), SCREEN_W/16+200, SCREEN_H/12, ALLEGRO_ALIGN_LEFT, "P2 CD time: %.2lf", plane2.hidden? 0.00:(20.0-cd_time2));
            if(plane2_mega >= 18) al_draw_textf(font_pirulen_16, al_map_rgb(200, 0, 0), SCREEN_W/16+200, SCREEN_H/10, ALLEGRO_ALIGN_LEFT, "INVINCIBLE time: %d", plane2_mega/60+1);
        }
    } else if (active_scene == SCENE_SETTINGS) {
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_draw_textf(font_pirulen_24, al_map_rgb(60, 60, 255), SCREEN_W/2, SCREEN_H/2-170, ALLEGRO_ALIGN_CENTER, "CURRENT PLAY MODE: %s", p2? "multiplayer":"solo");
        al_draw_text(font_pirulen_16, al_map_rgb(60, 60, 255), SCREEN_W/2, SCREEN_H/2-130, ALLEGRO_ALIGN_CENTER, "Press 'P' to change mode");
        al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W/2, SCREEN_H/2-50, ALLEGRO_ALIGN_CENTER, "Player 1: Arrow Keys; 'SPACE' to shoot, 'M' for skill");
        al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W/2, SCREEN_H/2, ALLEGRO_ALIGN_CENTER, "Player 2: WASD; 'Q' to shoot, 'G' for skill");
        al_draw_text(font_pirulen_16, al_map_rgb(255, 255, 255), SCREEN_W/2, SCREEN_H/2+90, ALLEGRO_ALIGN_CENTER, "Press 'BACKSPACE' to exit");
        al_draw_text(font_pirulen_16, al_map_rgb(255, 0, 0), SCREEN_W/2, SCREEN_H/2+120, ALLEGRO_ALIGN_CENTER, "Press 'R' to reset highscore");


    } else if (active_scene == SCENE_OVER) {
        al_draw_bitmap(start_img_background, 0, 0, 0);
        al_draw_text(font_pirulen_32, al_map_rgb(255, 0, 0), SCREEN_W/2, SCREEN_H/2-50, ALLEGRO_ALIGN_CENTER, "GAME OVER");
        al_draw_textf(font_pirulen_24, al_map_rgb(212, 203, 195), SCREEN_W/2, SCREEN_H/2, ALLEGRO_ALIGN_CENTER, "SCORE: %d       HIGHSCORE: %d", score, p2? highscore2:highscore);
        al_draw_text(font_pirulen_16, al_map_rgb(212, 203, 195), SCREEN_W/2, SCREEN_H/2+90, ALLEGRO_ALIGN_CENTER, "press 'ENTER' to restart");
        al_draw_text(font_pirulen_16, al_map_rgb(212, 203, 195), SCREEN_W/2, SCREEN_H/2+120, ALLEGRO_ALIGN_CENTER, "press 'BACKSPACE' to exit");
    }

    al_flip_display();
}

void game_destroy(void) {
    // Destroy shared resources.
    al_destroy_font(font_pirulen_32);
    al_destroy_font(font_pirulen_24);
    al_destroy_font(font_pirulen_16);

    /* Menu Scene resources*/
    al_destroy_bitmap(main_img_background);
    al_destroy_sample(main_bgm);

    al_destroy_sample(wine);
    // [HACKATHON 3-6]
    // TODO: Destroy the 2 settings images.
    // Uncomment and fill in the code below.
    al_destroy_bitmap(img_settings);
    al_destroy_bitmap(img_settings2);

    /* Start Scene resources*/
    al_destroy_bitmap(start_img_background);
    al_destroy_bitmap(start_img_plane);
    al_destroy_bitmap(player_2);
    al_destroy_bitmap(start_img_enemy);
    al_destroy_sample(start_bgm);
    al_destroy_sample(pew);
    al_destroy_sample(scratch);
    al_destroy_sample(die);
    al_destroy_sample(boom);
    al_destroy_sample(oof);
    al_destroy_sample(over);
    al_destroy_sample(over_bgm);
    al_destroy_bitmap(img_bullet);
    al_destroy_bitmap(mega);
    al_destroy_bitmap(img_wbullet);
    al_destroy_bitmap(test);
    al_destroy_bitmap(mdf);

    al_destroy_timer(game_update_timer);
    al_destroy_event_queue(game_event_queue);
    al_destroy_display(game_display);
    free(mouse_state);
}

void game_change_scene(int next_scene) {
    game_log("Change scene from %d to %d", active_scene, next_scene);
    // TODO: Destroy resources initialized when creating scene.
    if (active_scene == SCENE_MENU) {
        al_stop_sample(&main_bgm_id);
        game_log("stop audio (bgm)");
    } else if (active_scene == SCENE_START) {
        al_stop_sample(&start_bgm_id);
        game_log("stop audio (bgm)");
    } else if (active_scene == SCENE_OVER) {
        al_stop_sample(&over_bgm_id);
        game_log("highscore %d     duo-highscore %d", highscore, highscore2);
    } else if (active_scene == SCENE_SETTINGS) {
        al_stop_sample(&wine_id);
        game_log("stop audio (bgm)");
    }
    active_scene = next_scene;
    // TODO: Allocate resources before entering scene.
    if (active_scene == SCENE_MENU) {
        if (!al_play_sample(main_bgm, 1, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &main_bgm_id)) game_abort("failed to play audio (bgm)");
    } else if (active_scene == SCENE_START) {
        int i;

        score = 100;
        lives = 3;
        spawn = 0;
        thicc = 0;
        last_mega_timestamp = al_get_time();
        last_shoot_timestamp = al_get_time();

        last_mega_timestamp2 = al_get_time();
        last_shoot_timestamp2 = al_get_time();
        plane2.img = player_2;
        plane2.x = SCREEN_W/2;
        plane2.y = 500;
        plane2.w = al_get_bitmap_width(player_2);
        plane2.h = al_get_bitmap_height(player_2);
        p2? (plane2.hidden = false, lives2 = 3) : (plane2.hidden = true, lives2 = 0);

        last_wshoot_timestamp = al_get_time();
        last_boss_timestamp = al_get_time();
        last_mfg_timestamp = al_get_time();
        boss.hidden = true;
        wingman.hidden = true;

        plane.img = start_img_plane;
        plane.x = SCREEN_W/2;
        plane.y = 500;
        plane.w = al_get_bitmap_width(plane.img);
        plane.h = al_get_bitmap_height(plane.img);
        plane.hidden = false;

        hypno.w = al_get_bitmap_width(mega);
        hypno.h = al_get_bitmap_height(mega);
        hypno.img = mega;
        hypno.vx = 0;
        hypno.vy = -6;
        hypno.hidden = true;
        hypno.hp = 800;

        wingman.w = al_get_bitmap_width(start_img_enemy);
        wingman.h = al_get_bitmap_height(start_img_enemy);
        wingman.img = start_img_enemy;
        wingman.hp = 0;

        boss.w = al_get_bitmap_width(test);
        boss.h = al_get_bitmap_height(test);
        boss.img = test;
        boss.hp = 4000;

        for (i = 0; i < MAX_ENEMY; i++) {
            enemies[i].img = start_img_enemy;
            enemies[i].w = al_get_bitmap_width(start_img_enemy);
            enemies[i].h = al_get_bitmap_height(start_img_enemy);
            enemies[i].x = enemies[i].w / 2 + (float)rand() / RAND_MAX * (SCREEN_W - enemies[i].w);
            enemies[i].y = - enemies[i].h - (i*10);;
            enemies[i].hp = 800;
        }

        for (i = 0; i < MAX_BULLET; i++) {
            bullets[i].w = al_get_bitmap_width(img_bullet);
            bullets[i].h = al_get_bitmap_height(img_bullet);
            bullets[i].img = img_bullet;
            bullets[i].vx = 0;
            bullets[i].vy = -10;
            bullets[i].hidden = true;
            bullets[i].hp = 120;

            pbullets[i].w = al_get_bitmap_width(img_bullet);
            pbullets[i].h = al_get_bitmap_height(img_bullet);
            pbullets[i].img = img_bullet;
            pbullets[i].vx = 0;
            pbullets[i].vy = -10;
            pbullets[i].hidden = true;
            pbullets[i].hp = 120;
        }

        for (i = 0; i < MAX_WBULLETS; i++) {
            wbullets[i].w = al_get_bitmap_width(img_wbullet);
            wbullets[i].h = al_get_bitmap_height(img_wbullet);
            wbullets[i].img = img_wbullet;
            wbullets[i].vx = 0;
            wbullets[i].vy = -10;
            wbullets[i].hidden = true;
            wbullets[i].hp = 800;
        }

        for(i = 0; i < MAX_MFG; i++){
            mfg[i].w = al_get_bitmap_width(mdf);
            mfg[i].h = al_get_bitmap_height(mdf);
            mfg[i].img = mdf;
            mfg[i].vx = 0;
            mfg[i].vy = 0;
            mfg[i].hidden = true;
            mfg[i].hp = 1;
        }

        if (!al_play_sample(start_bgm, 1.5, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &start_bgm_id)) game_abort("failed to play audio (bgm)");
    } else if (active_scene == SCENE_OVER) {
        if (!al_play_sample(over_bgm, 1, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &over_bgm_id)) game_abort("failed to play audio (over_bgm)");
    }
}

void on_key_down(int keycode) {
    if (active_scene == SCENE_MENU) {
        if (keycode == ALLEGRO_KEY_ENTER)
            game_change_scene(SCENE_START);
    } else if (active_scene == SCENE_SETTINGS) {
        if (keycode == ALLEGRO_KEY_BACKSPACE) game_change_scene(SCENE_MENU);
        else if (keycode == ALLEGRO_KEY_R){
            FILE *fptr;
            if((fptr = fopen("scoring.txt", "wb")) == NULL) game_abort("error opening score file");
            if(p2){
                fprintf(fptr, "%d\n%d", highscore, 100);
                highscore2 = 100;
            } else{
                fprintf(fptr, "%d\n%d", 100, highscore2);
                highscore = 100;
            }
            fclose(fptr);
            free(fptr);
            prev_high = 100;
            prev_high2 = 100;
            game_log("post-edit     highscore: %d, duo-highscore: %d", highscore, highscore2);
        } else if(keycode == ALLEGRO_KEY_P){
            if(p2 == false){
                p2 = true;
                plane2.hidden = false;
                lives2 = 3;
            } else if(p2 == true){
                p2 = false;
                plane2.hidden = true;
                lives2 = 0;
            }
        }
    } else if (active_scene == SCENE_OVER) {
        if (keycode == ALLEGRO_KEY_BACKSPACE) game_change_scene(SCENE_MENU);
        else if (keycode == ALLEGRO_KEY_ENTER) game_change_scene(SCENE_START);
    } else if (active_scene == SCENE_START){
        if (keycode == ALLEGRO_KEY_BACKSPACE){
            game_change_scene(SCENE_MENU);
        }
    }
}

void on_mouse_down(int btn, int x, int y) {
    if (active_scene == SCENE_MENU) {
        if (btn == mouse_state[1]) {
            if (pnt_in_rect(x, y, SCREEN_W-29, 29, 19, 19)){
                if (!al_play_sample(wine, 1, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &wine_id)) game_abort("failed to play audio (bgm)");
                game_change_scene(SCENE_SETTINGS);
            } else if (pnt_in_rect(x, y, SCREEN_W/2, 470, 240, 120)){ ////
                //if (!al_play_sample(wine, 1, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &wine_id)) game_abort("failed to play audio (bgm)");
                game_change_scene(SCENE_START);
            }
        }
    }
}

void draw_movable_object(MovableObject obj) {
    if (obj.hidden) return;
    al_draw_bitmap(obj.img, round(obj.x - obj.w / 2), round(obj.y - obj.h / 2), 0);
}

void draw_movable_object_rotated(MovableObject obj) {
    if (obj.hidden) return;
    al_draw_rotated_bitmap(obj.img, round(obj.x - obj.w / 2), round(obj.y - obj.h / 2), 0, 0, 3.14, 0);
}

ALLEGRO_BITMAP *load_bitmap_resized(const char *filename, int w, int h) {
    ALLEGRO_BITMAP* loaded_bmp = al_load_bitmap(filename);
    if (!loaded_bmp) game_abort("failed to load image: %s", filename);
    ALLEGRO_BITMAP *resized_bmp = al_create_bitmap(w, h);
    ALLEGRO_BITMAP *prev_target = al_get_target_bitmap();
    if (!resized_bmp) game_abort("failed to create bitmap when creating resized image: %s", filename);
    al_set_target_bitmap(resized_bmp);
    al_draw_scaled_bitmap(loaded_bmp, 0, 0, al_get_bitmap_width(loaded_bmp), al_get_bitmap_height(loaded_bmp), 0, 0, w, h, 0);
    al_set_target_bitmap(prev_target);
    al_destroy_bitmap(loaded_bmp);

    game_log("resized image: %s", filename);

    return resized_bmp;
}

bool pnt_in_rect(int px, int py, int x, int y, int w, int h) {
    return (px >= x-w && px <= x+w && py >= y-h && py <= y+h);
}

// +=================================================================+
// | Code below is for debugging purpose, it's fine to remove it.    |
// +=================================================================+

void game_abort(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    game_vlog(format, arg);
    va_end(arg);
    fprintf(stderr, "error occured, exiting after 2 secs");
    // Wait 2 secs before exiting.
    al_rest(2);
    // Force exit program.
    exit(1);
}

void game_log(const char* format, ...) {
#ifdef LOG_ENABLED
    va_list arg;
    va_start(arg, format);
    game_vlog(format, arg);
    va_end(arg);
#endif
}

void game_vlog(const char* format, va_list arg) {
#ifdef LOG_ENABLED
    static bool clear_file = true;
    vprintf(format, arg);
    printf("\n");
    FILE* pFile = fopen("log.txt", clear_file ? "w" : "a");
    if (pFile) {
        vfprintf(pFile, format, arg);
        fprintf(pFile, "\n");
        fclose(pFile);
    }
    clear_file = false;
#endif
}
