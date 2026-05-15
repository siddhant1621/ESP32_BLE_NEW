#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"          
#include "esp_adc/adc_oneshot.h"  
#include "esp_log.h"
#include "nvs_flash.h"

//NimBLE stack headers
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gatt.h" 
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

//Log tag
static const char *TAG = "BLE_INIT";

//UUIDs for custom BLE service and charactersitic
#define ADC_SERVICE_UUID         0x181A
#define ADC_CHARACTERISTIC_UUID  0x2A58

//Buffer to hold the latest 10-sample ADC data in JSON string format
static char adc_data_buffer[100] = "[]";  

// BLE connection and characteristic handles
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t adc_char_handle = 0;

/**
 * GATT characteristic access callback.
 * Called when the client reads the ADC characteristic.
 */
static int adc_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc = os_mbuf_append(ctxt->om, adc_data_buffer, strlen(adc_data_buffer));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

/**
 * Define the custom GATT service and its characteristic
 */
static const struct ble_gatt_svc_def gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(ADC_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(ADC_CHARACTERISTIC_UUID),
                .access_cb = adc_chr_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &adc_char_handle, //store handle for notification
            },
            { 0 } 
        },
    },
    { 0 }
};

/**
 * GAP event handler to manage connection, disconnection, and advertising events
 */
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "Connection established");
            conn_handle = event->connect.conn_handle;
        } else {
            ESP_LOGI(TAG, "Connection failed; status=%d", event->connect.status);
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Disconnected; reason = %d", event->disconnect.reason);
        // Restart advertising after disconnection
        ble_gap_adv_start(0, NULL, BLE_HS_FOREVER, NULL, ble_gap_event, NULL);
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "Advertising complete");
        break;

    default:
        break;
    }
    return 0;
}

/**
 * Start advertising the device with a custom name
 */
void start_advertising(void)
{
    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    // Set the device name visible to BLE clients
    ble_svc_gap_device_name_set("ESP32_ADC_BLE");

    // Set Advertising fields
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    fields.name = (uint8_t *)"ESP32_ADC_BLE";
    fields.name_len = strlen("ESP32_ADC_BLE");
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);
    ble_gap_adv_start(0, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
    ESP_LOGI(TAG, "Started BLE Advertising as ESP32_ADC_BLE");
}

/**
 * Called when the BLE host stack is synchronized and ready
 */
void ble_app_on_sync(void)
{
    start_advertising();
}

/**
 * NimBLE host task entry point
 */
void host_task(void *param)
{
    nimble_port_run(); // Start BLE Host stack task
    nimble_port_freertos_deinit(); // Should never return
}

/**
 * Periodic task to sample ADC values and send them over BLE every second
 */
void adc_notify_task(void *arg)
{
    //1: Initilaze ADC unit handle(ADC1)
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));
    
    //2: Configure ADC channel (ADC1_CH0 = GPIO4)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, 
        .atten = ADC_ATTEN_DB_12,  // allows measuremnt upto ~3.3V
    };
    adc_channel_t channel = ADC_CHANNEL_4;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channel, &config)); // bind config

    //3: Main sampling + notification loop
    while (1) {
        int readings[10];
        char temp_buf[100];

        //Read 10 ADC samples and temporary buffer for outgoig message
        for (int i = 0; i < 10; i++) {
            adc_oneshot_read(adc_handle, channel, &readings[i]);
            vTaskDelay(pdMS_TO_TICKS(100));  // reads one adc value every 100ms, total 10 values in 1 second
        }

        //Format or convert the readings into a JSON array string
        int len = snprintf(temp_buf, sizeof(temp_buf),
                           "[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
                           readings[0], readings[1], readings[2], readings[3], readings[4],
                           readings[5], readings[6], readings[7], readings[8], readings[9]);

        //Copy to global buffer if valid                   
        if (len > 0 && len < sizeof(adc_data_buffer)) {
            strncpy(adc_data_buffer, temp_buf, sizeof(adc_data_buffer));
            adc_data_buffer[len] = '\0'; // null-terminate

            ESP_LOGI(TAG, "Sending ADC bundle: %s", adc_data_buffer);

            // Send BLE notify if client is connected
            if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                struct os_mbuf *om = ble_hs_mbuf_from_flat(adc_data_buffer, strlen(adc_data_buffer));
                if (om) {
                    ble_gatts_notify_custom(conn_handle, adc_char_handle, om);
                }
            }

        }
    }
}

/**
 * Main application entry point
 */
void app_main(void)
{
    esp_err_t ret;

    //1: Initialize NVS flash (required for BLE)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(ret);

    //2: Initialize NimBLE Host Stack
    nimble_port_init();

    //3: Register standard GATT service for ADC service
    ble_svc_gap_init();       // device name and apperance
    ble_svc_gatt_init();      // Generic Attribute Profile service

    //4: Register custom GATT service for ADC streaming
    ble_gatts_count_cfg(gatt_services); 
    ble_gatts_add_svcs(gatt_services);  

    //5: Register callback for BLE sync event
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    //6: Launch BLE stack task Start NimBLE thread
    nimble_port_freertos_init(host_task);

    //7: Launch ADC sampling task
    xTaskCreate(adc_notify_task, "adc_notify_task", 4096, NULL, 5, NULL);

}
