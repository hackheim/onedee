#include "Arduino.h"
#include "FastLED.h"

#include "main.hpp"
#include "player_controls.hpp"
#include "config.hpp"

int paddle_size = 10;
int ball_position = 15;

float ball_velocity_f = 0.0;
float ball_position_f = 0.0;

PlayerControls p1 = PlayerControls();
PlayerControls p2 = PlayerControls();

#define STATE_SPRING 0
#define STATE_PLAY 1
#define STATE_GAME_OVER 2
int game_state = STATE_SPRING;

CRGB leds[NUM_LEDS];

unsigned long next_frame = 0;

bool p1_primed;
bool p2_primed;

class BoardRenderState {
    int player_1_deadzone_size;
    CRGB player_1_deadzone_color;
    int player_1_area_size;
    CRGB player_1_area_color;
    bool player_1_primed;

    int player_2_deadzone_size;
    CRGB player_2_deadzone_color;
    int player_2_area_size;
    CRGB player_2_area_color;
    bool player_2_primed;

    int ball_position;
    int ball_color;
    int ball_trail_length;
};

void setup() {
    FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS); 

    p1.init(BUTTON_1);
    p2.init(BUTTON_2);

    Serial.begin(9600);
}

float sign(float num) {
    if (num > 0) {
        return 1.0;
    } else if (num < 0) {
        return -1.0;
    }
    return 0.0;
}

void update_ball_pos() {
    ball_position = (int)((((float)NUM_LEDS) / 2.0) + ball_position_f - 0.5);
}

void frame() {
    if (millis() >= next_frame) {
        Serial.println("overrun!");
    }
    while(millis() <= next_frame);
    next_frame = millis() + 10;
}

void render_board() {

    // Empty out
    for (int num = 0; num < NUM_LEDS; num++) {
        leds[num] = CRGB::Black;
    }

    int paddle_limit = paddle_size+DEADZONE_SIZE;

    // Player1 paddle
    CRGB p1_paddle_color = PADDLE_COLOR;
    if (p1_primed) {
        p1_paddle_color = PADDLE_COLOR_PRIMED;
    }
    if (p1.in_timeout()) {
        p1_paddle_color = PADDLE_COLOR_USED;
    }
    for (int num = DEADZONE_SIZE-1; num < paddle_limit; num++) {
        leds[num] = p1_paddle_color;
    }
    // Player1 deadzone
    for (int num = 0; num < DEADZONE_SIZE; num++) {
        leds[num] = P1_COLOR;
    }

    // Player2 paddle
    CRGB p2_paddle_color = PADDLE_COLOR;
    if (p2_primed) {
        p2_paddle_color = PADDLE_COLOR_PRIMED;
    }
    if (p2.in_timeout()) {
        p2_paddle_color = PADDLE_COLOR_USED;
    }
    for (int num = NUM_LEDS - paddle_limit; num < NUM_LEDS; num++) {
        leds[num] = p2_paddle_color;
    }
    // Player2 deadzone
    for (int num = NUM_LEDS - DEADZONE_SIZE; num < NUM_LEDS; num++) {
        leds[num] = P2_COLOR;
    }

    // Ball trail
    if (ball_velocity_f > 0.001) {
        int velocity_sign = sign(ball_velocity_f);
        leds[ball_position - velocity_sign] = BALL_TRAIL_COLOR;
    }

    // Ball
    leds[ball_position] = BALL_TRAIL_COLOR;

}

