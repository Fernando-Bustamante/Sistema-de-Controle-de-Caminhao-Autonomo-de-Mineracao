#include "../../src/include/tasks.h"
#include <boost/asio/steady_timer.hpp>
#include <algorithm>
#include <iostream>

const double STEP_VEL = 5.0;
const double STEP_ANG = 10.0;

void task_logica_comando(SharedResources& res, boost::asio::steady_timer& timer) {
    if (!res.sistema_ativo.load()) return;

    if (res.em_falha.load()) {
        res.em_modo_automatico.store(false);
        res.navegacao_ativa.store(false);
        res.comando_aceleracao_aplicado.store(0.0);
    }

    ComandoEvent cmd;
    if (res.fila_comandos.try_pop(cmd)) {
        
        if (cmd.tipo == ComandoEvent::REARME) {
            if (res.em_falha.load()) {
                SensorSample s = res.buffer.read_latest();
                bool condicao_segura = (s.temperatura <= 110) && (!s.falha_eletrica) && (!s.falha_hidraulica);
                if (condicao_segura) {
                    res.em_falha.store(false);
                    std::cout << "[LOGICA] Sistema REARMADO." << std::endl;
                } else {
                    std::cout << "[LOGICA] Falha ao Rearmar: Condicoes criticas!" << std::endl;
                }
            }
        }
        else if (res.em_falha.load()) {
             // Ignora comandos em falha
        }
        else {
            if (!res.em_modo_automatico.load()) {
                double acel = res.comando_aceleracao_aplicado.load();
                double dir  = res.comando_direcao_aplicado.load();
                switch (cmd.tipo) {
                    case ComandoEvent::ACELERA:   acel += 5.0; break;
                    case ComandoEvent::FREIA:     acel -= 5.0; break;
                    case ComandoEvent::ESQUERDA:  dir -= 5.0; break;
                    case ComandoEvent::DIREITA:   dir += 5.0; break;
                    default: break;
                }
                res.comando_aceleracao_aplicado.store(std::clamp(acel, -100.0, 100.0));
                res.comando_direcao_aplicado.store(std::clamp(dir, -180.0, 180.0));
            }

            double set_vel = res.setpoint_velocidade.load();
            double set_ang = res.setpoint_angulo.load();

            switch (cmd.tipo) {
                case ComandoEvent::MODO_AUTO:
                    res.em_modo_automatico.store(true);
                    break;
                case ComandoEvent::MODO_MANUAL:
                    res.em_modo_automatico.store(false);
                    res.navegacao_ativa.store(false);
                    break;
                case ComandoEvent::SET_VEL_UP:   res.setpoint_velocidade.store(std::min(set_vel + STEP_VEL, 80.0)); break;
                case ComandoEvent::SET_VEL_DOWN: res.setpoint_velocidade.store(std::max(set_vel - STEP_VEL, 0.0)); break;
                case ComandoEvent::SET_ANG_LEFT: set_ang -= STEP_ANG; if (set_ang < -180) set_ang += 360; res.setpoint_angulo.store(set_ang); break;
                case ComandoEvent::SET_ANG_RIGHT:set_ang += STEP_ANG; if (set_ang > 180) set_ang -= 360; res.setpoint_angulo.store(set_ang); break;
                
                // --- CORREÇÃO AQUI ---
                case ComandoEvent::GOTO_WAYPOINT:
                    // NÃO definimos mais 1000,1000 fixo aqui.
                    // O alvo já foi definido pelo MQTT (gestao_mina) ou pelo Teclado (interface_local)
                    res.navegacao_ativa.store(true);
                    res.em_modo_automatico.store(true);
                    std::cout << "[LOGICA] Navegacao ATIVA para (" 
                              << res.target_pos_x.load() << ", " 
                              << res.target_pos_y.load() << ")" << std::endl;
                    break;

                default: break;
            }
        }
    }

    timer.expires_at(timer.expiry() + std::chrono::milliseconds(50));
    timer.async_wait([&res, &timer](const boost::system::error_code& ec) {
        if (ec) return;
        task_logica_comando(res, timer);
    });
}
