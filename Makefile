# Cargamos el compilador y las banderas del estándar C++17
CXX = g++
CXXFLAGS = -std=c++17 -pthread -Iinclude

# Buscamos automáticamente todos los archivos .cpp en src y sus subcarpetas
SRCS = src/main.cpp $(wildcard src/industrial/*.cpp)

# Definimos el nombre del ejecutable y su carpeta
TARGET = build/main

# Regla principal: compila todo el entorno
all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# Asegura la existencia del directorio físico de diagnóstico
init:
	@mkdir -p logs

# Limpieza radical de binarios y mapas de logs residuales
clean:
	rm -f $(TARGET)
	rm -rf logs/*.log logs/*.csv

# Ejecución estándar garantizando la purga de registros previos
run: all init
	@echo "Purgando registros de simulaciones anteriores..."
	rm -f logs/*.log logs/*.csv
	@echo "Iniciando CoreOS-Sim de forma silenciosa..."
	./$(TARGET)

# Despliegue automatizado multipanel en terminales independientes (Tmux)
monitor: all init
	@echo "Purgando registros de simulaciones anteriores..."
	rm -f logs/*.log logs/*.csv
	@tmux kill-session -t coreos_monitor > /dev/null 2>&1 || true
	@echo "Lanzando el Kernel Administrador en segundo plano sin fugas..."
	./$(TARGET) > logs/kernel.log 2>&1 &
	@sleep 1
	@echo "Levantando la suite de monitorización industrial..."
	tmux new-session -d -s coreos_monitor 'tail -f logs/fase3_reduccion.log'
	tmux split-window -h 'tail -f logs/fase4_gtc.log'
	tmux split-window -v 'tail -f logs/kernel.log'
	tmux attach-session -t coreos_monitor
