# CoreOS-Sim · Simulador Industrial VENALUM

Este proyecto es un simulador multiproceso de una planta siderúrgica de fundición de aluminio (**VENALUM**), escrito en C++ para entornos POSIX, con una interfaz web interactiva desarrollada en React, Vite, Tailwind CSS v4 y un servidor intermedio (Sidecar) en Node.js.

El sistema simula las cuatro fases principales de producción:
1. **Fase 1: Planta de Carbón**: Preparación de ánodos de carbón (mezcladora de coque/brea y horno de cocción).
2. **Fase 2: Logística y Transporte**: Distribución neumática de alúmina desde el silo principal a las tolvas de las celdas.
3. **Fase 3: Celdas de Reducción**: Reducción electrolítica mediante el proceso Hall-Héroult (producción de aluminio líquido a 958 °C).
4. **Fase 4: Reciclaje y Tratamiento de Gases (GTC)**: Captura de gases fluorados nocivos y recirculación de alúmina enriquecida.

---

## Arquitectura del Proyecto

El sistema está diseñado de forma totalmente desacoplada en tres capas principales:

```
┌───────────────────────────────────────────────────────────┐
│              React Frontend (Vite + Tailwind v4)          │
└─────────────────────────────┬─────────────────────────────┘
                              │ WebSockets / HTTP
┌─────────────────────────────▼─────────────────────────────┐
│                 Node.js Sidecar Backend                   │
└─────────────────────────────┬─────────────────────────────┘
                              │ make run (Docker/Linux)  ·  wsl make run (Windows)
┌─────────────────────────────▼─────────────────────────────┐
│         Simulación C++ (Docker / WSL2 / POSIX)            │
└───────────────────────────────────────────────────────────┘
```

1. **Simulador C++ (Docker o WSL2)**: Corre en un entorno Linux POSIX por el uso de `fork()`, señales y control de procesos. Escribe tanto logs legibles (`.log`) como datos estructurados en tiempo real (`.csv`). Puede ejecutarse de forma nativa dentro de un contenedor Docker **o** a través de WSL2 en Windows.
2. **Sidecar Backend (Node.js)**: Servidor Node.js que:
   - Detecta automáticamente el entorno de ejecución: usa `make run` en Linux/Docker o `wsl make run` en Windows.
   - Lee los archivos CSV en caliente y transmite las nuevas líneas en tiempo real mediante **WebSockets**.
3. **React Frontend (Dashboard)**: Una interfaz moderna que divide las métricas en un dashboard general y pestañas independientes detalladas por fase de producción, incluyendo una consola virtual en vivo.

---

## Estructura de Directorios

- `include/` y `src/`: Código fuente C++ de la simulación industrial.
- `logs/`: Directorio de registros generado por la simulación (contiene archivos `.log` y `.csv`).
- `Dockerfile`: Imagen Docker del proyecto (Node.js + `build-essential` para compilar C++ nativamente).
- `docker-compose.yml`: Orquestación del contenedor con mapeo de puertos y volumen de logs.
- `.dockerignore`: Exclusiones de la imagen (node_modules, binarios compilados, archivos de IDE).
- `ui/`: Aplicación web completa.
  - `ui/server.js`: Entrypoint slim del servidor.
  - `ui/server/`: Código modular del backend (rutas de Express y servicios de WebSocket, monitoreo de archivos y ejecución del simulador).
  - `ui/src/`: Código fuente de React.
    - `ui/src/context/`: `SimulationContext` (estado centralizado de la simulación, WS y parsing).
    - `ui/src/components/`: Componentes modulares visuales de cada fase, de navegación y consola.
    - `ui/src/pages/`: Vistas independientes del Dashboard y las fases 1 a 4.

---

## Variables de Entorno

**No es necesario configurar ninguna variable de entorno** para que el proyecto funcione con sus valores predeterminados. El servidor arrancará por defecto en el puerto `5000`.

De forma opcional, puedes definir:
- `PORT`: El puerto de red en el que escuchará el servidor backend (ej. `PORT=8080`).
- `IN_DOCKER`: Usada internamente por el servidor. Cuando su valor es `true`, el `SimulationManager` ejecuta `make run` de forma nativa en lugar de `wsl make run`. El `docker-compose.yml` la establece automáticamente; **no es necesario configurarla a mano**.

