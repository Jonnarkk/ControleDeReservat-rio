// Ficheiro: server.c (Versão Final, Estável e Unificada)

#include "server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "global_manage.h"

// ====================================================================================
// ===================  PÁGINA WEB (HTML / CSS / JAVASCRIPT) ==========================
// ====================================================================================
// HTML minificado para economizar espaço
// HTML/CSS minificado e JavaScript com a correção de foco nos inputs.
const char HTML_BODY[] =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Controle de Nível</title>"
    "<style>"
    "body{font-family:sans-serif;text-align:center;padding:20px;background:#f0f2f5}"
    "h1{color:#333}.container{max-width:500px;margin:auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,.1)}"
    ".barra{width:100%;background:#e9ecef;border-radius:8px;overflow:hidden;margin:10px 0 20px 0;height:30px;border:1px solid #ccc}"
    ".preenchimento{height:100%;transition:width .5s ease-in-out;background:#007bff}"
    ".label{font-weight:bold;margin-top:15px;margin-bottom:5px;display:block;color:#555}"
    ".status-motor{font-size:24px;font-weight:bold;padding:10px;border-radius:5px}"
    ".status-motor.ligado{color:#28a745;background:#e9f7ec}.status-motor.desligado{color:#dc3545;background:#fdeeee}"
    ".input-group{margin:10px 0;display:flex;justify-content:center;align-items:center;gap:10px}"
    ".input-group input{width:80px;padding:8px;text-align:center;font-size:16px;border:1px solid #ccc;border-radius:5px}"
    ".input-group button{padding:8px 15px;border:none;background:#5cb85c;color:white;border-radius:5px;cursor:pointer}"
    "</style>"
    "<script>"
    "function setLimite(e){const t=document.getElementById('input_'+e).value;if(t>=0&&t<=100){fetch('/nivel/'+e+'?valor='+t);document.getElementById('input_'+e).blur()}else{alert('Por favor, insira um valor entre 0 e 100.')}}"
    "function atualizar(){fetch('/nivel/estado').then(e=>e.json()).then(e=>{document.getElementById('nivel_valor').innerText=e.nivel_pc;document.getElementById('barra_nivel').style.width=e.nivel_pc+'%';const t=document.getElementById('motor_estado');t.innerText=e.motor_status?'LIGADO':'DESLIGADO';t.className='status-motor '+(e.motor_status?'ligado':'desligado');const n=document.getElementById('input_min');document.activeElement!==n&&(n.value=e.nivel_min);const o=document.getElementById('input_max');document.activeElement!==o&&(o.value=e.nivel_max)}).catch(e=>console.error('Erro:',e))}"
    "setInterval(atualizar,500);window.onload=atualizar;"
    "</script></head><body>"
    "<div class=container><h1>Controle de Nível de Água</h1>"
    "<p class=label>Nível Atual: <span id=nivel_valor>--</span>%</p>"
    "<div class=barra><div id=barra_nivel class=preenchimento></div></div>"
    "<p class=label>Estado do Motor:</p><p id=motor_estado class='status-motor desligado'>--</p><hr style=margin:20px 0>"
    "<div class=input-group><label for=input_min>Mínimo (%):</label><input type=number id=input_min min=0 max=100><button onclick=\"setLimite('min')\">Definir</button></div>"
    "<div class=input-group><label for=input_max>Máximo (%):</label><input type=number id=input_max min=0 max=100><button onclick=\"setLimite('max')\">Definir</button></div>"
    "</div></body></html>";

// Estrutura de estado para cada conexão. Contém o buffer de resposta.
typedef struct HTTP_STATE_T {
    char response[4096]; // Buffer para TODAS as respostas (HTML cabe aqui)
    size_t len;
    size_t sent;
} HTTP_STATE;

// --- Funções do Servidor ---

// Callback chamado DEPOIS que os dados são enviados.
// Fecha a conexão e libera a memória de estado.
static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    HTTP_STATE *state = (HTTP_STATE *)arg;
    state->sent += len;

    // Se tudo foi enviado, podemos fechar
    if (state->sent >= state->len) {
        free(state);
        tcp_close(tpcb);
    }
    return ERR_OK;
}

