// ARQUIVO: src/core/task_tratamento_sensores.cpp
#include "../../src/include/tasks.h"
#include <iostream>
#include <chrono>
#include <mqtt/async_client.h>
#include <string>
#include <deque> // Necessário para a média móvel
#include <numeric>

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const int JANELA_MEDIA = 5; // Média móvel das últimas 5 amostras

void task_tratamento_sensores(SharedResources& res) {
    std::string client_id = "Caminhao_" + std::to_string(res.caminhao_id) + "_Input";
    std::string topic = "simulacao/" + std::to_string(res.caminhao_id) + "/sensores";

    mqtt::async_client client(SERVER_ADDRESS, client_id);
    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);

    try {
        client.connect(connOpts)->wait();
        client.start_consuming();
        client.subscribe(topic, 1)->wait();
    } catch (...) {}

    // Buffers locais para média móvel
    std::deque<int> hist_x, hist_y, hist_ang;

    while (res.sistema_ativo.load()) {
        mqtt::const_message_ptr msg;
        if (client.try_consume_message_for(&msg, std::chrono::milliseconds(50))) {
            std::string payload = msg->get_payload_str();
            SensorSample s;
            s.ts = std::chrono::steady_clock::now();
            
            int raw_x, raw_y, raw_ang, temp, f_elet, f_hid;
            int vel_int;
            
            // ATUALIZADO: Lê agora 7 valores (incluindo falhas)
            // Formato esperado: x,y,ang,vel,temp,falha_elet(0/1),falha_hid(0/1)
            if (sscanf(payload.c_str(), "%d,%d,%d,%d,%d,%d,%d", 
                       &raw_x, &raw_y, &raw_ang, &vel_int, &temp, &f_elet, &f_hid) >= 5) {
                
                // --- 1. FILTRO DE MÉDIA MÓVEL ---
                hist_x.push_back(raw_x);
                hist_y.push_back(raw_y);
                hist_ang.push_back(raw_ang);

                if (hist_x.size() > JANELA_MEDIA) hist_x.pop_front();
                if (hist_y.size() > JANELA_MEDIA) hist_y.pop_front();
                if (hist_ang.size() > JANELA_MEDIA) hist_ang.pop_front();

                // Calcula média
                long sum_x = 0, sum_y = 0, sum_ang = 0;
                for(int v : hist_x) sum_x += v;
                for(int v : hist_y) sum_y += v;
                for(int v : hist_ang) sum_ang += v;

                s.pos_x = (!hist_x.empty()) ? (sum_x / hist_x.size()) : raw_x;
                s.pos_y = (!hist_y.empty()) ? (sum_y / hist_y.size()) : raw_y;
                s.angulo = (!hist_ang.empty()) ? (sum_ang / hist_ang.size()) : raw_ang;
                
                // Outros dados (sem filtro)
                s.velocidade = (double)vel_int;
                s.temperatura = temp;
                s.falha_eletrica = (f_elet == 1);
                s.falha_hidraulica = (f_hid == 1);

                res.buffer.write(s);
            }
        }
    }
    if (client.is_connected()) client.disconnect()->wait();
}
