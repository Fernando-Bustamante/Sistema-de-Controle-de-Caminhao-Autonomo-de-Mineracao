#include "include/tasks.h"
#include <thread>
#include <vector>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    SharedResources res;
    
    // Verifica se o usuário passou um ID (Ex: ./atr_caminhao_app 2)
    if (argc > 1) {
        try {
            res.caminhao_id = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "ID invalido, usando ID 1." << std::endl;
            res.caminhao_id = 1;
        }
    }
    std::cout << "Iniciando Caminhao ID: " << res.caminhao_id << std::endl;

    // --- NOVO: Configuração das Tarefas Assíncronas ---
    // 1. Tarefa de Planejamento de Rota (100ms)
    boost::asio::steady_timer timer_rota(res.io_ctx, std::chrono::milliseconds(100));
    timer_rota.async_wait([&](const boost::system::error_code& ec){ if(ec) return; task_planejamento_rota(res, timer_rota); });

    // 2. Tarefa de Lógica de Comando (50ms)
    boost::asio::steady_timer timer_comando(res.io_ctx, std::chrono::milliseconds(50));
    timer_comando.async_wait([&](const boost::system::error_code& ec){ if(ec) return; task_logica_comando(res, timer_comando); });

    // 3. Tarefa de Monitor de Falhas (200ms)
    boost::asio::steady_timer timer_falhas(res.io_ctx, std::chrono::milliseconds(200));
    timer_falhas.async_wait([&](const boost::system::error_code& ec){ if(ec) return; task_monitor_falhas(res, timer_falhas); });

    // 4. Tarefa de Controle de Navegação (50ms)
    boost::asio::steady_timer timer_navegacao(res.io_ctx, std::chrono::milliseconds(50));
    timer_navegacao.async_wait([&](const boost::system::error_code& ec){ if(ec) return; task_controle_navegacao(res, timer_navegacao); });

    // --- NOVO: Pool de Threads para executar o io_context ---
    // Estas threads executarão as tarefas agendadas (como a de rota)
    std::vector<std::thread> thread_pool;
    const int num_threads_pool = 2; // 2 threads são suficientes para começar
    for (int i = 0; i < num_threads_pool; ++i) {
        thread_pool.emplace_back([&res]() {
            res.io_ctx.run();
        });
    }

    // Tarefas com loop próprio continuam como antes
    std::thread t_sensores(task_tratamento_sensores, std::ref(res));
    std::thread t_coletor(task_coletor_dados, std::ref(res));

    // Interface Local roda na thread principal para Ncurses
    // std::thread t_sensores(task_tratamento_sensores, std::ref(res));
    // std::thread t_comando(task_logica_comando, std::ref(res));
    // std::thread t_rota(task_planejamento_rota, std::ref(res));
    // std::thread t_nav(task_controle_navegacao, std::ref(res));
    // std::thread t_falhas(task_monitor_falhas, std::ref(res));
    // std::thread t_coletor(task_coletor_dados, std::ref(res));
    
    task_interface_local(res); // Bloqueia aqui até 'q' ser pressionado

    // // Join nas threads ao sair
    // if (t_sensores.joinable()) t_sensores.join();
    // if (t_comando.joinable()) t_comando.join();
    // if (t_rota.joinable()) t_rota.join();
    // if (t_nav.joinable()) t_nav.join();
    // if (t_falhas.joinable()) t_falhas.join();
    // if (t_coletor.joinable()) t_coletor.join();

    res.io_ctx.stop(); // Para todas as tarefas assíncronas

    // Join em todas as threads
    for (auto& t : thread_pool) {
        if (t.joinable()) t.join();
    }
    if (t_sensores.joinable()) t_sensores.join();
    if (t_coletor.joinable()) t_coletor.join();

    std::cout << "Sistema finalizado." << std::endl;

    return 0;
}
