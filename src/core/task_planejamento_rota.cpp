#include "../../src/include/tasks.h"
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <functional> // Necessário para std::bind

// Constantes de Navegação (permanecem as mesmas)
const double PI = 3.14159265358979323846;
const double DISTANCIA_CHEGADA = 30.0;
const double VEL_CRUZEIRO = 60.0;
const double VEL_APROXIMACAO = 20.0;

// A assinatura da função muda: agora ela recebe uma referência ao seu timer
void task_planejamento_rota(SharedResources& res, boost::asio::steady_timer& timer) {
    // Se o sistema estiver desligando, a tarefa simplesmente para de se re-agendar
    if(!res.sistema_ativo.load()) {
        return;
    }

    if(res.navegacao_ativa.load() && res.em_modo_automatico.load()) {
        SensorSample atual = res.buffer.read_latest();
        double x_atual = (double)atual.pos_x;
        double y_atual = (double)atual.pos_y;
        
        double x_alvo = (double)res.target_pos_x.load();
        double y_alvo = (double)res.target_pos_y.load();
        
        double dx = x_alvo - x_atual;
        double dy = y_alvo - y_atual;
        double distancia = std::sqrt(dx*dx + dy*dy);
        
        if(distancia < DISTANCIA_CHEGADA) {
            res.setpoint_velocidade.store(0.0);
            res.navegacao_ativa.store(false);
            std::cout << "[ROTA] Destino alcancado com sucesso!" << std::endl;
        } else {
            double ang_rad = std::atan2(dy, dx);
            double ang_deg = ang_rad * (180.0 / PI);
            res.setpoint_angulo.store(ang_deg);
            
            if (distancia > 150.0){
                res.setpoint_velocidade.store(VEL_CRUZEIRO);
            } else {
                res.setpoint_velocidade.store(VEL_APROXIMACAO);
            }
        }
    }

    // lógica de reagendamento com timer de 100ms
    timer.expires_at(timer.expiry() + std::chrono::milliseconds(100));
    
    // 2. Pede ao io_context para chamar esta mesma função quando o timer expirar
    timer.async_wait([&res, &timer](const boost::system::error_code& ec) {
        if (ec) return;
        task_planejamento_rota(res, timer);
    });
}
