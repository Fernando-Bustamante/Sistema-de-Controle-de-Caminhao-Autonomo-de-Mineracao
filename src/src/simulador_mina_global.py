import paho.mqtt.client as mqtt
import time
import math
import re
import threading
import sys
import json
import random

BROKER = "localhost"
NUM_CAMINHOES = 10

class TruckPhysics:
    def __init__(self, truck_id, start_x, start_y):
        self.id = truck_id
        self.x = float(start_x)
        self.y = float(start_y)
        self.velocidade = 0.0
        self.angulo = 0.0
        self.temperatura = 30.0
        self.cmd_acel = 0.0
        self.cmd_dir = 0.0
        self.last_update = time.time()
        self.falha_eletrica = 0
        self.falha_hidraulica = 0

    def update(self, dt):
        fator_potencia = 20.0
        atrito = -0.5 * self.velocidade
        delta_v = (self.cmd_acel * 0.01 * fator_potencia + atrito) * dt
        self.velocidade += delta_v

        if abs(self.velocidade) > 0.1:
            delta_angle = (self.cmd_dir * 0.5) * dt
            self.angulo += delta_angle
        self.angulo = self.angulo % 360

        vel_ms = self.velocidade / 3.6
        rad = math.radians(self.angulo)
        self.x += math.cos(rad) * vel_ms * dt
        self.y += math.sin(rad) * vel_ms * dt
        
        target_temp = 85 + (abs(self.velocidade) * 0.25) + (abs(self.cmd_acel) * 0.1)
        if self.falha_eletrica: target_temp += 60 # Sobe rápido
        self.temperatura += (target_temp - self.temperatura) * 0.01 * dt

    def get_sensor_str(self):

        # Adiciona rúido aos sensores
        ruido_x = random.uniform(-1, 1)
        ruido_y = random.uniform(-1, 1)
        ruido_ang = random.uniform(-1, 1)
        ruido_vel = random.uniform(-0.5, 0.5)
        ruido_temp = random.uniform(-0.2, 0.2)

        x_ruidoso = self.x + ruido_x
        y_ruidoso = self.y + ruido_y
        ang_ruidoso = self.angulo + ruido_ang
        vel_ruidoso = self.velocidade + ruido_vel
        temp_ruidoso = self.temperatura + ruido_temp

        return f"{int(x_ruidoso)},{int(y_ruidoso)},{int(ang_ruidoso)},{int(vel_ruidoso)},{int(temp_ruidoso)},{self.falha_eletrica},{self.falha_hidraulica}"

print(f"--- MINA GLOBAL INICIADA ---")

frota = {}
for i in range(1, NUM_CAMINHOES + 1):
    frota[i] = TruckPhysics(truck_id=i, start_x=0, start_y=(i-1)*50)

def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode()
    
    # 1. Comandos de Movimento (C++ -> Simulador)
    if "comando" in topic:
        match = re.search(r'caminhao/(\d+)/comando', topic)
        if match:
            t_id = int(match.group(1))
            if t_id in frota:
                try:
                    partes = payload.split(',')
                    frota[t_id].cmd_acel = float(partes[0])
                    frota[t_id].cmd_dir = float(partes[1])
                except: pass
    
    # 2. Injeção de Falhas (Interface -> Simulador)
    elif topic == "simulacao/falhas":
        try:
            data = json.loads(payload)
            t_id = data.get("id")
            cmd = data.get("cmd")
            
            if t_id in frota:
                if cmd == "FALHA_ELET":
                    frota[t_id].falha_eletrica = 1
                    print(f"-> FALHA ELETRICA INJETADA NO ID {t_id}")
                elif cmd == "FALHA_HIDR":
                    frota[t_id].falha_hidraulica = 1
                    print(f"-> FALHA HIDRAULICA INJETADA NO ID {t_id}")
                elif cmd == "LIMPAR":
                    frota[t_id].falha_eletrica = 0
                    frota[t_id].falha_hidraulica = 0
                    print(f"-> FALHAS LIMPAS NO ID {t_id}")
        except: pass

client = mqtt.Client("Simulador_Mina_Global")
client.on_message = on_message
client.connect(BROKER, 1883, 60)
client.subscribe("caminhao/+/comando")
client.subscribe("simulacao/falhas") # <-- Novo tópico
client.loop_start()

try:
    while True:
        now = time.time()
        for t_id, caminhao in frota.items():
            dt = now - caminhao.last_update
            caminhao.last_update = now
            caminhao.update(dt)
            topic_pub = f"simulacao/{t_id}/sensores"
            client.publish(topic_pub, caminhao.get_sensor_str())
        time.sleep(0.05)
except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()
