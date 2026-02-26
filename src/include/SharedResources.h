// ARQUIVO: src/include/SharedResources.h
#pragma once
#include "types.h"
#include "CircularBuffer.h"
#include "ThreadSafeQueue.h"
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

struct SharedResources {
    int caminhao_id = 1;

    std::atomic<bool> sistema_ativo{true};
    std::atomic<bool> em_modo_automatico{false};
    std::atomic<bool> em_falha{false}; // <--- NOVO: Estado de falha global
    
    std::atomic<double> setpoint_velocidade{0.0};
    std::atomic<double> setpoint_angulo{0.0};
    
    std::atomic<double> comando_aceleracao_aplicado{0.0};
    std::atomic<double> comando_direcao_aplicado{0.0};

    std::atomic<bool> navegacao_ativa{false}; 
    std::atomic<int> target_pos_x{0};
    std::atomic<int> target_pos_y{0};

    CircularBuffer buffer;
    ThreadSafeQueue<ComandoEvent> fila_comandos;
    ThreadSafeQueue<FalhaEvent> fila_falhas;
    boost::asio::io_context io_ctx;

    double pid_integral_erro_vel = 0.0;
    double pid_integral_erro_ang = 0.0;
};
