#ifndef __UIMANAGER_H__ 
#define __UIMANAGER_H__

#ifndef MAX_BUTTONS
#define MAX_BUTTONS 10
#endif

#ifndef MAX_POTS
#define MAX_POTS 4
#endif

// TODO - use inturrupts for the buttons
// use the bounce library for the button(float)(float)(float)(float)s

class UIManager {
    public:
        UIManager(uint16_t _polling_delay, bool print);
        bool addBut(int pin, bool pull_up, int low, int high, bool low_active, int *val, String name);
        bool addPot(int pin, bool reverse, float play, float *val, String name);

        bool addPotRange(uint8_t idx, double min, double mid, double max);

        // bool updatePotMapping(uint8_t which, double min, double max);

        void setup(bool test_controls);
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
        int but_vals[MAX_BUTTONS];
        int *linked_but_vals[MAX_BUTTONS];
        int but_lows[MAX_BUTTONS];
        int but_low_active[MAX_BUTTONS];
        int but_highs[MAX_BUTTONS];
        String but_names[MAX_BUTTONS];
        int but_pins[MAX_BUTTONS];

        // will flip the reading orientation for buts when needed
        bool but_active[MAX_BUTTONS];
        bool but_pullup[MAX_BUTTONS];

        // Everything to keep the pots in order
        uint8_t num_pots    = 0;
        float pot_vals[MAX_POTS];
        float pot_raw_vals[MAX_POTS];
        float *linked_pot_vals[MAX_POTS];
        float pot_min[4];
        float pot_mid[4];
        float pot_max[4];
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

bool UIManager::addBut(int pin, bool pull_up, int low, int high, bool low_active, int *val, String name) {
    if (num_buttons < MAX_BUTTONS) {
        but_low_active[num_buttons] = low_active;
        but_names[num_buttons] = name;
        but_pullup[num_buttons] = pull_up;
        but_active[num_buttons] = true;
        but_pins[num_buttons] = pin;
        but_lows[num_buttons] = low;
        but_highs[num_buttons] = high;
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

bool UIManager::addPotRange(uint8_t idx, double min, double mid, double max) {
    if (idx < num_pots) {
        pot_min[idx] = min;
        pot_mid[idx] = mid;
        pot_max[idx] = max;
        return true;
    } else {
        return false;
    }
}

void UIManager::setup(bool test_controls) {
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
    if (test_controls) {
        testControls(20, 50);
    }
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
                Serial.print(but_vals[i]);
                Serial.print("\t");
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
        int itemp;
        if (pot_reverse[i] == false) {
            itemp = analogRead(pot_pins[i]);
        } else {
            itemp = 1024 - analogRead(pot_pins[i]);
        }

        // scale either from min to mid or from mid to max depending on pot value
        if (itemp < 512) {
            pot_vals[i] = (itemp/512 * (pot_mid[i] - pot_min[i])) + pot_min[i];
        } else {
            pot_vals[i] = ((itemp - 512) / 512) * (pot_max[i] - pot_mid[i]) + pot_mid[i];
        }
    }

    delay(wait);
    for (int t = 0; t < times; t++) {
        for (int i = 0; i < num_pots; i++) {
            // read once for reference
            float reading;
            int itemp;
            if (pot_reverse[i] == false) {
                itemp = analogRead(pot_pins[i]);
            } else {
                itemp = 1024 - analogRead(pot_pins[i]);
            }
            // scale either from min to mid or from mid to max depending on pot value
            if (itemp < 512) {
                reading  = (itemp/512 * (pot_mid[i] - pot_min[i])) + pot_min[i];
            } else {
                reading  = ((itemp - 512) / 512) * (pot_max[i] - pot_mid[i]) + pot_mid[i];
            }

            if (reading > (pot_vals[i] + pot_play[i]) ||
                    reading < (pot_vals[i] - pot_play[i])) {
                pot_active[i] = false;
                Serial.print("WARNING POT " );
                Serial.print(pot_names[i]);
                Serial.println(" is returning differing values, flagging as inactive");
            } else {
                Serial.print(reading);
                Serial.print("\t");
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
            int reading;
            bool temp = digitalRead(but_pins[i]);
            if (temp == true) {
                reading = but_highs[i];
            } else {
                reading = but_lows[i];
            }
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
            int itemp;
            if (pot_reverse[i] == false) {
                itemp = analogRead(pot_pins[i]);
            } else {
                itemp = 1024 - analogRead(pot_pins[i]);
            }
            freading = (float)analogRead(pot_pins[i]) / 1024.0;
            pot_raw_vals[i] = freading;
            if (itemp < 512) {
                pot_vals[i] = ((float)itemp/512.0 * (pot_mid[i] - pot_min[i])) + pot_min[i];
            } else {
                pot_vals[i] = (((float)itemp - 512) / 512.0) * (pot_max[i] - pot_mid[i]) + pot_mid[i];
            }
            *linked_pot_vals[i] = pot_vals[i];
            if (print_updates == true) {
                Serial.print(pot_names[i]);Serial.print(" value has changed to: ");
                Serial.println(*linked_pot_vals[i], 5);
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
            int reading;
            bool temp = digitalRead(but_pins[i]);
            if (temp == true) {
                reading = but_highs[i];
            } else {
                reading = but_lows[i];
            }
            // only update the values if the readings has changed since last time
            if (but_vals[i] !=  reading) {
                // De-reference the pointer so the value is now updated to the reading
                if (reading == but_highs[i] || (reading == but_lows[i] && but_low_active[i]) ) { 
                    *linked_but_vals[i] = reading;
                    but_vals[i] = reading;
                    if (print_updates == true) {
                        Serial.print(but_names[i]);Serial.print(" value has changed to: ");
                        Serial.println(*linked_but_vals[i]);
                    }
                }
            }
        }
    }
    // update the pot readings

        // scale either from min to mid or from mid to max depending on pot value
    for (int i = 0; i < num_pots; i++) {
        if (pot_active[i] == true) {
            int itemp;
            if (pot_reverse[i] == false) {
                itemp = analogRead(pot_pins[i]);
            } else {
                itemp = 1024 - analogRead(pot_pins[i]);
            }
            float reading = (float)itemp / 1024.0;

            // now check to see if it has changed enough for the new reading to be integrated
            if (reading > (pot_raw_vals[i] + pot_play[i]) ||
                            reading < (pot_raw_vals[i] - pot_play[i])) {
                if (itemp < 512) {
                    pot_vals[i] = ((float)itemp/512.0 * (pot_mid[i] - pot_min[i])) + pot_min[i];
                } else {
                    pot_vals[i] = (((float)itemp - 512) / 512.0) * (pot_max[i] - pot_mid[i]) + pot_mid[i];
                }
                *linked_pot_vals[i] = pot_vals[i];
                if (print_updates == true) {
                    Serial.print(pot_names[i]);Serial.print(" value has changed to: ");
                    Serial.println(*linked_pot_vals[i], 5);
                }
            }
            pot_raw_vals[i] = reading;
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
        Serial.print(but_names[i]);Serial.print(":\t");
        Serial.println(but_vals[i]);
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
