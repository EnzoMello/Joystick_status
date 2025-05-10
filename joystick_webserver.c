#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h" // Para acessar netif_default e IP

// Configurações de Wi-Fi
#define WIFI_SSID "Roteadorzin"
#define WIFI_PASSWORD "ENZOMELO1000"

// Definição dos botões
#define BUTTON_A 5
#define BUTTON_B 6

/** 
 * @brief Lê o valor do eixo X do joystick e retorna a porcentagem de deslocamento.
 * 
 * A leitura é realizada através do ADC e o valor é convertido em uma porcentagem
 * (0 a 100) com base no valor máximo (4095).
 * 
 * @return A porcentagem de deslocamento do eixo X (0-100%).
 */
int read_joystick_x() {
    adc_select_input(1);
    return adc_read() * 100 / 4095;  // Lê o valor de X (0-100%)
}

/** 
 * @brief Lê o valor do eixo Y do joystick e retorna a porcentagem de deslocamento.
 * 
 * A leitura é realizada através do ADC e o valor é convertido em uma porcentagem
 * (0 a 100) com base no valor máximo (4095).
 * 
 * @return A porcentagem de deslocamento do eixo Y (0-100%).
 */
int read_joystick_y() {
    adc_select_input(0);
    return adc_read() * 100 / 4095;
}

/** 
 * @brief Determina a direção da rosa dos ventos com base nos valores dos eixos X e Y.
 * 
 * O cálculo é feito considerando os valores dos eixos do joystick e uma zona morta 
 * para evitar pequenas variações. As direções possíveis são: Norte, Sul, Leste, Oeste, 
 * Nordeste, Sudeste, Noroeste, Sudoeste e Centro.
 * 
 * @param x Valor do eixo X do joystick (0-100%).
 * @param y Valor do eixo Y do joystick (0-100%).
 * @param dead_zone Valor da zona morta para a detecção da direção.
 * 
 * @return A direção da rosa dos ventos correspondente.
 */
const char* joystick_direction(int x, int y, int dead_zone) {
    int dx = 0, dy = 0;

    if (x > (50 + dead_zone)) dx = 1;
    else if (x < (50 - dead_zone)) dx = -1;

    if (y > (50 + dead_zone)) dy = 1;
    else if (y < (50 - dead_zone)) dy = -1;

    // Mapeia (dx, dy) para direções
    const char* directions[3][3] = {
        {"Sudoeste", "Oeste", "Noroeste"},
        {"Sul",      "Centro", "Norte"},
        {"Sudeste", "Leste",  "Nordeste"}
    };

    return directions[dx + 1][dy + 1];  // dx e dy variam de -1 a +1
}

/** 
 * @brief Função de callback que processa requisições HTTP recebidas pelo servidor TCP.
 * 
 * Essa função recebe os dados da requisição, verifica se é uma requisição para
 * obter os dados do joystick e da direção da rosa dos ventos, e gera uma resposta
 * no formato JSON ou HTML.
 * 
 * @param arg Argumento passado para a função (não utilizado aqui).
 * @param tpcb Ponteiro para o controle da conexão TCP.
 * @param p Ponteiro para o buffer de dados da requisição.
 * @param err Código de erro da requisição.
 * 
 * @return ERR_OK para indicar que a requisição foi processada com sucesso.
 */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Copia o payload completo para uma string terminada em '\0'
    char *request = malloc(p->tot_len + 1);
    if (!request) {
        pbuf_free(p);
        return ERR_MEM;
    }
    memcpy(request, p->payload, p->tot_len);
    request[p->tot_len] = '\0';

    // Informa ao stack LWIP que consumimos os bytes
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    // Lê sensores
    int eixo_x = read_joystick_x(); // Eixo X do Joystick
    int eixo_y = read_joystick_y(); // Eixo Y do Joystick
    const char* direction = joystick_direction(eixo_x, eixo_y, 10);  // Direção da rosa dos ventos

    bool is_data = (strstr(request, "GET /data") != NULL);

    // Buffer de resposta único, em memória estática
    static char response_buffer[1024];

    if (is_data) {
        // monta JSON
        char json_body[256];
        int n = snprintf(json_body, sizeof(json_body),
            "{\"eixo_x\":%d,\"eixo_y\":%d,\"direcao\":\"%s\"}",
            eixo_x, eixo_y, direction);

        int header_len = snprintf(response_buffer, sizeof(response_buffer),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n",
            n);
        // copia o corpo JSON após o header
        memcpy(response_buffer + header_len, json_body, n);
    } else {
        // monta HTML
        const char *html_body =
            "<!DOCTYPE html><html><head>"
            "<meta charset=\"utf-8\">"
            "<title>Monitoramento Joystick</title>"
            "<style>"
              "body{font-family:Arial,sans-serif;text-align:center;margin:20px;}"
              ".data{font-size:1.2em;margin:8px;}"
            "</style>"
            "<script>"
              "function update(){"
                "fetch('/data').then(r=>r.json()).then(d=>{"
                  "document.getElementById('eixo_x').textContent=d.eixo_x.toFixed(2);"
                  "document.getElementById('eixo_y').textContent=d.eixo_y.toFixed(2);"
                  "document.getElementById('direcao').textContent=d.direcao;"
                "});"
              "} "
              "setInterval(update,1000);window.onload=update;"
            "</script>"
            "</head><body>"
            "<h1>Monitor de Joystick e Rosa dos Ventos</h1>"
            "<div class=\"data\">Posição X: <span id=\"eixo_x\">-</span></div>"
            "<div class=\"data\">Posição Y: <span id=\"eixo_y\">-</span></div>"
            "<div class=\"data\">Direção: <span id=\"direcao\">-</span></div>"
            "</body></html>";

        int body_len = strlen(html_body);
        int header_len = snprintf(response_buffer, sizeof(response_buffer),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n",
            body_len);
        memcpy(response_buffer + header_len, html_body, body_len);
    }

    // envia tudo de uma vez
    tcp_write(tpcb, response_buffer, strlen(response_buffer), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    free(request);
    return ERR_OK;
}

/** 
 * @brief Função de callback que é chamada quando uma nova conexão TCP é aceita.
 * 
 * Esta função define a função de recepção de dados para a nova conexão TCP.
 * 
 * @param arg Argumento passado para a função (não utilizado aqui).
 * @param newpcb Ponteiro para o controle da nova conexão TCP.
 * @param err Código de erro da conexão.
 * 
 * @return ERR_OK para indicar que a nova conexão foi aceita com sucesso.
 */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

/** 
 * @brief Função principal que inicializa o servidor TCP e gerencia a conexão Wi-Fi.
 * 
 * Esta função inicializa o Wi-Fi, configura o servidor TCP na porta 80 e entra no
 * loop principal para processar as requisições.
 * 
 * @return 0 em caso de sucesso ou -1 em caso de falha.
 */
int main()
{
    stdio_init_all();
    sleep_ms(2000); // Espera para o terminal conectar

    // Inicializa ADC para joystick
    adc_init();

    // Inicializa Wi-Fi
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return -1;
    }

    printf("Conectado ao Wi-Fi\n");

    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP (porta 80)
    struct tcp_pcb *server = tcp_new();
    if (!server) {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);

    printf("Servidor ouvindo na porta 80\n");

    // Loop principal
    while (true) {
        cyw43_arch_poll(); // Necessário para manter Wi-Fi funcionando
    }

    cyw43_arch_deinit();
    return 0;
}
