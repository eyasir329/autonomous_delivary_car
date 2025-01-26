#include <ESP32Servo.h>
#include <WiFi.h>

#include "Arduino.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "fb_gfx.h"
#include "img_converters.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

const char *ssid = "SUB1";
const char *password = "newtown123";

#define PART_BOUNDARY "123456789000000000000987654321"
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#else
#error "Camera model not selected"
#endif

// Pin Definitions
#define MOTOR_FORWARD_PIN 15
#define MOTOR_BACKWARD_PIN 12
#define MOTOR_SPEED_PIN 4

#define PAN_SERVO 13
#define TILT_SERVO 2
#define STEERING_SERVO 14

// #define LED_PIN 4
Servo panServo, tiltServo, steeringServo;
Servo dummyServo1;
Servo dummyServo2;
Servo dummyServo3;

int panPos = 70, tiltPos = 70, steeringPos = 90, dammyPos = 90;
int forwardSpeed = 30, backwardSpeed = 30;

static const char *_STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>Robot Car</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>
      body {
        font-family: Arial, sans-serif;
        text-align: center;
        margin: 0;
        padding: 10px;
      }

      th {
        font-size: medium;
        background-color: aquamarine;
        padding: 8px;
      }

      td {
        padding: 5px;
      }

      .button:hover {
        background-color: #3b587a;
      }

      .slider-label {
        font-size: small;
        margin-bottom: 5px;
        display: block;
      }

      .slider {
        width: 60%;
        margin-top: 5px;
      }

      img {
        max-width: 100%;
        height: auto;
      }

      .button {
        background-color: #2f4468;
        border: none;
        color: white;
        padding: 14px 24px;
        font-size: 18px;
        cursor: pointer;
        border-radius: 4px;
        user-select: none;
        margin: 2px 5px;
        display: inline-block;
        text-align: center;
      }

      .container {
        display: flex;
        justify-content: space-between;
      }

      .img-container {
        flex: 1;
        text-align: right;
      }

      .controls-container {
        flex: 1;
        text-align: left;
      }

      .section {
        text-align: center;
        display: flex;
        flex-direction: column;
        align-items: center;
      }

      table {
        margin: auto;
        text-align: center;
      }
    </style>
  </head>

  <body>
    <h2>Manual Mode</h2>

    <div class="main">
      <div class="container">
        <!-- Image on the left -->
        <div class="img-container">
          <img src="" id="photo" alt="Live Video Stream" />
        </div>

        <!-- Controls on the right -->
        <div class="controls-container" style="margin-top: 20px">
          <div class="section">
            <table>
              <tr>
                <th>Car Control</th>
              </tr>
              <tr>
                <td>
                  <button
                    class="button"
                    onmousedown="speedCar('forward');"
                    ontouchstart="speedCar('forward');"
                  >
                    Forward
                  </button>
                </td>
              </tr>
              <tr>
                <td>
                  <button
                    class="button"
                    onmousedown="moveCar('left');"
                    ontouchstart="moveCar('left');"
                  >
                    Left
                  </button>
                  <button
                    class="button"
                    onmousedown="moveCar('center');"
                    ontouchstart="moveCar('center');"
                  >
                    Center
                  </button>
                  <button
                    class="button"
                    onmousedown="moveCar('right');"
                    ontouchstart="moveCar('right');"
                  >
                    Right
                  </button>
                </td>
              </tr>
              <tr>
                <td>
                  <button
                    class="button"
                    onmousedown="speedCar('backward');"
                    ontouchstart="speedCar('backward');"
                  >
                    Backward
                  </button>
                </td>
              </tr>
              <tr>
                <td>
                  <button
                    class="button"
                    onmousedown="speedCar('stop');"
                    ontouchstart="speedCar('stop');"
                  >
                    Stop
                  </button>
                </td>
              </tr>
            </table>
          </div>

          <!-- Car control section -->
          <div class="section" style="margin-top: 10px">
            <table>
              <tr>
                <th>Pan Tilt Control</th>
              </tr>
              <tr>
                <td>
                  <button
                    class="button"
                    onmousedown="moveCamera('up');"
                    ontouchstart="moveCamera('up');"
                  >
                    Up
                  </button>
                </td>
              </tr>
              <tr>
                <td>
                  <button
                    class="button"
                    onmousedown="moveCamera('left');"
                    ontouchstart="moveCamera('left');"
                  >
                    Left
                  </button>
                  <button
                    class="button"
                    onmousedown="moveCamera('center');"
                    ontouchstart="moveCamera('center');"
                  >
                    Center
                  </button>
                  <button
                    class="button"
                    onmousedown="moveCamera('right');"
                    ontouchstart="moveCamera('right');"
                  >
                    Right
                  </button>
                </td>
              </tr>
              <tr>
                <td>
                  <button
                    class="button"
                    onmousedown="moveCamera('down');"
                    ontouchstart="moveCamera('down');"
                  >
                    Down
                  </button>
                </td>
              </tr>
            </table>
          </div>
        </div>
      </div>
    </div>

    <script>
      // Function to handle actions with GET requests
      function toggleCheckbox(action) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/action?go=" + action, true);
        xhr.send();
      }

      // Function to handle car movements ( left, right)
      function moveCar(direction) {
        toggleCheckbox(`moveCar_${direction}`);
      }

      // Function to handle camera pan and tilt (up, down, left, right)
      function moveCamera(direction) {
        toggleCheckbox(`moveCamera_${direction}`);
      }

      // Function to set speed for the car's movement
      function speedCar(direction) {
        toggleCheckbox(`speed_${direction}`);
      }

      // Set up the live stream URL on page load
      window.onload = function () {
        document.getElementById("photo").src =
          window.location.href.slice(0, -1) + ":81/stream";
      };
    </script>
  </body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req);
