// ARQUIVO: src/core/task_controle_navegacao.cpp
#include "../../src/include/tasks.h"
#include <boost/asio/steady_timer.hpp>
#include <algorithm>
#include <cmath>

const double KP_VEL = 2.0; const double KI_VEL = 0.5;   
const double KP_ANG = 1.5; const double KI_ANG = 0.05;  
const double DT = 0.05;

void task_controle_navegacao(SharedResources& res, boost::asio::steady_timer& timer) {
    if (!res.sistema_ativo.load()) return;

    SensorSample atual = res.buffer.read_latest();

    // Se estiver em modo Manual OU em Falha, não controla
    if (!res.em_modo_automatico.load() || res.em_falha.load()) {
        // Reseta Integradores
        res.pid_integral_erro_vel = 0.0;
        res.pid_integral_erro_ang = 0.0;
        
        // --- BUMPLESS TRANSFER (Requisito 5) ---
        // "Coloca o setpoint do controlador para os valores atuais"
        // Só faz sentido se NÃO estiver em navegação ativa (GoTo)
        if (!res.navegacao_ativa.load()) {
             res.setpoint_velocidade.store(atual.velocidade);
             res.setpoint_angulo.store((double)atual.angulo);
        }
    } else {
        // MODO AUTOMÁTICO (PID) - (Código igual ao anterior)
        double set_vel = res.setpoint_velocidade.load();
        double set_ang = res.setpoint_angulo.load();
        
        double erro_vel = set_vel - atual.velocidade;
        if (std::abs(erro_vel) > 0.5) res.pid_integral_erro_vel += erro_vel * DT;
        res.pid_integral_erro_vel = std::clamp(res.pid_integral_erro_vel, -100.0, 100.0);
        double out_vel = (KP_VEL * erro_vel) + (KI_VEL * res.pid_integral_erro_vel);
        res.comando_aceleracao_aplicado.store(std::clamp(out_vel, -100.0, 100.0));
        
        double erro_ang = set_ang - (double)atual.angulo;
        while (erro_ang > 180.0)  erro_ang -= 360.0;
        while (erro_ang < -180.0) erro_ang += 360.0;
        res.pid_integral_erro_ang += erro_ang * DT;
        res.pid_integral_erro_ang = std::clamp(res.pid_integral_erro_ang, -50.0, 50.0);
        double out_ang = (KP_ANG * erro_ang) + (KI_ANG * res.pid_integral_erro_ang);
        res.comando_direcao_aplicado.store(std::clamp(out_ang, -180.0, 180.0));
    }

    timer.expires_at(timer.expiry() + std::chrono::milliseconds(50));
    timer.async_wait([&res, &timer](const boost::system::error_code& ec) {
        if (ec) return;
        task_controle_navegacao(res, timer);
    });
}
