void setup() {
  Serial.begin(9600);

  setup_hardware();
  setup_nextion();
}

void loop() {
  loop_hardware();
  loop_nextion();
}
