// Below are links I used for testing each component, any code I referenced from them will have a comment added to them
// L298N Motor Driver
// https://howtomechatronics.com/tutorials/arduino/arduino-dc-motor-control-tutorial-l298n-pwm-h-bridge/
// Ultra Sonic Sensor
// https://www.tutorialspoint.com/arduino/arduino_ultrasonic_sensor.htm
// IR Receiver
// https://roboticsbackend.com/arduino-ir-remote-controller-tutorial-setup-and-map-buttons/#:~:text=For%20the%20IR%20receiver%2C%20you%20use%20the%20begin,%7B%20if%20%28IrReceiver.decode%28%29%29%20%7B%20IrReceiver.resume%28%29%3B%20Serial.println%28IrReceiver.decodedIRData.command%29%3B%20%7D%20%7D
// Servo Motor
// https://www.instructables.com/Arduino-Servo-Motors/
#include <IRremote.hpp>
#include <Servo.h>

// IR Remote Control related variables
int RECV_PIN = 9;
unsigned long control = 0;
bool gate = true;

// Motor/Motor Driver related variables
int enA = 2;
int in1 = 4;
int in2 = 5;
int enB = 8;
int in3 = 6;
int in4 = 7;
unsigned long mph = 150;

// Servo control related variables
int pos = 90;
int SERVO_PIN = 13;
Servo myservo;

// Ultrasonic sensor related variables
const int pingPin = 10;
const int echoPin = 12;
long duration = 0;
bool warn = false;

// LED related variables
int led = 3;

typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
} task;

const unsigned short tasksNum = 5;
task tasks[tasksNum];

// Remote input helper function
// I had some knowled of using IR Receiver before but this code in particular that I got from the link made it a lot more appealing when reading the value
// IrReceiver.decodedIRData.command
void remote_input() {
  switch (IrReceiver.decodedIRData.command) {
    // Motor control
    case 70: // forward / up
        control = 1;
      break;
    case 21: // backward / down
        control = 2;
      break;
    case 68: // left / left
        control = 3;
      break;
    case 67: // right / right
        control = 4;
      break;
    case 64: // neutral / ok
        control = 5;
      break;
    // Speed control
    case 22: // increase / 1
        control = 6;
      break;
    case 12: // decrease / 4
        control = 7;
      break;
    // Servo control
    case 13: // rotate_left / 3
        control = 8;
      break;
    case 94: // rotate_right / 6
        control = 9;
      break;
    // Reset variables and states before turning off
    case 82: // reset / 0
        control = 10;
      break;
    default:
        control = 0;
      break;
  }
}

// Ultra Sonic Sensor helper function
// I used the code in the link and made small adjustment to get the code below
void us_ping() {
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  duration = pulseIn(echoPin, HIGH);

  if (duration/148 < 15) {
    warn = true;
  } else {
    warn = false;
  }
}

// Read input from controller
enum CONT { Start, CONT_Input };
int CarController(int state) {
  switch(state) {
    case Start:
      state = CONT_Input;
      break;
    case CONT_Input:
      if (control == 10) {
        state = Start;
      }
      break;
    default:
      break;
  }
  switch(state) {
    case Start:
      control = 0;
      break;
    case CONT_Input:
      if (IrReceiver.decode() && gate) {
        remote_input();
        IrReceiver.resume();
        gate = false;
      }
      if (!IrReceiver.decode()) {
        gate = true;
      }
      break;
    default:
      break;
  }
  return state;
}

// Controls direction of the motors to run
// There's only one way to set the motor direction but I had to figure out how to get a specific direction myself when testing
enum ENG { ENG_INIT, parked, forward, reverse, rotate_left, rotate_right, motor_reset };
int Motor(int state) {
  switch(state) {
    case ENG_INIT:
      state = parked;
      break;
    case parked:
      if (!warn && (control == 1)) {
        state = forward;
      } else if (control == 2) {
        state = reverse;
      } else if (control == 3) {
        state = rotate_left;
      } else if (control == 4) {
        state = rotate_right;
      } else if (control == 10) {
        state = motor_reset;
      }
      break;
    case forward:
      if (warn || (control == 5)) {
        state = parked;
      } else if (control == 2) {
        state = reverse;
      } else if (control == 3) {
        state = rotate_left;
      } else if (control == 4) {
        state = rotate_right;
      } else if (control == 10) {
        state = motor_reset;
      }
      break;
    case reverse:
      if (control == 5) {
        state = parked;
      } else if (!warn && (control == 1)) {
        state = forward;
      } else if (control == 3) {
        state = rotate_left;
      } else if (control == 4) {
        state = rotate_right;
      } else if (control == 10) {
        state = motor_reset;
      }
      break;
    case rotate_left:
      if (control == 5) {
        state = parked;
      } else if (!warn && (control == 1)) {
        state = forward;
      } else if (control == 2) {
        state = reverse;
      } else if (control == 4) {
        state = rotate_right;
      } else if (control == 10) {
        state = motor_reset;
      }
      break;
    case rotate_right:
      if (control == 5) {
        state = parked;
      } else if (!warn && (control == 1)) {
        state = forward;
      } else if (control == 2) {
        state = reverse;
      } else if (control == 3) {
        state = rotate_left;
      } else if (control == 10) {
        state = motor_reset;
      }
      break;
    case motor_reset:
      state = ENG_INIT;
      break;
    default:
      state = ENG_INIT;
      break;
  }
  switch(state) {
    case parked:
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      break;
    case forward:
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      break;
    case reverse:
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
      break;
    case rotate_left:
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
      break;
    case rotate_right:
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      break;
    case motor_reset:
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      break;
    default:
      break;
  }
  return state;
}

