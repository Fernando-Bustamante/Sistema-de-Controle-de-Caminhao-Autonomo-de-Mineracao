#include "../../src/include/tasks.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <mqtt/async_client.h>
#include <string>
#include <cstring>
#include <chrono>

const std::string PUB_SERVER("tcp://localhost:1883");

// Auxiliar para extrair inteiros
int extrair_valor_json(const std::string& json, const std::string& chave) {
    size_t pos_chave = json.find("\"" + chave + "\"");
    if (pos_chave == std::string::npos) return -999999;
    size_t pos_dois_pontos = json.find(":", pos_chave);
    if (pos_dois_pontos == std::string::npos) return -999999;
    try { return std::stoi(json.substr(pos_dois_pontos + 1)); } catch (...) { return -999999; }
}

// Auxiliar para extrair string (simplificado)
std::string extrair_string_json(const std::string& json, const std::string& chave) {
    size_t pos = json.find("\"" + chave + "\"");
    if (pos == std::string::npos) return "";
    size_t start_quote = json.find("\"", pos + chave.length() + 2);
    if (start_quote == std::string::npos) return "";
    size_t end_quote = json.find("\"", start_quote + 1);
    if (end_quote == std::string::npos) return "";
    return json.substr(start_quote + 1, end_quote - start_quote - 1);
}

class MqttCallback : public virtual mqtt::callback {
    SharedResources& res;
public:
    MqttCallback(SharedResources& r) : res(r) {}
    void connection_lost(const std::string& cause) override {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string payload = msg->get_payload_str();
        std::string topic = msg->get_topic();

        if (topic == "mina/setpoint") {
            int id_recebido = extrair_valor_json(payload, "id");
            std::string cmd_str = extrair_string_json(payload, "cmd");

            if (id_recebido == res.caminhao_id) {
                // COMANDO GOTO
                if (cmd_str == "GOTO") {
                    int tx = extrair_valor_json(payload, "target_x");
                    int ty = extrair_valor_json(payload, "target_y");
                    if (tx != -999999 && ty != -999999) {
                        res.target_pos_x.store(tx);
                        res.target_pos_y.store(ty);
                        res.navegacao_ativa.store(true);
                        res.em_modo_automatico.store(true);
                        // Cria evento GOTO para notificar a lógica
                        ComandoEvent evt; evt.tipo = ComandoEvent::GOTO_WAYPOINT;
                        res.fila_comandos.push(evt);
                        std::cout << "[MQTT] GOTO recebido: (" << tx << "," << ty << ")" << std::endl;
                    }
                }
                // COMANDO REARME
                else if (cmd_str == "REARME") {
                    ComandoEvent evt;
                    evt.tipo = ComandoEvent::REARME;
                    res.fila_comandos.push(evt);
                    std::cout << "[MQTT] Comando REARME recebido!" << std::endl;
                }
            }
        }
    }
};

void task_coletor_dados(SharedResources& res) {
    std::string log_name = "log_caminhao_" + std::to_string(res.caminhao_id) + ".csv";
    std::ofstream logfile(log_name, std::ios::app);
    if (logfile.tellp() == 0) logfile << "timestamp,id,estado,pos_x,pos_y,evento" << std::endl;

    std::string topic_cmd = "caminhao/" + std::to_string(res.caminhao_id) + "/comando";
    std::string topic_telem = "mina/telemetry";
    std::string client_id = "Caminhao_" + std::to_string(res.caminhao_id) + "_Telemetry";

    mqtt::async_client client(PUB_SERVER, client_id);
    MqttCallback cb(res);
    client.set_callback(cb);
    mqtt::connect_options connOpts; connOpts.set_clean_session(true);
    
    try { 
        client.connect(connOpts)->wait(); 
        client.subscribe("mina/setpoint", 0);
    } catch(...) {}

    while (res.sistema_ativo.load()) {
        SensorSample s = res.buffer.read_fifo();
        long long ts = std::chrono::duration_cast<std::chrono::milliseconds>(s.ts.time_since_epoch()).count();

        std::string estado = "MANUAL";
        if (res.em_falha.load()) estado = "DEFEITO";
        else if (res.em_modo_automatico.load()) estado = "AUTOMATICO";
        
        std::string evento = "INFO";
        if (s.temperatura > 110) evento = "ALERTA_TEMP";
        if (s.falha_eletrica) evento = "FALHA_ELET";
        if (s.falha_hidraulica) evento = "FALHA_HIDR";

        logfile << ts << "," << res.caminhao_id << "," << estado << "," 
                << s.pos_x << "," << s.pos_y << "," << evento << std::endl;

        if (client.is_connected()) {
            std::stringstream ss_cmd;
            ss_cmd << res.comando_aceleracao_aplicado.load() << "," << res.comando_direcao_aplicado.load();
            std::string payload_cmd = ss_cmd.str();
            try { 
                client.publish(topic_cmd, payload_cmd.c_str(), payload_cmd.size(), 0, false); 
                std::stringstream ss_telem;
                ss_telem << "{\"id\":" << res.caminhao_id 
                         << ",\"x\":" << s.pos_x << ",\"y\":" << s.pos_y 
                         << ",\"v\":" << s.velocidade << ",\"ang\":" << s.angulo 
                         << ",\"st\":\"" << estado << "\"}";
                client.publish(topic_telem, ss_telem.str().c_str(), ss_telem.str().size(), 0, false);
            } catch(...) {}
        }
    }
    if (client.is_connected()) client.disconnect()->wait();
}
