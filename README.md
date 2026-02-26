# Trabalho Final - Automação em Tempo Real (ATR)
## Sistema de Controle de Caminhão Autônomo de Mineração

Este repositório contém a implementação de um sistema de controle concorrente para um caminhão de mineração autônomo, incluindo um simulador de física e uma interface de gerenciamento central.

---

## 1. Arquitetura Geral

O sistema é composto por três processos independentes que se comunicam via rede através do protocolo MQTT.

1.  **Núcleo de Controle (C++):** A aplicação embarcada no caminhão. Utiliza uma arquitetura multithread com `Boost.Asio` para tarefas periódicas (controle, planejamento) e `std::thread` para tarefas de I/O bloqueante (sensores, logs).

2.  **Simulador Global (Python):** Um único script que simula a física (posição, inércia, temperatura) de múltiplos caminhões. Ele recebe comandos de atuadores e publica dados de sensores com ruído via MQTT.

3.  **Gestão da Mina (Python):** Uma interface gráfica (GUI) que visualiza a posição de todos os caminhões em um mapa. Permite selecionar um caminhão por ID e enviar comandos de navegação (Go-To) ou injetar falhas para fins de teste.

## 2. Comunicação

A comunicação é dividida em duas categorias:

### 2.1. Comunicação Interna (C++)

Dentro do núcleo de controle, as threads trocam informações de forma segura usando:

*   **`CircularBuffer`**: Estrutura de dados central para o histórico de sensores. Permite que tarefas de controle leiam o dado mais recente (`read_latest`) sem atrasar tarefas de log, que consomem todos os dados em ordem (`read_fifo`). O acesso é protegido por `std::mutex`.

*   **`ThreadSafeQueue`**: Fila de eventos para comandos do operador e alertas de falha, garantindo que nenhum evento seja perdido.

*   **`std::atomic`**: Variáveis atômicas para flags de estado (ex: `em_modo_automatico`, `em_falha`), permitindo acesso rápido e sem bloqueio.

### 2.2. Comunicação Externa (MQTT)

Os processos comunicam-se através de um broker Mosquitto usando os seguintes tópicos:

*   `simulacao/{ID}/sensores`: (Simulador -> C++) Envia dados brutos de sensores para um caminhão específico.
*   `caminhao/{ID}/comando`: (C++ -> Simulador) Envia comandos de atuadores (aceleração, direção) para o simulador.
*   `mina/telemetry`: (C++ -> Gestão) Tópico global onde todos os caminhões publicam seu estado (posição, velocidade) para a GUI.
*   `mina/setpoint`: (Gestão -> C++) Tópico global para enviar comandos de alto nível (Go-To, Rearme) para um caminhão específico, identificado por um campo "id" no payload JSON.
*   `simulacao/falhas`: (Gestão -> Simulador) Tópico para injetar falhas (elétrica, hidráulica) em um caminhão específico no simulador.

## 3. Estrutura de Pastas

```
atr_caminhao_autonomo/
│
├── build/              # Arquivos de compilação (gerados pelo CMake)
├── docker/             # Dockerfile para criar o ambiente de build
├── scripts/            # Scripts Python (gestao_mina.py)
├── src/                # Código fonte do núcleo de controle C++
│   ├── simulador_mina_global.py # Script do simulador
│   ├── main.cpp        # Ponto de entrada, inicialização das threads
│   ├── include/        # Arquivos de cabeçalho (.h)
│   └── core/           # Arquivos de implementação (.cpp)
│
├── CMakeLists.txt      # Arquivo de build principal
└── README.md           # Este arquivo
```
