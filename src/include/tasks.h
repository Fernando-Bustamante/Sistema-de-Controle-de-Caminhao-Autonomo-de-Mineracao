#pragma once
#include "SharedResources.h"
#include <boost/asio/steady_timer.hpp> // Adicionado para o tipo do timer

// --- Tarefas Assíncronas (gerenciadas pelo Boost.Asio) ---
// A assinatura delas agora inclui uma referência ao seu timer
void task_monitor_falhas(SharedResources& res, boost::asio::steady_timer& timer);
void task_controle_navegacao(SharedResources& res, boost::asio::steady_timer& timer);
void task_logica_comando(SharedResources& res, boost::asio::steady_timer& timer);
void task_planejamento_rota(SharedResources& res, boost::asio::steady_timer& timer);

// --- Tarefas com Loop Próprio (em threads dedicadas) ---
// A assinatura delas permanece a mesma
void task_tratamento_sensores(SharedResources& res);
void task_coletor_dados(SharedResources& res);
void task_interface_local(SharedResources& res);