---

## Requisitos Previos

### Opción A / B — Ejecución local con WSL2 (Windows)

1. **Windows con WSL2**: Tener instalado WSL2 con una distribución de Linux (ej. Ubuntu).
2. **Herramientas de Compilación en WSL2**: Asegúrate de tener `g++` y `make` instalados en tu WSL2:
   ```bash
   wsl sudo apt update
   wsl sudo apt install -y build-essential
   ```
3. **Node.js en Windows**: Tener Node.js instalado en tu sistema Windows (versión 18 o superior).

### Opción C — Ejecución con Docker (Recomendado · Multiplataforma)

1. **Docker Desktop**: Instalar [Docker Desktop](https://www.docker.com/products/docker-desktop/) en Windows, macOS o Linux.
   - En Windows, Docker Desktop incluye su propio motor Linux; **no se requiere WSL2 configurado manualmente** ni tener `g++` instalado.
2. **Sin dependencias adicionales**: El contenedor instala automáticamente `g++`, `make`, `Node.js` y las dependencias de npm durante la construcción.

---

## Cómo Ejecutar el Proyecto

Sigue estos pasos para arrancar todo el sistema:

### Paso 1: Instalar dependencias del Frontend
Abre tu terminal de Windows (PowerShell o CMD) en la carpeta del proyecto y navega al directorio `ui/` para instalar las dependencias de Node:
```bash
cd ui
npm install
```
*(Si usas PowerShell y obtienes un error de políticas de ejecución al ejecutar comandos de npm, utiliza `npm.cmd install`)*.

### Paso 2: Ejecutar el Proyecto (Modos Disponibles)

#### Opción A: Modo Producción (Recomendado y más simple)
Este modo compila la aplicación de React y la sirve directamente desde el servidor Express en un único puerto (`5000`):

1. **Compilar la interfaz React**:
   ```bash
   npm run build
   ```
2. **Iniciar el servidor**:
   ```bash
   npm run start
   ```
3. Abre tu navegador web en **[http://localhost:5000](http://localhost:5000)**. Presiona **"Iniciar Simulación"** para ver correr el simulador y las métricas fluir en tiempo real.

---

#### Opción B: Modo Desarrollo (Para realizar cambios de código)
Este modo te permite ver cambios de interfaz en tiempo real sin tener que compilar cada vez (Hot Reloading):

1. **Iniciar el servidor backend (Sidecar)**:
   En una terminal, entra en la carpeta `ui/` y ejecuta:
   ```bash
   node server.js
   ```
   *(Escuchará en el puerto `5000`)*.

2. **Iniciar el servidor de desarrollo de Vite**:
   Abre una segunda terminal en la carpeta `ui/` y ejecuta:
   ```bash
   npm run dev
   ```
   *(Este servidor escuchará en el puerto `5173` y redirigirá de forma automática las peticiones de API y WebSockets al puerto `5000`)*.

3. Abre tu navegador en **[http://localhost:5173](http://localhost:5173)**.

---

#### Opción C: Docker (Multiplataforma · Sin instalar g++ ni WSL2)

Este modo empaqueta toda la aplicación (C++, Node.js y React) en un único contenedor Linux portable. Funciona en Windows, macOS y Linux sin configuración adicional.

1. **Construir y levantar el contenedor** (solo necesitas tener Docker Desktop abierto):
   ```bash
   docker compose up --build
   ```
   Docker descargará la imagen base, instalará `build-essential`, compilará React y arrancará el servidor Express.

2. Abre tu navegador web en **[http://localhost:5000](http://localhost:5000)**. Presiona **"Iniciar Simulación"** para ejecutar el simulador C++ de forma nativa dentro del contenedor y ver las métricas en tiempo real.

3. Para detener el contenedor:
   ```bash
   docker compose down
   ```

> **Nota sobre los logs**: el directorio `logs/` del proyecto se monta como un volumen dentro del contenedor, por lo que los archivos `.csv` y `.log` generados por la simulación son accesibles directamente desde tu sistema de archivos local.

> **Reconstruir después de cambios**: si modificas el código C++ (`src/`), el servidor (`ui/server/`) o instalas nuevas dependencias de npm, ejecuta `docker compose up --build` para reconstruir la imagen.
