// Ficheiro: server.c (Versão Limpa e Corrigida)

#include "server.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Para atoi

#include "pico/stdlib.h"

// Inclui nossas variáveis globais e definições
#include "global_manage.h"

// ====================================================================================
// ===================  PÁGINA WEB (HTML / CSS / JAVASCRIPT) ==========================
// ====================================================================================
// Nenhuma alteração aqui, a string estava correta.
const char HTML_BODY[] =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Controle de Nível</title>"
    "<style>"
    "body { font-family: sans-serif; text-align: center; padding: 20px; margin: 0; background: #f0f2f5; }"
    "h1 { color: #333; }"
    ".container { max-width: 500px; margin: auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
    ".barra { width: 100%; background: #e9ecef; border-radius: 8px; overflow: hidden; margin: 10px 0 20px 0; height: 30px; border: 1px solid #ccc; }"
    ".preenchimento { height: 100%; transition: width 0.5s ease-in-out; background: #007bff; }"
    ".label { font-weight: bold; margin-top: 15px; margin-bottom: 5px; display: block; color: #555; }"
    ".status-motor { font-size: 24px; font-weight: bold; padding: 10px; border-radius: 5px; }"
    ".status-motor.ligado { color: #28a745; background: #e9f7ec; }"
    ".status-motor.desligado { color: #dc3545; background: #fdeeee; }"
    ".input-group { margin: 10px 0; display: flex; justify-content: center; align-items: center; gap: 10px; }"
    ".input-group input { width: 80px; padding: 8px; text-align: center; font-size: 16px; border: 1px solid #ccc; border-radius: 5px; }"
    ".input-group button { padding: 8px 15px; border: none; background: #5cb85c; color: white; border-radius: 5px; cursor: pointer; }"
    "</style>"
    "<script>"
    "function setLimite(tipo) {"
    "  const valor = document.getElementById('input_' + tipo).value;"
    "  if (valor >= 0 && valor <= 100) {"
    "    fetch('/nivel/' + tipo + '?valor=' + valor);"
    "  } else { alert('Por favor, insira um valor entre 0 e 100.'); }"
    "}"
    "function atualizar() {"
    "  fetch('/nivel/estado').then(res => res.json()).then(data => {"
    "    document.getElementById('nivel_valor').innerText = data.nivel_pc;"
    "    document.getElementById('barra_nivel').style.width = data.nivel_pc + '%';"
    "    const motorStatusEl = document.getElementById('motor_estado');"
    "    motorStatusEl.innerText = data.motor_status ? 'LIGADO' : 'DESLIGADO';"
    "    motorStatusEl.className = 'status-motor ' + (data.motor_status ? 'ligado' : 'desligado');"
    "    document.getElementById('input_min').value = data.nivel_min;"
    "    document.getElementById('input_max').value = data.nivel_max;"
    "  });"
    "}"
    "setInterval(atualizar, 1500);"
    "window.onload = atualizar;"
    "</script></head><body>"
    "<div class='container'>"
    "<h1>Controle de Nível de Água</h1>"
    "<p class='label'>Nível Atual: <span id='nivel_valor'>--</span>%</p>"
    "<div class='barra'><div id='barra_nivel' class='preenchimento'></div></div>"
    "<p class='label'>Estado do Motor:</p>"
    "<p id='motor_estado' class='status-motor desligado'>--</p>"
    "<hr style='margin: 20px 0;'>"
    "<div class='input-group'>"
    "  <label for='input_min'>Mínimo (%):</label>"
    "  <input type='number' id='input_min' min='0' max='100'>"
    "  <button onclick=\"setLimite('min')\">Definir</button>"
    "</div>"
    "<div class='input-group'>"
    "  <label for='input_max'>Máximo (%):</label>"
    "  <input type='number' id='input_max' min='0' max='100'>"
    "  <button onclick=\"setLimite('max')\">Definir</button>"
    "</div>"
    "</div></body></html>";

// Estrutura para manter o estado da conexão
struct http_state {
    char response[4096];
    size_t len;
    size_t sent;
};

// --- Funções do Servidor ---
// Adicionei 'static' para manter as boas práticas, pois elas só são usadas neste arquivo.

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    if (hs->sent >= hs->len) {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs) {
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    hs->sent = 0;

    // --- ROTEAMENTO DAS REQUISIÇÕES ---
    if (strstr(req, "GET /nivel/max?valor=")) {
        char *valor_str = strstr(req, "valor=") + 6;
        set_max(atoi(valor_str));
        const char *txt = "Maximo definido";
        hs->len = snprintf(hs->response, sizeof(hs->response), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", (int)strlen(txt), txt);
    }
    else if (strstr(req, "GET /nivel/min?valor=")) {
        char *valor_str = strstr(req, "valor=") + 6;
        set_min(atoi(valor_str));
        const char *txt = "Minimo definido";
        hs->len = snprintf(hs->response, sizeof(hs->response), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", (int)strlen(txt), txt);
    }
    else if (strstr(req, "GET /nivel/estado")) {
        int nivel = get_nivel();
        int max = get_max();
        int min = get_min();

        if (nivel < min) {
            set_motor(true);
        } else if (nivel > max) {
            set_motor(false);
        }

        char json_payload[128];
        int json_len = snprintf(json_payload, sizeof(json_payload),
                                "{\"nivel_pc\":%d,\"motor_status\":%s,\"nivel_min\":%d,\"nivel_max\":%d}",
                                nivel, get_motor() ? "true" : "false", min, max);

        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
                           "Content-Length: %d\r\nConnection: close\r\n\r\n%s",
                           json_len, json_payload);
    }
    else {
        // Rota padrão: servir a página HTML principal
        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
                           "Content-Length: %d\r\nConnection: close\r\n\r\n%s",
                           (int)strlen(HTML_BODY), HTML_BODY);
    }

    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);
    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB TCP\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor de Controle de Nível rodando na porta 80...\n");
}