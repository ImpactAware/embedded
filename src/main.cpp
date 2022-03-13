/* Dependencies */
#include <Arduino.h>
#include <painlessMesh.h>

/* Definitions */
#define MESSAGE_DELAY 1
#define SSID "BombSensors"
#define PASS "1amp0ilr0peb0mb5"
#define PORT 5555

/* Function Prototypes */
void broadcastVIBR();
void waitingBlink();
void readyBlink();
IRAM_ATTR void incrementCounter();
void newMessage(uint32_t from, String &msg);
void newConnection(uint32_t nodeId);
void droppedConnection(uint32_t nodeId);

/* Global Variables */
int triggers = 0;

/* Constructors */
painlessMesh sensorMesh;
Scheduler userScheduler;
Task reportTask(TASK_SECOND * MESSAGE_DELAY, TASK_FOREVER, &broadcastVIBR);
Task waitingBlinkTask(TASK_SECOND, TASK_FOREVER, &waitingBlink, &userScheduler, true, NULL, &readyBlink);

/* 
    ---------
    Main
    ---------
*/

void setup() {
    Serial.begin(115200);
    pinMode(SENSOR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // Increment counter upon sensor trigger
    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), incrementCounter, RISING);

    // Create mesh network for boards
    // Set debug before init() so that you can see startup messages
    sensorMesh.setDebugMsgTypes( ERROR | STARTUP );
    sensorMesh.init(SSID, PASS, &userScheduler, PORT);

    sensorMesh.onReceive(&newMessage);
    sensorMesh.onNewConnection(&newConnection);
    sensorMesh.onDroppedConnection(&droppedConnection);

    // Add reporting task
    userScheduler.addTask(reportTask);
    reportTask.enable();
}

void loop() {
    sensorMesh.update();
}

/* 
    ---------
    Functions
    ---------
*/

IRAM_ATTR void incrementCounter() {
    triggers += 1;
}

void broadcastVIBR() {
    if (triggers > 0) {
        String msg = "VIBR ";
        msg += triggers;
        sensorMesh.sendBroadcast(msg, true);
    }
    triggers = 0;
}

// Waiting for connection to network
void waitingBlink() {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// Connection established
void readyBlink() {
    for (char i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
}

// This function is called when a new message is recieved
void newMessage(uint32_t from, String &msg) {
    if (msg.startsWith("VIBR")) {
        Serial.printf("%s %u\n", msg.c_str(), from);
    } else if (msg.startsWith("DROP")) {
        Serial.printf("%s %u\n", msg.c_str(), from);
    }
}

// This function is called when a new node connects to the mesh
void newConnection(uint32_t nodeId) {
    Serial.printf("CONN %u\n", nodeId);

    // Stop LED blinking
    if (waitingBlinkTask.isEnabled())
        waitingBlinkTask.disable();
    
}

// This function is called when a node disconnects from the mesh
void droppedConnection(uint32_t nodeId) {
    String msg = "DROP ";
    msg += nodeId;
    sensorMesh.sendBroadcast(msg, true);

    // Start LED blinking again if the last node disconnects
    if (sensorMesh.getNodeList().size() == 0)
        waitingBlinkTask.enable();
    
}