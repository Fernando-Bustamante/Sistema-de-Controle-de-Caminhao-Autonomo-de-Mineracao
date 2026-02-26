cmake_minimum_required(VERSION 3.10)
project(atr_caminhao)

set(CMAKE_CXX_STANDARD 17)

# --- 1. Bibliotecas ---
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Curses REQUIRED)
find_package(Threads REQUIRED)

find_library(PAHO_MQTT_CPP paho-mqttpp3)
find_library(PAHO_MQTT_C paho-mqtt3a)

if(NOT PAHO_MQTT_CPP OR NOT PAHO_MQTT_C)
    message(WARNING "Bibliotecas Paho MQTT não encontradas!")
endif()

# --- 2. Diretórios de Inclusão ---
# Usa CMAKE_SOURCE_DIR para garantir caminhos absolutos
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/src/include")

# --- 3. Encontrar Fontes ---
# Procura recursivamente dentro de src
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

# --- Debug: Mostra o que achou ---
message(STATUS "Procurando fontes em: ${CMAKE_SOURCE_DIR}/src")
foreach(f ${SOURCES})
    message(STATUS "Fonte encontrada: ${f}")
endforeach()

# --- 4. Criar Executável ---
add_executable(atr_caminhao_app ${SOURCES})

# --- 5. Linkagem ---
target_link_libraries(atr_caminhao_app 
    Boost::system 
    Boost::thread 
    ${CURSES_LIBRARIES} 
    Threads::Threads
    paho-mqttpp3
    paho-mqtt3a
)
