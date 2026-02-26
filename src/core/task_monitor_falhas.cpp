// ARQUIVO: src/core/task_monitor_falhas.cpp
#include "../include/tasks.h"
#include <boost/asio/steady_timer.hpp>
#include <iostream>

void task_monitor_falhas(SharedResources& res, boost::asio::steady_timer& timer) {
    if (!res.sistema_ativo.load()) return;

    SensorSample atual = res.buffer.read_latest();
    bool nova_falha = false;
    std::string comp;

    // 1. Falha de Temperatura
    if (atual.temperatura > 130) {
        comp = "MOTOR (Superaquecimento)";
        nova_falha = true;
    }
    // 2. Falha Elétrica
    else if (atual.falha_eletrica) {
        comp = "SISTEMA ELETRICO";
        nova_falha = true;
    }
    // 3. Falha Hidráulica
    else if (atual.falha_hidraulica) {
        comp = "SISTEMA HIDRAULICO";
        nova_falha = true;
    }

    // Se detectou e o sistema ainda não estava em estado de falha
    if (nova_falha && !res.em_falha.load()) {
        FalhaEvent f;
        f.componente = comp;
        f.critico = true;
        res.fila_falhas.push(f);
        
        // Marca o sistema como "Em Falha" (bloqueia operações)
        res.em_falha.store(true);
        std::cerr << "[MF] FALHA DETECTADA: " << comp << std::endl;
    }

    timer.expires_at(timer.expiry() + std::chrono::milliseconds(200));
    timer.async_wait([&res, &timer](const boost::system::error_code& ec) {
        if (ec) return;
        task_monitor_falhas(res, timer);
    });
}
