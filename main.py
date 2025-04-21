import RPi.GPIO as GPIO
import time
import signal
import sys
import threading
import time
from datetime import datetime

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

menu = ["Slot 1", "Slot 2", "Slot 3", "Slot 4", "Status", "Exit"]
setting = {"Slot 1": {"Pill Size": 0, "Interval": 0, "Next Alert": 0, "Amount": 0, "Degree": 90}, "Slot 2": {"Pill Size": 0, "Interval": 0, "Next Alert": 0, "Amount": 0, "Degree": 90}, "Slot 3": {"Pill Size": 0, "Interval": 0, "Next Alert": 0, "Amount": 0, "Degree": 90}, "Slot 4": {"Pill Size": 0, "Interval": 0, "Next Alert": 0, "Amount": 0, "Degree": 90}}
settingOptions = ["Pill Size", "Interval", "Amount", "Exit"]
slot = "Slot 1"
menuOp = 0
settingOp = 0
settingVal = False
screen = "Home"

# Servo
GATE_PIN = 18
SIZE_PIN = 19
MIN_DUTY = 2.5
MAX_DUTY = 12.5
FREQUENCY = 50

# Define GPIO pins
CLK_PIN = 17    # Clock pin
DT_PIN = 18     # Data pin
SW_PIN = 27     # Switch pin for button press

# Global variables
counter = 0
clk_last_state = None
button_pressed = False
lock = threading.Lock()  # For thread safety when updating counter

