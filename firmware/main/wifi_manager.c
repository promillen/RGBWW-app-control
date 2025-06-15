#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

static const char *TAG = "WIFI_MANAGER";

#define WIFI_SSID      "RGBW_Setup"
#define WIFI_PASS      ""
#define WIFI_CHANNEL   1
#define MAX_STA_CONN   1

static httpd_handle_t server = NULL;

// QR Code HTML Page
static const char* qr_html = 
"<!DOCTYPE html>"
"<html><head><title>RGBW LED Setup</title>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<style>body{font-family:Arial;text-align:center;padding:20px;}"
".qr{margin:20px auto;}"
"h1{color:#333;}"
"p{font-size:18px;margin:20px;}"
"</style></head>"
"<body>"
"<h1>RGBW LED Controller</h1>"
"<p>Scan this QR code with your phone's camera:</p>"
"<div class='qr'>"
"<div id='qrcode'></div>"
"</div>"
"<p>Or manually open: <br><strong>rgbwled://connect?name=RGBW_LED&mac=%s</strong></p>"
"<script src='https://cdnjs.cloudflare.com/ajax/libs/qrious/4.0.2/qrious.min.js'></script>"
"<script>"
"var qr = new QRious({"
"element: document.getElementById('qrcode'),"
"value: 'rgbwled://connect?name=RGBW_LED&mac=%s',"
"size: 256"
"});"
"</script>"
"</body></html>";

static esp_err_t qr_handler(httpd_req_t *req)
{
   uint8_t mac[6];
   esp_wifi_get_mac(WIFI_IF_AP, mac);
   
   char mac_str[18];
   sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
   
   char response[2048];
   snprintf(response, sizeof(response), qr_html, mac_str, mac_str);
   
   httpd_resp_set_type(req, "text/html");
   httpd_resp_send(req, response, strlen(response));
   
   return ESP_OK;
}

static httpd_uri_t qr_uri = {
   .uri       = "/",
   .method    = HTTP_GET,
   .handler   = qr_handler,
   .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
   httpd_handle_t server = NULL;
   httpd_config_t config = HTTPD_DEFAULT_CONFIG();
   config.lru_purge_enable = true;

   if (httpd_start(&server, &config) == ESP_OK) {
       httpd_register_uri_handler(server, &qr_uri);
       return server;
   }

   ESP_LOGI(TAG, "Error starting server!");
   return NULL;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data)
{
   if (event_id == WIFI_EVENT_AP_STACONNECTED) {
       wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
       ESP_LOGI(TAG, "Station "MACSTR" join, AID=%d",
                MAC2STR(event->mac), event->aid);
   } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
       wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
       ESP_LOGI(TAG, "Station "MACSTR" leave, AID=%d",
                MAC2STR(event->mac), event->aid);
   }
}

void wifi_init_softap(void)
{
   ESP_ERROR_CHECK(esp_netif_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default());
   esp_netif_create_default_wifi_ap();

   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));

   ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                       ESP_EVENT_ANY_ID,
                                                       &wifi_event_handler,
                                                       NULL,
                                                       NULL));

   wifi_config_t wifi_config = {
       .ap = {
           .ssid = WIFI_SSID,
           .ssid_len = strlen(WIFI_SSID),
           .channel = WIFI_CHANNEL,
           .password = WIFI_PASS,
           .max_connection = MAX_STA_CONN,
           .authmode = WIFI_AUTH_OPEN
       },
   };

   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
   ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
   ESP_ERROR_CHECK(esp_wifi_start());

   ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s channel:%d", WIFI_SSID, WIFI_CHANNEL);
   
   // Start web server
   server = start_webserver();
}