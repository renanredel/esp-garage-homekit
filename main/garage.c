#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "wifi.h"
#include <driver/adc.h>

#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN 0
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED 1
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING 2
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING 3
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_STOPPED 4
#define HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_UNKNOWN 255

#define HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_OPEN 0
#define HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED 1
#define HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_UNKNOWN 255


void on_wifi_ready();

homekit_value_t gdo_target_state_get();
void gdo_target_state_set(homekit_value_t new_value);
homekit_value_t gdo_current_state_get();
homekit_value_t gdo_obstruction_get();
void identify(homekit_value_t _value);

const char *state_description(uint8_t state) {
        const char* description = "unknown";
        switch (state) {
        case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN: description = "open"; break;
        case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING: description = "opening"; break;
        case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED: description = "closed"; break;
        case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING: description = "closing"; break;
        case HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_STOPPED: description = "stopped"; break; //NOT USED
        default:;
        }
        return description;
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
        switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
                printf("STA start\n");
                esp_wifi_connect();
                break;
        case SYSTEM_EVENT_STA_GOT_IP:
                printf("WiFI ready\n");
                on_wifi_ready();
                break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
                printf("STA disconnected\n");
                esp_wifi_connect();
                break;
        default:
                break;
        }
        return ESP_OK;
}

static void wifi_init() {
        tcpip_adapter_init();
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

        wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

        wifi_config_t wifi_config = {
                .sta = {
                        .ssid = WIFI_SSID,
                        .password = WIFI_PASSWORD,
                },
        };

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        gdo_current_state_get();
}

const int control_gpio = 25;
const int led_gpio = 2;

void control_write() {
        gpio_set_level(control_gpio, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(control_gpio, 0);
}

void control_init() {
        gpio_set_direction(control_gpio, GPIO_MODE_OUTPUT);
}

void led_write(bool on) {
        gpio_set_level(led_gpio, on ? 1 : 0);
}

void led_init() {
        gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT);
        led_write(false);
}



homekit_accessory_t *accessories[] = {
        HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_garage, .services=(homekit_service_t*[]){
                HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
                        HOMEKIT_CHARACTERISTIC(NAME, "Portão Garagem"),
                        HOMEKIT_CHARACTERISTIC(MANUFACTURER, "RENAN"),
                        HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "237A2BAB119E"),
                        HOMEKIT_CHARACTERISTIC(MODEL, "ESP"),
                        HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.5"),
                        HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
                        NULL
                }),
                HOMEKIT_SERVICE(GARAGE_DOOR_OPENER, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                        HOMEKIT_CHARACTERISTIC(NAME, "Tor"),
                        HOMEKIT_CHARACTERISTIC(
                                CURRENT_DOOR_STATE, HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED,
                                .getter=gdo_current_state_get,
                                .setter=NULL
                                ),
                        HOMEKIT_CHARACTERISTIC(
                                TARGET_DOOR_STATE, HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED,
                                .getter=gdo_target_state_get,
                                .setter=gdo_target_state_set
                                ),
                        HOMEKIT_CHARACTERISTIC(
                                OBSTRUCTION_DETECTED, HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED,
                                .getter=gdo_obstruction_get,
                                .setter=NULL
                                ),
                        NULL
                }),
                NULL
        }),
        NULL
};

uint8_t current_door_state = HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_UNKNOWN;

homekit_server_config_t config = {
        .accessories = accessories,
        .password = "111-11-111"
};

void gdo_current_state_notify_homekit() {

        homekit_value_t new_value = HOMEKIT_UINT8(current_door_state);
        printf("Door State: '%s'\n", state_description(current_door_state));

        homekit_accessory_t *accessory = accessories[0];
        homekit_service_t *service = accessory->services[1];
        homekit_characteristic_t *c = service->characteristics[1];

        assert(c);

        printf("State Changed: '%s'\n", c->description);
        homekit_characteristic_notify(c, new_value);
}

void gdo_target_state_notify_homekit() {

        homekit_value_t new_value = gdo_target_state_get();
        printf("Door State: '%s'\n", state_description(new_value.int_value));

        homekit_accessory_t *accessory = accessories[0];
        homekit_service_t *service = accessory->services[1];
        homekit_characteristic_t *c = service->characteristics[2];

        assert(c);

        printf("State Changed: '%s'\n", c->description);
        homekit_characteristic_notify(c, new_value);
}

void current_state_set(uint8_t new_state) {
        if (current_door_state != new_state) {
                current_door_state = new_state;
                gdo_target_state_notify_homekit();
                gdo_current_state_notify_homekit();
        }
}

void current_door_state_update_from_sensor(int repeats){
        //	CONFIG ADC1 WIDTH
        adc1_config_width(ADC_WIDTH_12Bit);
        int open = 0;
        int close = 0;
        for (int i = 0; i<repeats; i++) {
                //	READ THE INTERNAL SENSOR FROM ESP32
                int val = hall_sensor_read();
                if (val > 20) {
                        open++;
                }else{
                        close++;
                }
                vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        if (open>close) {
                current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN);
        }else{
                current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED);
        }
}

homekit_value_t gdo_current_state_get() {
        if (current_door_state == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_UNKNOWN) {
                //	IF IN FIRST RUN, RUN THE COMPLETY CHECK
                current_door_state_update_from_sensor(20);
                printf("Updating from sensor");
        }else{
                current_door_state_update_from_sensor(5);
        }

        printf("Current Door State: '%s'.\n", state_description(current_door_state));

        return HOMEKIT_UINT8(current_door_state);
}

homekit_value_t gdo_target_state_get() {
        uint8_t result = gdo_current_state_get().int_value;
        if (result == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING) {
                result = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_OPEN;
        } else if (result == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING) {
                result = HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE_CLOSED;
        }

        printf("Current Door State: '%s'.\n", state_description(result));

        return HOMEKIT_UINT8(result);
}

void gdo_target_state_set(homekit_value_t new_value) {


        if (new_value.format != homekit_format_uint8) {
                printf("Invalid value format: %d\n", new_value.format);
                return;
        }

        if (current_door_state != HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPEN &&
            current_door_state != HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED) {
                printf("gdo_target_state_set() ignored: current state not open or closed (%s).\n", state_description(current_door_state));
                return;
        }

        if (current_door_state == new_value.int_value) {
                printf("gdo_target_state_set() ignored: new target state == current state (%s)\n", state_description(current_door_state));
                return;
        }

        control_write();
        vTaskDelay(400 / portTICK_PERIOD_MS);
        //	TURN ON THE CONTROLLER

        if (current_door_state == HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSED) {
                current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_OPENING);
        } else {
                current_state_set(HOMEKIT_CHARACTERISTIC_CURRENT_DOOR_STATE_CLOSING);
        }
        //	WAIT THE DOOR CLOSE/OPEN
        vTaskDelay(2500 / portTICK_PERIOD_MS);
        current_door_state_update_from_sensor(20);
}
void identify_task(void *_args) {

        vTaskDelete(NULL);
}

void identify(homekit_value_t _value) {
        printf("GDO identify\n");
        xTaskCreate(identify_task, "GDO identify", 128, NULL, 2, NULL);
}


homekit_value_t gdo_obstruction_get() {
        return HOMEKIT_BOOL(false);
}

void on_wifi_ready() {
        homekit_server_init(&config);
}

void app_main(void) {
        // Initialize NVS
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
                ESP_ERROR_CHECK(nvs_flash_erase());
                ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK( ret );

        wifi_init();
        control_init();
        led_init()
}
