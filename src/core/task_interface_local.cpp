#include "../../src/include/tasks.h"
#include <ncurses.h>
#include <cmath> 
#include <string>

void task_interface_local(SharedResources& res) {
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); timeout(100); 

    std::string ultima_falha_msg;

    while (res.sistema_ativo.load()) {

        FalhaEvent falha;
        if(res.fila_falhas.try_pop(falha)) {
            ultima_falha_msg = "ALERTA: " + falha.componente + (falha.critico ? " [CRITICA]" : " [NAO CRITICA]");
        }

        clear();
        mvprintw(0, 2, "=== CAMINHAO AUTONOMO (ID %d) ===", res.caminhao_id);
        
        if (res.em_falha.load()) 
            mvprintw(2, 2, "Modo: [DEFEITO] - Use 'z' para Rearmar");
        else if (res.navegacao_ativa.load())
            mvprintw(2, 2, "Modo: [NAVEGACAO] -> Alvo (%d, %d)", res.target_pos_x.load(), res.target_pos_y.load());
        else if (res.em_modo_automatico.load()) 
            mvprintw(2, 2, "Modo: [AUTOMATICO] (PID Ativo)");
        else 
            mvprintw(2, 2, "Modo: [MANUAL] (Use as Setas)");

        SensorSample last = res.buffer.read_latest();
        
        mvprintw(4, 2, "POSICAO: X: %d | Y: %d | Ang %d", last.pos_x, last.pos_y, last.angulo);
        mvprintw(5, 2, "SENSORES: Vel %.1f km/h | Temp %d C", last.velocidade, last.temperatura);

        mvprintw(7, 2, "SETPOINT: Vel %.1f | Ang %.0f", res.setpoint_velocidade.load(), res.setpoint_angulo.load());
        mvprintw(8, 2, "ATUADORES: Acel %.0f%% | Dir %.0f", res.comando_aceleracao_aplicado.load(), res.comando_direcao_aplicado.load());
        
        mvprintw(10, 2, "[Setas]:Dirigir | [G]:GoTo Teste | [Z]:Rearme | [Q]:Sair");
        
        if (!ultima_falha_msg.empty()) {
            attron(A_BOLD | A_REVERSE);
            mvprintw(12, 2, "%s", ultima_falha_msg.c_str());
            attroff(A_BOLD | A_REVERSE);
        }
        
        refresh();

        int ch = getch();
        ComandoEvent cmd; cmd.tipo = ComandoEvent::NENHUM;

        switch(ch) {
            case 'a': cmd.tipo = ComandoEvent::MODO_AUTO; break;
            case 'm': cmd.tipo = ComandoEvent::MODO_MANUAL; break;
            case 'q': res.sistema_ativo.store(false); break;
            case 'z': cmd.tipo = ComandoEvent::REARME; break; // Tecla Z rearmando

            case KEY_UP:    cmd.tipo = ComandoEvent::ACELERA; break;
            case KEY_DOWN:  cmd.tipo = ComandoEvent::FREIA; break;
            case KEY_LEFT:  cmd.tipo = ComandoEvent::ESQUERDA; break;
            case KEY_RIGHT: cmd.tipo = ComandoEvent::DIREITA; break;
            
            case 'r': cmd.tipo = ComandoEvent::SET_VEL_UP; break;
            case 'f': cmd.tipo = ComandoEvent::SET_VEL_DOWN; break;
            case 't': cmd.tipo = ComandoEvent::SET_ANG_LEFT; break;
            case 'y': cmd.tipo = ComandoEvent::SET_ANG_RIGHT; break;
            
            case 'g': 
                // Define um alvo de teste APENAS se apertar 'g' no teclado
                res.target_pos_x.store(1000); 
                res.target_pos_y.store(1000);
                cmd.tipo = ComandoEvent::GOTO_WAYPOINT; 
                break;
        }

        if (cmd.tipo != ComandoEvent::NENHUM) res.fila_comandos.push(cmd);
    }
    endwin();
}