// Controls speed that the motors run
// There's only one way to set the motor speed
enum SPEED { SPD_INIT, stabled, increase, decrease, reset };
int Speed(int state) {
  switch(state) {
    case SPD_INIT:
      state = stabled;
      break;
    case stabled:
      if (control == 6) {
        state = increase;
      } else if (control == 7) {
        state = decrease;
      } else if ((control == 5) || (control == 10)) {
        state = reset;
      }
      break;
    case increase:
      if ((control == 5) || (control == 10)) {
        state = reset;
      } else {
        state = stabled;
      }
      break;
    case decrease:
      if (control == 7) {
        state = decrease;
      } else if ((control == 5) || (control == 10)) {
        state = reset;
      } else {
        state = stabled;
      }
      break;
    case reset:
      state = SPD_INIT;
      break;
    default:
      state = SPD_INIT;
      break;
  }
  switch(state) {
    case stabled:
      if (control == 1) {
        mph = 200;
        analogWrite(enA, mph);
        analogWrite(enB, mph);
      } else if ((control == 2) || (control == 3) || (control == 4)) {
        mph = 150;
        analogWrite(enA, mph);
        analogWrite(enB, mph);
      }
      break;
    case increase:
      if (mph < 250) {
        mph += 50;
        analogWrite(enA, mph);
        analogWrite(enB, mph);
      }
      break;
    case decrease:
      if (mph > 150) {
        mph -= 50;
        analogWrite(enA, mph);
        analogWrite(enB, mph);
      }
      break;
    case reset:
      mph = 150;
      analogWrite(enA, mph);
      analogWrite(enB, mph);
      break;
    default:
      break;
  }
  return state;
}

// Enabled user warning
enum SIGNALS { SIG_INIT, enabled };
int Warning(int state) {
  switch(state) {
    case SIG_INIT:
      state = enabled;
      break;
    case enabled:
      if (control == 10) {
        state = SIG_INIT;
      }
      break;
    default:
      state = SIG_INIT;
      break;
  }
  switch(state) {
    case enabled:
      us_ping();
      if (warn) {
        digitalWrite(led, HIGH);
        if (control == 1) {
          control = 5;
          digitalWrite(in1, LOW);
          digitalWrite(in2, LOW);
          digitalWrite(in3, LOW);
          digitalWrite(in4, LOW);
        }
      } else if (!warn) {
        digitalWrite(led, LOW);
      }
      break;
    default:
      break;
  }
  return state;
}

// Rotates sensor left and right
enum SERVO { S_INIT, S_Wait, left, right, pos_reset };
int USS_Move(int state) {
  switch(state) {
    case S_INIT:
      state = S_Wait;
      break;
    case S_Wait:
      if (control == 8) {
        state = left;
      } else if (control == 9) {
        state = right;
      } else if (control == 10) {
        state = pos_reset;
      }
      break;
    case left:
      if (control == 10) {
        state = pos_reset;
      } else {
        state = S_Wait;
      }
      break;
    case right:
      if (control == 10) {
        state = pos_reset;
      } else {
        state = S_Wait;
      }
      break;
    case pos_reset:
      state = S_INIT;
      break;
    default:
      state = S_INIT;
      break;
  }
  switch(state) {
    case left:
      if (pos < 180) {
        pos += 30;
        myservo.write(pos);
      }
      break;
    case right:
      if (pos > 0) {
        pos -= 30;
        myservo.write(pos);
      }
      break;
    case pos_reset:
      break;
      pos = 90;
      myservo.write(pos);
    default:
      break;
  }
  return state;
}

void setup() {
  IrReceiver.begin(RECV_PIN);
  
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(enB, OUTPUT);

  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);

  myservo.attach(SERVO_PIN);
  
  pinMode(pingPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(pingPin, LOW);

  pinMode(led, OUTPUT);

  unsigned char i = 0;
  tasks[i].state = Start;
  tasks[i].period = 500;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &CarController;
  i++;
  tasks[i].state = ENG_INIT;
  tasks[i].period = 500;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &Motor;
  i++;
  tasks[i].state = SPD_INIT;
  tasks[i].period = 500;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &Speed;
  i++;
  tasks[i].state = SIG_INIT;
  tasks[i].period = 500;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &Warning;
  i++;
  tasks[i].state = S_INIT;
  tasks[i].period = 500;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &USS_Move;
  
  Serial.begin(9600);
}

void loop() {
  unsigned char i;
  for (i = 0; i < tasksNum; i++) {
    if ((millis() - tasks[i].elapsedTime) >= tasks[i].period) {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = millis();
    }
  }
}