static esp_err_t stream_handler(httpd_req_t *req);
static esp_err_t cmd_handler(httpd_req_t *req);

// Function Prototypes
void startCameraServer();
void moveMotorForward(int speed);
void moveMotorBackward(int speed);
void stopMotor();
void initServos();
void initMotorPins();
void initWiFi();
void initCamera();

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // disable brownout detector
  Serial.begin(115200);
  initWiFi();
  initServos();
  initMotorPins();
  initCamera();
  startCameraServer();
}

void loop() {}

void initServos() {
  // Initialize dummy servos to avoid PWM conflicts
  dummyServo1.attach(MOTOR_FORWARD_PIN);  // Pin 12 or 13
  dummyServo2.attach(MOTOR_BACKWARD_PIN);
  dummyServo3.attach(MOTOR_SPEED_PIN);

  panServo.setPeriodHertz(50);
  tiltServo.setPeriodHertz(50);
  steeringServo.setPeriodHertz(50);

  panServo.attach(PAN_SERVO, 1000, 2000);
  tiltServo.attach(TILT_SERVO, 1000, 2000);
  steeringServo.attach(STEERING_SERVO, 1000, 2000);

  panServo.write(panPos);
  tiltServo.write(tiltPos);
  steeringServo.write(steeringPos);
}

void initMotorPins() {
  pinMode(MOTOR_FORWARD_PIN, OUTPUT);
  pinMode(MOTOR_BACKWARD_PIN, OUTPUT);
  pinMode(MOTOR_SPEED_PIN, OUTPUT);

  // Ensure motor is stopped initially
  stopMotor();

  // pinMode(LED_PIN, OUTPUT);
  // digitalWrite(LED_PIN, LOW);
}

void initWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());
}

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    // config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    // config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if (fb->width > 400) {
        if (fb->format != PIXFORMAT_JPEG) {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted) {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY,
                                  strlen(_STREAM_BOUNDARY));
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) {
      break;
    }
    // Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

