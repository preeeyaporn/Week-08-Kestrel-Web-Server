#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#define POT_ADC_CHANNEL ADC_CHANNEL_6 

void app_main(void)
{
    printf("\n[SYSTEM] LDR Sensor Stream Starting...\n");

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, POT_ADC_CHANNEL, &config));

    int raw_val = 0;
    float filtered_val = 0.0f;
    const float alpha = 0.25f; 

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, POT_ADC_CHANNEL, &raw_val));
        
        filtered_val = (alpha * raw_val) + ((1.0f - alpha) * filtered_val);
        printf("%d\n", (int)filtered_val);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}