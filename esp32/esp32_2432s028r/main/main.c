#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "lvgl.h"
#include "lvgl_helpers.h"

static const char *TAG = "ESP32_2432S028R";

// Wi-Fi credentials
#define WIFI_SSID "Cafe_Net"
#define WIFI_PASS "password"

// MQTT configuration
#define MQTT_BROKER "mqtt://192.168.4.1"
#define MQTT_USER "your_username"
#define MQTT_PASSWORD "your_password"

typedef struct {
    char ssid[32];
    int base_rssi;
    int current_rssi;
} wifi_config_t;

wifi_config_t wifi_configs[10];
int wifi_count = 0;
esp_mqtt_client_handle_t mqtt_client;

// LVGL UI elements
lv_obj_t *wifi_labels[10];

void wifi_init_sta() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void wifi_init_softap() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    for (int i = 0; i < wifi_count; i++) {
        wifi_config_t ap_config = {
            .ap = {
                .ssid = "",
                .ssid_len = strlen(wifi_configs[i].ssid),
                .password = "",
                .max_connection = 4,
                .authmode = WIFI_AUTH_OPEN,
            },
        };
        strncpy((char *)ap_config.ap.ssid, wifi_configs[i].ssid, sizeof(ap_config.ap.ssid));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_LOGI(TAG, "Broadcasting SSID: %s", wifi_configs[i].ssid);
    }
}

void lvgl_init() {
    lv_init();
    lvgl_driver_init();

    for (int i = 0; i < 10; i++) {
        wifi_labels[i] = lv_label_create(lv_scr_act());
        lv_label_set_text(wifi_labels[i], "Wi-Fi: Inactive");
        lv_obj_align(wifi_labels[i], LV_ALIGN_TOP_LEFT, 10, 10 + i * 20);
    }
}

void update_rssi_display() {
    for (int i = 0; i < wifi_count; i++) {
        char rssi_str[50];
        snprintf(rssi_str, sizeof(rssi_str), "%s: %d dBm", wifi_configs[i].ssid, wifi_configs[i].current_rssi);
        lv_label_set_text(wifi_labels[i], rssi_str);
    }
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            esp_mqtt_client_subscribe(mqtt_client, "esp32/all/config", 0);
            esp_mqtt_client_subscribe(mqtt_client, "esp32/all/start", 0);
            esp_mqtt_client_subscribe(mqtt_client, "esp32/all/stop", 0);
            break;
        case MQTT_EVENT_DATA: {
            if (strncmp(event->topic, "esp32/all/config", event->topic_len) == 0) {
                cJSON *root = cJSON_Parse(event->data);
                if (root) {
                    cJSON *wifi_array = cJSON_GetObjectItem(root, "wifi");
                    if (wifi_array) {
                        wifi_count = cJSON_GetArraySize(wifi_array);
                        for (int i = 0; i < wifi_count; i++) {
                            cJSON *item = cJSON_GetArrayItem(wifi_array, i);
                            strncpy(wifi_configs[i].ssid, cJSON_GetObjectItem(item, "ssid")->valuestring, 32);
                            wifi_configs[i].base_rssi = cJSON_GetObjectItem(item, "rssi")->valueint;
                            wifi_configs[i].current_rssi = wifi_configs[i].base_rssi;
                        }
                        update_rssi_display();
                    }
                    cJSON_Delete(root);
                }
            } else if (strncmp(event->topic, "esp32/all/start", event->topic_len) == 0) {
                wifi_init_softap();
                update_rssi_display();
            } else if (strncmp(event->topic, "esp32/all/stop", event->topic_len) == 0) {
                esp_wifi_stop();
            }
            break;
        }
    }
}

void mqtt_init() {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASSWORD,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
    lvgl_init();
    mqtt_init();

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
