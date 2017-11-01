#include "Bounce2.h"

class PlayerControls {

    public:
        int trigger = 0;
        int hold = 0;

        PlayerControls();
        void init(int button_pin);

        void tick();

    private:
        Bounce button;
        int last = 0;

        int trigger_timeout = 0;

};