def setup():
    """
    Arduino-like setup function - runs once at the beginning
    """
    global clk_last_state, Gate_pwm, Size_pwm
    
    print("Setting up...")
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(CLK_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(DT_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(SW_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    
    # Initialize the last state
    clk_last_state = GPIO.input(CLK_PIN)

    GPIO.setup(GATE_PIN, OUTPUT)
    GPIO.setup(SIZE_PIN, OUTPUT)
    
    # Create PWM instance with frequency
    Gate_pwm = GPIO.PWM(GATE_PIN, FREQUENCY)
    Size_pwm = GPIO.PWM(SIZE_PIN, FREQUENCY)

    set_angle(Gate_pwm, 0)
    time.sleep(1)
    set_angle(Size_pwm, 0)
    time.sleep(1)
    
    # Start PWM with duty cycle 0
    Gate_pwm.start(0)
    Size_pwm.start(0)
    
    print("Setup complete!")

def loop():
    """
    Arduino-like loop function - runs continuously
    """
    global menuOp, settingOp, slot, clk_last_state, counter, screen, settingVal
    
    # Read current state of CLK pin
    clk_current = GPIO.input(CLK_PIN)
    dt_current = GPIO.input(DT_PIN)

    current_time = datetime.now()

    if GPIO.input(SW_PIN) == GPIO.LOW:
        # Simple debounce
        time.sleep(0.05)  
        if GPIO.input(SW_PIN) == GPIO.LOW:  # Still pressed after debounce
            print("Button pressed!")
            
            # Handle button press based on current screen
            if screen == "Home":
                screen = "Menu"
                print("Entering Menu screen")
            
            elif screen == "Menu":
                if menu[menuOp] == "Exit":
                    screen = "Home"
                    print("Returning to Home screen")
                elif menu[menuOp] == "Status":
                    screen = "Status"
                    print("Entering Status screen")
                else:  # Selected a slot
                    screen = "Setting"
                    settingOp = 0  # Reset setting option to first option
                    print(f"Entering Setting screen for {slot}")
            
            elif screen == "Setting":
                if settingVal is False:  # Selecting a setting option
                    if settingOptions[settingOp] == "Exit":
                        screen = "Menu"
                        print("Returning to Menu screen")
                    else:
                        settingVal = True
                        print(f"Entering value edit mode for {settingOptions[settingOp]}")
                else:  # Exiting value edit mode
                    settingVal = False
                    print("Exiting value edit mode")
            
            elif screen == "Status":
                screen = "Menu"
                print("Returning to Menu screen")
                
            # Wait for button release to prevent multiple triggers
            while GPIO.input(SW_PIN) == GPIO.LOW:
                time.sleep(0.01)
                
            time.sleep(0.2)  # Additional debounce delay
    
    # Check for rotation by comparing with last CLK state
    if clk_current != clk_last_state:
        # If DT state is different from CLK state - clockwise rotation
        if dt_current != clk_current:
            if screen == "Menu":
                menuOp = (menuOp + 1) % len(menu)
                slot = menu[menuOp]
                print(f"Menu option: {list(menu)[menuOp]}")
            elif screen == "Setting" and settingVal is False:
                settingOp = (settingOp + 1) % len(settingOptions)
                print(f"Menu option: {list(settingOptions)[settingOp]}")
            elif screen == "Setting" and settingVal is True:
                if settingOptions[settingOp] == "Pill Size":
                    setting[slot]["Pill Size"] = (setting[slot]["Pill Size"] + 1) % 5
                elif settingOptions[settingOp] == "Interval":
                    setting[slot]["Interval"] = (setting[slot]["Interval"] + 1) % 48
                    if setting[slot]["Interval"] == 0:
                        setting[slot]["Next Alert"] = 0
                    else:
                        nextAlert = (60 * (current_time.hour + setting[slot]["Interval"])) + current_time.minute
                        setting[slot]["Next Alert"] = nextAlert
                elif settingOptions[settingOp] == "Amount":
                    setting[slot]["Amount"] = (setting[slot]["Amount"] + 1) % 10
        else:
            # Counterclockwise rotation
            if screen == "Menu":
                menuOp = (menuOp - 1) % len(menu)
                slot = menu[menuOp]
                print(f"Menu option: {list(menu)[menuOp]}")
            elif screen == "Setting" and settingVal is False:
                settingOp = (settingOp - 1) % len(settingOptions)
                print(f"Menu option: {list(settingOptions)[settingOp]}")
            elif screen == "Setting" and settingVal is True:
                if settingOptions[settingOp] == "Pill Size":
                    setting[slot]["Pill Size"] = (setting[slot]["Pill Size"] - 1) % 5
                elif settingOptions[settingOp] == "Interval":
                    setting[slot]["Interval"] = (setting[slot]["Interval"] - 1) % 48
                    if setting[slot]["Interval"] == 0:
                        setting[slot]["Next Alert"] = 0
                    else:
                        nextAlert = (60 * (current_time.hour + setting[slot]["Interval"])) + current_time.minute
                        setting[slot]["Next Alert"] = nextAlert
                elif settingOptions[settingOp] == "Amount":
                    setting[slot]["Amount"] = (setting[slot]["Amount"] - 1) % 10
    
    for slot in setting:
        if setting[slot]["Next Alert"] == 0 and setting[slot]["Interval"] == 0:
            pass
        elif setting[slot]["Next Alert"] == (60 * current_time.hour) + current_time.minute:
            set_angle(Gate_pwm, setting[slot]["Degree"])
            time.sleep(1)
            set_angle(Size_pwm, setting[slot]["Pill Size"] * 15)
            time.sleep(1)
            # Do something to check for pill drop
            set_angle(Gate_pwm, 0)
            time.sleep(1)
            set_angle(Size_pwm, 0)
            time.sleep(1)
            render_alert()
            if setting[slot]["Interval"] != 0:
                nextAlert = (60 * (current_time.hour + setting[slot]["Interval"])) + current_time.minute
                setting[slot]["Next Alert"] = nextAlert
            else: 
                setting[slot]["Next Alert"] = 0

    
    # Update the last state for next loop iteration
    clk_last_state = clk_current

    if screen == "Home":
        render_home()
    elif screen == "Menu":
        render_menu()
    elif screen == "Setting":
        render_setting()
    elif screen == "Status":
        render_status()

    time.sleep(1)

def render_home():
    return False

def render_menu():
    return False

def render_setting():
    return False

def render_status():
    return False

def render_nopill():
    return False

def render_alert():
    return False

def angle_to_duty_cycle(angle):
    """Convert angle in degrees (0-180) to duty cycle"""
    if angle < 0:
        angle = 0
    elif angle > 180:
        angle = 180
    
    # Linear conversion from angle to duty cycle
    duty_cycle = MIN_DUTY + (angle / 180) * (MAX_DUTY - MIN_DUTY)
    return duty_cycle

def set_angle(pwm, angle):
    """Set servo to a specific angle (0-180)"""
    duty_cycle = angle_to_duty_cycle(angle)
    pwm.ChangeDutyCycle(duty_cycle)
    # Small delay to allow the servo to reach position
    time.sleep(0.5)

def cleanup():
    """
    Clean up function for graceful exit
    """
    print("\nCleaning up...")
    GPIO.cleanup()
    print("Done. Goodbye!")
    sys.exit(0)

# Handle Ctrl+C and other termination signals gracefully
def signal_handler(sig, frame):
    cleanup()

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

if __name__ == "__main__":
    try:
        # Run the setup function once
        setup()
        
        # Run the loop function continuously
        print("Starting main loop. Press CTRL+C to exit.")
        while True:
            loop()
            
    except Exception as e:
        print(f"Error: {e}")
        cleanup()