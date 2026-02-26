import pygame
import paho.mqtt.client as mqtt
import json
import sys
import math

BROKER = "localhost"
PORT = 1883
TOPIC_TELEMETRY = "mina/telemetry"
TOPIC_SETPOINT = "mina/setpoint"
TOPIC_FALHAS = "simulacao/falhas" # <-- Novo tópico

WIDTH, HEIGHT = 900, 700
OFFSET_X, OFFSET_Y = 50, 130
MINA_W, MINA_H = 800, 520

# Cores
BLACK, WHITE, DARK_GRAY = (0,0,0), (255,255,255), (30,30,30)
GREEN, BLUE, RED = (0,200,0), (50,100,255), (255,0,0)
ORANGE, GRAY, YELLOW = (255,165,0), (100,100,100), (255,255,0)

trucks = {} 
target_visual = {}      
selected_truck_id = 1   
input_text = ""         

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        t_id = data.get("id")
        if t_id is not None:
            trucks[t_id] = {
                "x": data.get("x", 0), "y": data.get("y", 0),
                "v": data.get("v", 0), "ang": data.get("ang", 0),
                "st": data.get("st", "MANUAL")
            }
    except: pass

client = mqtt.Client()
client.on_message = on_message
try:
    client.connect(BROKER, PORT, 60)
    client.subscribe(TOPIC_TELEMETRY)
    client.loop_start()
except:
    sys.exit()

pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Gestão da Mina - Controle Central")
clock = pygame.time.Clock()
font = pygame.font.SysFont("Arial", 16)
font_large = pygame.font.SysFont("Arial", 24, bold=True)
font_input = pygame.font.SysFont("Arial", 32, bold=True)

running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        
        elif event.type == pygame.KEYDOWN:
            # --- COMANDOS DE FALHA E REARME ---
            if event.key == pygame.K_e: # Falha Elétrica
                msg = {"id": selected_truck_id, "cmd": "FALHA_ELET"}
                client.publish(TOPIC_FALHAS, json.dumps(msg))
                print(f"Enviada Falha Elétrica para ID {selected_truck_id}")

            elif event.key == pygame.K_h: # Falha Hidráulica
                msg = {"id": selected_truck_id, "cmd": "FALHA_HIDR"}
                client.publish(TOPIC_FALHAS, json.dumps(msg))
                print(f"Enviada Falha Hidráulica para ID {selected_truck_id}")

            elif event.key == pygame.K_r: # Rearme Completo
                # 1. Limpa simulador
                msg_sim = {"id": selected_truck_id, "cmd": "LIMPAR"}
                client.publish(TOPIC_FALHAS, json.dumps(msg_sim))
                # 2. Avisa C++ para rearmar lógica
                msg_cpp = {"id": selected_truck_id, "cmd": "REARME"}
                client.publish(TOPIC_SETPOINT, json.dumps(msg_cpp))
                print(f"Enviado REARME para ID {selected_truck_id}")

            # --- DIGITAÇÃO ---
            elif event.key == pygame.K_RETURN or event.key == pygame.K_KP_ENTER:
                if input_text.isdigit():
                    val = int(input_text)
                    if val > 0: selected_truck_id = val
                input_text = ""
            elif event.key == pygame.K_BACKSPACE: input_text = input_text[:-1]
            elif event.unicode.isdigit(): input_text += event.unicode

        elif event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1: 
                mx, my = pygame.mouse.get_pos()
                if (OFFSET_X <= mx <= OFFSET_X + MINA_W) and (OFFSET_Y <= my <= OFFSET_Y + MINA_H):
                    real_x, real_y = mx - OFFSET_X, my - OFFSET_Y
                    target_visual[selected_truck_id] = (mx, my)
                    msg = {"id": selected_truck_id, "target_x": real_x, "target_y": real_y, "cmd": "GOTO"}
                    client.publish(TOPIC_SETPOINT, json.dumps(msg, separators=(',', ':')))

    screen.fill(DARK_GRAY)
    
    # Status
    header_color = RED if (selected_truck_id in trucks and trucks[selected_truck_id]['st'] == "DEFEITO") else ORANGE
    screen.blit(font_large.render(f"CONTROLANDO: CAMINHÃO {selected_truck_id}", True, header_color), (20, 15))
    screen.blit(font.render("ID+ENTER:Trocar | Clique:GOTO | 'E':Eletrica 'H':Hidraulica 'R':Rearme", True, WHITE), (20, 50))

    pygame.draw.rect(screen, BLACK, (20, 80, 100, 40))
    pygame.draw.rect(screen, WHITE, (20, 80, 100, 40), 2)
    screen.blit(font_input.render(input_text if input_text else "ID", True, YELLOW if input_text else GRAY), (25, 85))

    pygame.draw.rect(screen, WHITE, (OFFSET_X, OFFSET_Y, MINA_W, MINA_H))
    pygame.draw.rect(screen, BLUE, (OFFSET_X, OFFSET_Y, MINA_W, MINA_H), 3)

    for tid, pos in target_visual.items():
        color = ORANGE if tid == selected_truck_id else GRAY
        pygame.draw.line(screen, color, (pos[0]-8, pos[1]-8), (pos[0]+8, pos[1]+8), 2)
        pygame.draw.line(screen, color, (pos[0]-8, pos[1]+8), (pos[0]+8, pos[1]-8), 2)

    for tid, data in trucks.items():
        px, py = int(data["x"]) + OFFSET_X, int(data["y"]) + OFFSET_Y
        col = GREEN
        if data['st'] == "AUTOMATICO": col = BLUE
        if data['st'] == "DEFEITO": col = RED
        if tid == selected_truck_id: pygame.draw.circle(screen, ORANGE, (px, py), 22, 3) 
        pygame.draw.circle(screen, col, (px, py), 18) 
        rad = math.radians(data["ang"])
        pygame.draw.line(screen, BLACK, (px, py), (px + 25*math.cos(rad), py + 25*math.sin(rad)), 3)
        screen.blit(font.render(str(tid), True, BLACK), (px-5, py-10))

    pygame.display.flip()
    clock.tick(30)
