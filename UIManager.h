#ifndef __UIMANAGER_H__ 
#define __UIMANAGER_H__

#ifndef MAX_BUTTONS
#define MAX_BUTTONS 12
#endif

#ifndef MAX_POTS
#define MAX_POTS 6
#endif

// TODO - use inturrupts for the buttons
// use the bounce library for the button(float)(float)(float)(float)s

class UIManager {
    public:
        UIManager(uint16_t _polling_delay, bool print);
        bool addBut(int pin, bool reverse, bool pull_up, bool *val, String name);
        bool addPot(int pin, bool reverse, float play, float *val, String name);

        // bool updatePotMapping(uint8_t which, double min, double max);

        void setup();
        void testControls(uint8_t times, uint16_t wait);
        void initalRead();

        bool update();
        void printAll();

    private:
        //////////// For managing the updating rate /////////////
        bool print_updates = false;
        uint16_t polling_delay = 0;
        elapsedMillis last_reading;

        // Everything to keep the buts in order
        uint8_t num_buttons = 0;
        bool but_vals[MAX_BUTTONS];
        bool *linked_but_vals[MAX_BUTTONS];
        String but_names[MAX_BUTTONS];
        int but_pins[MAX_BUTTONS];
        bool but_reverse[MAX_BUTTONS];
        // will flip the reading orientation for buts when needed
        bool but_active[MAX_BUTTONS];
        bool but_pullup[MAX_BUTTONS];

        // Everything to keep the pots in order
        uint8_t num_pots    = 0;
        float pot_vals[MAX_POTS];
        float *linked_pot_vals[MAX_POTS];
        String pot_names[MAX_POTS];
        int pot_pins[MAX_POTS];
        // will flip the reading orientation for pots when needed
        bool pot_reverse[MAX_POTS];
        bool pot_active[MAX_POTS];
        // this is how much the pot reading has to change in order 
        // for a value update to be enacted
        float pot_play[MAX_POTS];
};

UIManager::UIManager(uint16_t _polling_delay, bool print) {
    polling_delay = _polling_delay;
    print_updates = print;
}

bool UIManager::addBut(int pin, bool reverse, bool pull_up, bool *val, String name) {
    if (num_buttons < MAX_BUTTONS) {
        but_names[num_buttons] = name;
        but_pullup[num_buttons] = pull_up;
        but_active[num_buttons] = true;
        but_reverse[num_buttons] = reverse;
        but_pins[num_buttons] = pin;
        linked_but_vals[num_buttons] = val; // store the pointer to the linked value
        Serial.print("Added button ");Serial.print(but_names[num_buttons]);
        Serial.print("\t#");Serial.println(num_buttons);
        num_buttons++;
        return true;
    } else {
        Serial.println("WARNING --- TOO MANY BUTTONS EXIST IN THE UIManager ALREADY");
        Serial.println("WARNING --- IGNORING CALL TO addBut()");
        return false;
    }
}

bool UIManager::addPot(int pin, bool reverse, float play, float *val, String name) {
    if (num_pots < MAX_POTS){
        pot_play[num_pots] = play;
        pot_names[num_pots] = name;
        pot_active[num_pots] = true;
        pot_reverse[num_pots] = reverse;
        pot_pins[num_pots] = pin;
        linked_pot_vals[num_pots] = val; // store the pointer to the linked value
        Serial.print("Added pot ");Serial.print(pot_names[num_pots]);
        Serial.print(" #");Serial.println(num_pots);
        num_pots++;
        return true;
    } else {
        Serial.println("WARNING --- TOO MANY POTS EXIST IN THE UIManager ALREADY");
        Serial.println("WARNING --- IGNORING CALL TO addPot()");
        return false;
    }
}

void UIManager::setup() {
    Serial.println("-----------------------------------------------------------------");
    Serial.println("--------------- calling pinMode for user controls  --------------");
    Serial.println("-----------------------------------------------------------------");
    for (int i = 0; i < num_buttons; i++) {
        Serial.print("discrete control #");
        Serial.print(i);
        if (but_pullup[i] == false) {
            Serial.println(" set to input");
            pinMode(but_pins[i], INPUT);
        } else if (but_pullup[i] == true) {
            Serial.println(" set to input with internal pullup");
            pinMode(but_pins[i], INPUT_PULLUP);
        }else {
            Serial.println("WARNING - but_pullup[i] is not set correctly");
        }
    }
    for (int i = 0; i < num_pots; i++) {
        Serial.print("continuous control #");
        Serial.print(i);
        Serial.println(" set to input");
        pinMode(pot_pins[i], INPUT);
    }
    delay(1000);
    Serial.println("finished with pinMode()");
    testControls(20, 50);
    initalRead();
}

