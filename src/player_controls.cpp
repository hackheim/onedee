#include "player_controls.hpp"
#include "config.hpp"

PlayerControls::PlayerControls() {
    this->button = Bounce();
}

void PlayerControls::init(int button_pin) {
    this->button.attach(button_pin);
    this->button.interval(BUTTON_DEBOUNCE_MS);
}

void PlayerControls::tick() {
    this->button.update();
    int curr = this->button.read();
    this->trigger = 0;

    if (trigger_timeout <= 0) {
        if (curr && !this->last) {
            this->trigger = 1;
            this->trigger_timeout = 60;
        }
    } else {
        trigger_timeout -= 1;
    }

    if (curr) {
        this->hold += 1;
    } else {
        this->hold = 0;
    }

    this->last = curr;
}

bool PlayerControls::in_timeout() {
    return this->trigger_timeout > 0;
}