// HTTP Handler Function
static esp_err_t cmd_handler(httpd_req_t *req) {
  char *buf;
  size_t buf_len;
  char variable[32] = {0};
  buf_len = httpd_req_get_url_query_len(req) + 1;

  if (buf_len > 1) {
    buf = (char *)malloc(buf_len);
    if (!buf) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }

    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK &&
        httpd_query_key_value(buf, "go", variable, sizeof(variable)) ==
            ESP_OK) {
      Serial.printf("Command received: %s\n", variable);
    } else {
      free(buf);
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "Invalid query parameter");
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No query string provided");
    return ESP_FAIL;
  }

  // Command Processing
  if (strstr(variable, "speed_forward")) {
    forwardSpeed = constrain(forwardSpeed + 10, 0, 255);
    backwardSpeed = 0;
    moveMotorForward(forwardSpeed);
  } else if (strstr(variable, "speed_backward")) {
    backwardSpeed = constrain(backwardSpeed + 10, 0, 255);
    forwardSpeed = 0;
    moveMotorBackward(backwardSpeed);
  } else if (strstr(variable, "speed_stop")) {
    stopMotor();
  } else if (!strcmp(variable, "moveCamera_down")) {
    tiltPos = constrain(tiltPos + 10, 0, 170);
    tiltServo.write(tiltPos);
    Serial.printf("Camera Tilt Down: %d\n", tiltPos);
  } else if (!strcmp(variable, "moveCamera_up")) {
    tiltPos = constrain(tiltPos - 10, 0, 170);
    tiltServo.write(tiltPos);
    Serial.printf("Camera Tilt Up: %d\n", tiltPos);
  } else if (!strcmp(variable, "moveCamera_left")) {
    panPos = constrain(panPos + 10, 0, 170);
    panServo.write(panPos);
    Serial.printf("Camera Pan Left: %d\n", panPos);
  } else if (!strcmp(variable, "moveCamera_right")) {
    panPos = constrain(panPos - 10, 0, 170);
    panServo.write(panPos);
    Serial.printf("Camera Pan Right: %d\n", panPos);
  } else if (!strcmp(variable, "moveCamera_center")) {
    panPos = tiltPos = 90;
    panServo.write(panPos);
    tiltServo.write(tiltPos);
    Serial.println("Camera Centered");
  } else if (!strcmp(variable, "moveCar_left")) {
    steeringPos = constrain(steeringPos + 10, 30, 150);
    steeringServo.write(steeringPos);
    Serial.printf("Car Steering Left: %d\n", steeringPos);
  } else if (!strcmp(variable, "moveCar_right")) {
    steeringPos = constrain(steeringPos - 10, 30, 150);
    steeringServo.write(steeringPos);
    Serial.printf("Car Steering Right: %d\n", steeringPos);
  } else if (!strcmp(variable, "moveCar_center")) {
    steeringPos = 90;
    steeringServo.write(90);
    Serial.println("Car Centered");
  } else {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown command");
    return ESP_FAIL;
  }

  // Success Response
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);  // Respond with no content (OK)
}

// Move Motor Forward
void moveMotorForward(int speed) {
  digitalWrite(MOTOR_FORWARD_PIN, HIGH);
  digitalWrite(MOTOR_BACKWARD_PIN, LOW);
  analogWrite(MOTOR_SPEED_PIN, speed);  // Set PWM speed (0-255)
  Serial.printf("Motor Moving Forward at Speed: %d\n", speed);
}

// Move Motor Backward
void moveMotorBackward(int speed) {
  digitalWrite(MOTOR_FORWARD_PIN, LOW);
  digitalWrite(MOTOR_BACKWARD_PIN, HIGH);
  analogWrite(MOTOR_SPEED_PIN, speed);  // Set PWM speed (0-255)
  Serial.printf("Motor Moving Backward at Speed: %d\n", speed);
}

// Stop Motor
void stopMotor() {
  digitalWrite(MOTOR_FORWARD_PIN, LOW);
  digitalWrite(MOTOR_BACKWARD_PIN, LOW);
  analogWrite(MOTOR_SPEED_PIN, 0);  // Set speed to 0
  Serial.println("Motor Stopped");
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t index_uri = {.uri = "/",
                           .method = HTTP_GET,
                           .handler = index_handler,
                           .user_ctx = NULL};

  httpd_uri_t cmd_uri = {.uri = "/action",
                         .method = HTTP_GET,
                         .handler = cmd_handler,
                         .user_ctx = NULL};
  httpd_uri_t stream_uri = {.uri = "/stream",
                            .method = HTTP_GET,
                            .handler = stream_handler,
                            .user_ctx = NULL};
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
  }
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}