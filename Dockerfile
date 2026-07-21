# ─────────────────────────────────────────────────────────────────────────────
# CoreOS-Sim · Dockerfile
# Construye un contenedor Linux self-contained con:
#   - Node.js 20 (servidor Express + compilación de React)
#   - g++ / make  (compilador C++ para la simulación POSIX)
# ─────────────────────────────────────────────────────────────────────────────

# 1. Imagen base: Node 20 sobre Debian Bookworm (Linux nativo, sin WSL)
FROM node:20-bookworm

# 2. Instalar herramientas de compilación C++ (g++, gcc, make)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# 3. Directorio de trabajo principal
WORKDIR /app

# 4. Copiar solo los manifiestos de Node para aprovechar el cache de capas
COPY ui/package*.json ./ui/

# 5. Instalar dependencias de Node dentro del directorio ui/
WORKDIR /app/ui
RUN npm ci --prefer-offline || npm install

# 6. Copiar el código fuente completo (C++, server, React)
WORKDIR /app
COPY . .

# 7. Compilar el Frontend React → genera ui/dist/
WORKDIR /app/ui
RUN npm run build

# 8. Asegurar la existencia del directorio de logs que usará C++
WORKDIR /app
RUN mkdir -p logs build

# 9. Variables de entorno
#    IN_DOCKER=true  → SimulationManager usa 'make run' en lugar de 'wsl make run'
ENV IN_DOCKER=true
ENV PORT=5000

# 10. Exponer el puerto del servidor Express
EXPOSE 5000

# 11. Arrancar el servidor Sidecar (Express + WebSockets)
CMD ["node", "ui/server.js"]