void state_play() {
    int b1_hold_time = 0;
    int b2_hold_time = 0;

    const float paddle_start = (((float)NUM_LEDS) / 2.0) - DEADZONE_SIZE - paddle_size - GRACE_SIZE;
    const float deadzone_start = (((float)NUM_LEDS) / 2.0) - DEADZONE_SIZE;

    render_board();

    while (1) {
        // Update inputs
        p1.tick();
        p2.tick();

        // Check for triggers
        if (ball_position_f < -paddle_start && ball_velocity_f < 0) {
            p1_primed = true;
            if (p1.trigger) {
                ball_velocity_f = abs(ball_velocity_f);
            }
        } else {
            p1_primed = false;
        }
        if (ball_position_f > paddle_start && ball_velocity_f > 0) {
            p2_primed = true;
            if (p2.trigger) {
                ball_velocity_f = -abs(ball_velocity_f);
            }
        } else {
            p2_primed = false;
        }

        // Apply acceleration and velocity
        ball_velocity_f *= PLAY_ACCELERATION;
        ball_position_f += ball_velocity_f;

        // Check victory condition
        if (ball_position_f <= -deadzone_start) {
            game_state = STATE_GAME_OVER;
            return;
        }
        if (ball_position_f > deadzone_start) {
            game_state = STATE_GAME_OVER;
            return;
        }

        // Render board
        update_ball_pos();
        render_board();

        // Update LEDs
        FastLED.show();

        // Sleep until end of frame
        frame();
    }
}

void state_spring() {

    ball_position = NUM_LEDS / 2;

    render_board();
    leds[ball_position+1] = CRGB::White;

    p1_primed = false;
    p2_primed = false;

    const float drag = LAUNCH_SPRING_DRAG;
    const float springyness = LAUNCH_SPRING_SPRINGYNESS;

    float velocity = 0.0;

    int do_launch = 0;

    while (1) {
        p1.tick();
        p2.tick();
        int b1 = p1.hold != 0;
        int b2 = p2.hold != 0;

        int running = !(b1 || b2);

        if (b1) {
            ball_position_f += (LAUNCH_SPRING_PULL_SPEED * (((-ball_position_f) / LAUNCH_SPRING_LENGTH) - 1));
            ball_velocity_f = 0;
        }
        if (b2) {
            ball_position_f -= (LAUNCH_SPRING_PULL_SPEED * (((ball_position_f) / LAUNCH_SPRING_LENGTH) - 1));
            ball_velocity_f = 0;
        }
        if (abs(ball_position_f) > LAUNCH_SPRING_IGNORE_ZONE && !running) {
            do_launch = 1;
        }

        if (do_launch && (ball_position_f*sign(ball_velocity_f)) > 0) {
            game_state = STATE_PLAY;
            return;
        }

        if (running) {
            ball_velocity_f = (ball_velocity_f + (-ball_position_f * springyness)) * drag;
            ball_position_f += ball_velocity_f;
        }

        update_ball_pos();
        render_board();

        FastLED.show();

        frame();
    }

}

void state_game_over() {
    int winner = (int)sign(ball_position_f);

    int counter = 0;

    while (counter < 80) {
        float animation_progress = counter / 60.0;

        render_board();

        for (int i = 0; i < NUM_LEDS; i++) {
            float rel_pos = (i / (float)NUM_LEDS);

            if (rel_pos < animation_progress) {
                if (animation_progress - rel_pos < 0.1f) {
                    float r = 1.0f / (animation_progress - rel_pos);
                    leds[i].r += (int)(32.0f / r);
                    leds[i].g += (int)(2.0f / r);
                } else {
                    int flash = 0;
                    if (random(0, 4) == 0) flash = random(0, 20);
                    leds[i] = CRGB(32 + flash, 2, 0);
                }

            }
        }

        FastLED.show();

        counter++;
        frame();
    }

    //counter = 0;
    //while (counter < 100) {
    //    float animation_progress = counter / 100.0;

    //    render_board();

    //    for (int i = 0; i < NUM_LEDS; i++) {
    //        int flash = 0;
    //        if (random(0, 4) == 0) flash = random(0, 20);

    //        leds[i].r += (int)((32 + flash) / animation_progress);
    //        leds[i].g += (int)((12) / animation_progress);
    //    }

    //    FastLED.show();
    //    counter++;
    //    frame();
    //}

    ball_position_f = 0.0;
    ball_velocity_f = 0.0;

    render_board();
    FastLED.show();

    game_state = STATE_SPRING;
}

void loop() {

    if (game_state == STATE_SPRING) {
        state_spring();
    } else if (game_state == STATE_PLAY) {
        state_play();
    } else if (game_state == STATE_GAME_OVER) {
        state_game_over();
    }

    FastLED.show();
    delay(2);
}
