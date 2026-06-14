# Cargamos el compilador y las banderas
CXX = g++
CXXFLAGS = -std=c++17 -pthread -Iinclude

# Buscamos automáticamente todos los archivos .cpp en src y sus subcarpetas
SRCS = src/main.cpp $(wildcard src/industrial/*.cpp)

# Definimos el nombre del ejecutable y su carpeta
TARGET = build/main

# Regla principal: compila todo
all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# Regla para limpiar archivos temporales
clean:
	rm -f $(TARGET)

# Para ejecutar rápidamente
run: all
	./$(TARGET)
