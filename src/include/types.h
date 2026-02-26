// ARQUIVO: src/include/types.h
#pragma once
#include <chrono>
#include <string>
#include <atomic>

struct SensorSample {
    std::chrono::steady_clock::time_point ts;
    int pos_x = 0;
    int pos_y = 0;
    int angulo = 0;
    double velocidade = 0.0;
    int temperatura = 0;
    // Falhas vindas do simulador
    bool falha_eletrica = false;
    bool falha_hidraulica = false;
};

struct ComandoEvent {
    enum Tipo { 
        NENHUM, 
        MODO_AUTO, MODO_MANUAL, 
        ACELERA, FREIA, ESQUERDA, DIREITA, 
        SET_VEL_UP, SET_VEL_DOWN, SET_ANG_LEFT, SET_ANG_RIGHT,
        GOTO_WAYPOINT,
        REARME // <--- NOVO: Comando de Rearme
    } tipo;
};

struct FalhaEvent {
    std::string componente;
    bool critico;
};