void UIManager::testControls(uint8_t times, uint16_t wait) {
    Serial.println("-----------------------------------------------------------------");
    Serial.println("--------------- Starting UIControl Testing ----------------------");
    Serial.println("-----------------------------------------------------------------");
    for (int i = 0; i < num_buttons; i++) {
        but_vals[i] = digitalRead(but_pins[i]);
    }
    Serial.println("Testing Buttons Now");
    Serial.println("-----------------------------------------------------------------");
    for (int t = 0; t < times; t++) {
        // read once for reference
        delay(wait);
        for (int i = 0; i < num_buttons; i++) {
            if (but_vals[i] != digitalRead(but_pins[i])) {
                but_active[i] = false;
                Serial.print("WARNING BUTTON " );
                Serial.print(but_names[i]);
                Serial.println(" is returning differing values, flagging as inactive");
            } else {
                Serial.print(" . ");
                // flip the state of the button quickly
                // if there is no button present then this should
                // encourage unstable readings
                // likewise, if there is no pull-up this will 
                // cause the same issues
                pinMode(but_pins[i], OUTPUT);
                digitalWrite(but_pins[i], !but_vals[i]);
                digitalWrite(but_pins[i], but_vals[i]);
                if (but_pullup[i] == false) {
                    pinMode(but_pins[i], INPUT);
                } else {
                    pinMode(but_pins[i], INPUT_PULLUP);
                }
            }
        }
        Serial.println();
        delay(wait);
    }
    Serial.println("-----------------------------------------------------------------");
    Serial.println("Testing Pots Now");
    for (int i = 0; i < num_pots; i++) {
        pot_vals[i] = (float)analogRead(pot_pins[i])/1024.0;
    }
    delay(wait);
    for (int t = 0; t < times; t++) {
        for (int i = 0; i < num_pots; i++) {
            // read once for reference
            float reading = (float)analogRead(pot_pins[i])/1024.0;
            if (reading > (pot_vals[i] + pot_play[i]) ||
                    reading < (pot_vals[i] - pot_play[i])) {
                pot_active[i] = false;
                Serial.print("WARNING POT " );
                Serial.print(pot_names[i]);
                Serial.println(" is returning differing values, flagging as inactive");
            } else {
                Serial.print(" . ");
            }
        }
        delay(wait);
        Serial.println();
    }

}

void UIManager::initalRead() {
    // first determine if the buttons and switches are operating properly
    // TODO
    Serial.println("-----------------------------------------------------------------");
    Serial.println("------------------ Collecting Initial Readings -----------------");
    Serial.println("-----------------------------------------------------------------");
    // IF and only if they are operating correctly then the linked value should be changed
    for (int i = 0; i < num_buttons; i++) {
        if (but_active[i] == true) {
            bool reading = digitalRead(but_pins[i]);
            // only update the values if the readings has changed since last time
            if (but_vals[i] !=  reading) {
                // De-reference the pointer so the value is now updated to the reading
                *linked_but_vals[i] = reading;
                but_vals[i] = reading;
                if (print_updates == true) {
                    Serial.print(but_names[i]);Serial.print(" value has changed to: ");
                    Serial.println(*linked_but_vals[i]);
                }
            }
        }
    }
    // update the pot readings
    for (int i = 0; i < num_pots; i++) {
        if (pot_active[i] == true) {
            float freading;
            if (pot_reverse[i] == false) {
                freading = (float)analogRead(pot_pins[i]) / 1024.0;
            } else {
                freading = 1.0 - ((float)analogRead(pot_pins[i]) / 1024.0);
            }
            // now check to see if it has changed enough for the new reading to be integrated
            if (freading > pot_vals[i] + (pot_vals[i] * pot_play[i]) ||
                    freading < pot_vals[i] - (pot_vals[i] * pot_play[i])) {
                pot_vals[i] = freading;
                *linked_pot_vals[i] = freading;
                if (print_updates == true) {
                    Serial.print(pot_names[i]);Serial.print(" value has changed to: ");
                    Serial.println(*linked_pot_vals[i], 5);
                }
            }
        }
    }
    Serial.println("-----------------------------------------------------------------");
    Serial.println("------------------ Finished Initial Readings --------------------");
    Serial.println("-----------------------------------------------------------------");
}

bool UIManager::update() {
    // update the buttons readings
    if (last_reading < polling_delay) {
        return false;
    }
    for (int i = 0; i < num_buttons; i++) {
        if (but_active[i] == true){
            bool reading = digitalRead(but_pins[i]);
            // only update the values if the readings has changed since last time
            if (but_vals[i] !=  reading) {
                // De-reference the pointer so the value is now updated to the reading
                *linked_but_vals[i] = reading;
                but_vals[i] = reading;
                if (print_updates == true) {
                    Serial.print(but_names[i]);Serial.print(" value has changed to: ");
                    Serial.println(*linked_but_vals[i]);
                }
            }
        }
    }
    // update the pot readings
    for (int i = 0; i < num_pots; i++) {
        if (pot_active[i] == true) {
            float reading;
            if (pot_reverse[i] == false) {
                reading = (float)analogRead(pot_pins[i]) / 1024.0;
            } else {
                reading = 1.0 - ((float)analogRead(pot_pins[i]) / 1024.0);
            }
            // now check to see if it has changed enough for the new reading to be integrated
            if (reading > (pot_vals[i] + pot_play[i]) ||
                    reading < (pot_vals[i] - pot_play[i])) {
                *linked_pot_vals[i] = reading;
                pot_vals[i] = reading;
                if (print_updates == true) {
                    Serial.print(pot_names[i]);Serial.print(" value has changed to: ");
                    Serial.println(*linked_pot_vals[i], 5);
                }
            }
        }
    }
    // update the Serial Commands TODO
    last_reading = 0;
    return true;
}


void UIManager::printAll() {
    Serial.println("|||||||||||||||||||||||||||||||||||||||||||");
    Serial.println("---------- UIManager.printAll() -----------");
    Serial.println("-------------------------------------------");
    Serial.println("----------- printing buttons --------------");
    Serial.println("-------------------------------------------");
    for (int i = 0; i < num_buttons; i++) {
        Serial.print(but_names[i]);Serial.print(":\t");Serial.println(but_vals[i]);
    }
    Serial.println("-------------------------------------------");
    Serial.println("------------ printing pots ----------------");
    Serial.println("-------------------------------------------");
    for (int i = 0; i < num_pots; i++) {
        Serial.print(pot_names[i]);Serial.print(":\t");Serial.println(pot_vals[i]);
    }
    Serial.println("-------------------------------------------");
    Serial.println("--------------- finished ------------------");
    Serial.println("|||||||||||||||||||||||||||||||||||||||||||");
}

#endif // __UIMANAGER_H__