// Callback chamado quando DADOS SÃO RECEBIDOS.
// Esta é a função principal que processa TODAS as requisições.
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    // Se não houver dados ou houver erro, apenas feche a conexão.
    if (!p || err != ERR_OK) {
        if (p) pbuf_free(p);
        tcp_close(tpcb);
        return ERR_OK;
    }

    // [CORREÇÃO CRÍTICA 1] Avisar à pilha lwIP que recebemos os dados.
    tcp_recved(tpcb, p->tot_len);

    // [CORREÇÃO CRÍTICA 2] Copiar a requisição para um buffer local seguro.
    char req_buffer[128];
    pbuf_copy_partial(p, req_buffer, p->len < sizeof(req_buffer) ? p->len : sizeof(req_buffer) - 1, 0);
    req_buffer[p->len < sizeof(req_buffer) ? p->len : sizeof(req_buffer) - 1] = '\0';

    // Libera o pbuf de entrada IMEDIATAMENTE. Já copiamos o que precisávamos.
    pbuf_free(p);

    // Alocar um estado para esta requisição específica.
    HTTP_STATE *state = malloc(sizeof(HTTP_STATE));
    if (!state) {
        tcp_close(tpcb);
        return ERR_MEM;
    }
    state->sent = 0;

    char *body_ptr = NULL;
    char body_buffer[128]; // Buffer temporário para corpos de resposta pequenos (JSON, texto)

    // [CORREÇÃO CRÍTICA 3] Roteamento seguro com strncmp.
    if (strncmp(req_buffer, "GET /nivel/max?valor=", 21) == 0) {
        char *valor_str = strstr(req_buffer, "valor=") + 6;
        set_max(atoi(valor_str));
        strcpy(body_buffer, "Maximo definido");
        body_ptr = body_buffer;
        state->len = snprintf(state->response, sizeof(state->response),
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", strlen(body_ptr), body_ptr);
    }
    else if (strncmp(req_buffer, "GET /nivel/min?valor=", 21) == 0) {
        char *valor_str = strstr(req_buffer, "valor=") + 6;
        set_min(atoi(valor_str));
        strcpy(body_buffer, "Minimo definido");
        body_ptr = body_buffer;
        state->len = snprintf(state->response, sizeof(state->response),
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", strlen(body_ptr), body_ptr);
    }
    else if (strncmp(req_buffer, "GET /nivel/estado ", 18) == 0) {
        snprintf(body_buffer, sizeof(body_buffer),
            "{\"nivel_pc\":%d,\"motor_status\":%s,\"nivel_min\":%d,\"nivel_max\":%d}",
            get_nivel(), get_motor() ? "true" : "false", get_min(), get_max());
        body_ptr = body_buffer;
        state->len = snprintf(state->response, sizeof(state->response),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", strlen(body_ptr), body_ptr);
    }
    else if (strncmp(req_buffer, "GET / ", 6) == 0) {
        body_ptr = (char*)HTML_BODY;
        state->len = snprintf(state->response, sizeof(state->response),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", strlen(body_ptr), body_ptr);
    }
    else {
        strcpy(body_buffer, "<h1>404 Not Found</h1>");
        body_ptr = body_buffer;
        state->len = snprintf(state->response, sizeof(state->response),
            "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", strlen(body_ptr), body_ptr);
    }

    // Configura o callback 'http_sent' para limpar a memória e fechar a conexão DEPOIS do envio.
    tcp_arg(tpcb, state);
    tcp_sent(tpcb, http_sent);

    // Envia a resposta. O callback http_sent cuidará do fechamento.
    tcp_write(tpcb, state->response, state->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || !newpcb) return err;
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) return;
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        tcp_close(pcb);
        return;
    }
    pcb = tcp_listen(pcb);
    if (!pcb) return;
    tcp_accept(pcb, connection_callback);
    printf("Servidor de Controle de Nível rodando na porta 80...\n");
}