#include <Bounce2.h>
#include "FastLED.h"

#define NUM_LEDS 104
#define DATA_PIN 12
#define BUTTON_1 11
#define BUTTON_2 10

#define BUTTON_DEBOUNCE_MS 2

#define DEADZONE_SIZE 5
#define SPRING_LENGTH 40

int paddle_size = 10;
int ball_position = 15;

float ball_velocity_f = 0.0;
float ball_position_f = 0.0;

Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();
int b1_last = 0;
int b2_last = 0;

#define STATE_SPRING 0
#define STATE_PLAY 1
#define STATE_GAME_OVER 2
int game_state = STATE_SPRING;

CRGB leds[NUM_LEDS];

void setup() {
 FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS); 
 debouncer1.attach(BUTTON_1);
 debouncer2.attach(BUTTON_2);
 debouncer1.interval(BUTTON_DEBOUNCE_MS);
 debouncer2.interval(BUTTON_DEBOUNCE_MS);
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
  int new_ball_position = (int)((((float)NUM_LEDS) / 2.0) + ball_position_f - 0.5);
  
  //int move_sign = (int)sign(new_ball_position - ball_position);
  //if (move_sign != 0) {
  //  int curr_update_pos = ball_position;
  //  while (curr_update_pos != new_ball_position) {
  //    curr_update_pos += move_sign;
  //    leds[curr_update_pos] = play_render_led(curr_update_pos);
  //  }
  //}
  
  ball_position = new_ball_position;
}

//void render_board() {
//  for (int i = 0
//}

void play_render_led(int num) {
  if (num == ball_position) {
    leds[num] = CRGB::White;
    return;
  } else if (num < DEADZONE_SIZE) {
    leds[num] = CRGB(0, 0, 10);
    return;
  } else if (num >= NUM_LEDS - DEADZONE_SIZE) {
    leds[num] = CRGB(10, 0, 0);
    return;
  } else if (num < DEADZONE_SIZE + paddle_size || num >= NUM_LEDS - (DEADZONE_SIZE + paddle_size) ) {
    leds[num] = CRGB(0, 10, 0);
    return;
  }

  leds[num] = CRGB::Black;
}

void state_play() {
  int b1_last = debouncer1.read();
  int b2_last = debouncer2.read();
  
  for (int i = 0; i < NUM_LEDS; i++) {
    play_render_led(i);
  }
  
  const float paddle_start = (((float)NUM_LEDS) / 2.0) - DEADZONE_SIZE - paddle_size;
  const float deadzone_start = (((float)NUM_LEDS) / 2.0) - DEADZONE_SIZE;
  
  while (1) {
    debouncer1.update();
    debouncer2.update();
    int b1 = debouncer1.read();
    int b2 = debouncer2.read();
    
    if (b1 && !b1_last) {
      if (ball_position_f < -paddle_start && ball_velocity_f < 0) {
        ball_velocity_f = abs(ball_velocity_f);
      }
    }
    if (b2 && !b2_last) {
      if (ball_position_f > paddle_start && ball_velocity_f > 0) {
        ball_velocity_f = -abs(ball_velocity_f);
      }
    }
    b1_last = b1;
    b2_last = b2;
    
    ball_velocity_f *= 1.0002;
    ball_position_f += ball_velocity_f;
    
    if (ball_position_f <= -deadzone_start) {
      game_state = STATE_GAME_OVER;
      return;
    }
    if (ball_position_f > deadzone_start) {
      game_state = STATE_GAME_OVER;
      return;
    }
  
    update_ball_pos();
    
    for (int i = ball_position-6; i <= ball_position+6; i++) {
      if (i >= 0 && i < NUM_LEDS+1) {
        play_render_led(i);
      }
    }
    
    FastLED.show();
    delay(2);
  }
}

void state_spring() {
  
  ball_position = NUM_LEDS / 2;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    play_render_led(i);
  }
  leds[ball_position+1] = CRGB::White;
  
  const float drag = 0.96;
  const float springyness = 0.002;
  
  float velocity = 0.0;
  
  int do_launch = 0;
  
  while (1) {
    debouncer1.update();
    debouncer2.update();
    int b1 = debouncer1.read();
    int b2 = debouncer2.read();
    
    int running = !(b1 || b2);
    
    if (b1) {
      ball_position_f += (0.1 * (((-ball_position_f) / SPRING_LENGTH) - 1));
      ball_velocity_f = 0;
    }
    if (b2) {
      ball_position_f -= (0.1 * (((ball_position_f) / SPRING_LENGTH) - 1));
      ball_velocity_f = 0;
    }
    if (abs(ball_position_f) > 10 && !running) {
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
    
    for (int i = ball_position-5; i <= ball_position+5; i++) {
      if (i >= 0 && i < NUM_LEDS+1) {
        play_render_led(i);
      }
    }
    
    FastLED.show();
    delay(2);
  }
  
}

void state_game_over() {
  int winner = (int)sign(ball_position_f);
  
  for (int i = 0; i < 3; i++) {
    for (int num = 0; num < NUM_LEDS; num++) {
      leds[num] = CRGB::Black;
    }
    FastLED.show();
    
    delay(500);
    
    for (int num = 0; num < NUM_LEDS; num++) {
      if (num <= NUM_LEDS / 2) {
        leds[num] = CRGB(0, 20, 0);
      } else {
        leds[num] = CRGB(20, 0, 0);
      }
    }
    FastLED.show();
    
    delay(500);
  }
  
  ball_position_f = 0.0;
  ball_velocity_f = 0.0;
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
  
  //leds[0] = CRGB::Red;
  FastLED.show();
  delay(2);
  
  //leds[0] = CRGB::Black;
  //FastLED.show();
  //delay(500);
}
