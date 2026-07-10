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
                              │ wsl make run / File Watcher
┌─────────────────────────────▼─────────────────────────────┐
│               Simulación C++ (WSL2 / POSIX)               │
└───────────────────────────────────────────────────────────┘
```

1. **Simulador C++ (WSL2)**: Corre en un entorno Linux POSIX por el uso de `fork()`, señales y control de procesos. Escribe tanto logs legibles (`.log`) como datos estructurados en tiempo real (`.csv`).
2. **Sidecar Backend (Windows)**: Servidor Node.js que:
   - Controla el ciclo de vida de la simulación ejecutando la compilación y ejecución mediante `wsl make run`.
   - Lee los archivos CSV en caliente y transmite las nuevas líneas en tiempo real mediante **WebSockets**.
3. **React Frontend (Dashboard)**: Una interfaz moderna que divide las métricas en un dashboard general y pestañas independientes detalladas por fase de producción, incluyendo una consola virtual en vivo.

---

## Estructura de Directorios

- `include/` y `src/`: Código fuente C++ de la simulación industrial.
- `logs/`: Directorio de registros generado por la simulación (contiene archivos `.log` y `.csv`).
- `ui/`: Aplicación web completa.
  - `ui/server.js`: Entrypoint slim del servidor.
  - `ui/server/`: Código modular del backend (rutas de Express y servicios de WebSocket, monitoreo de archivos y ejecución de WSL).
  - `ui/src/`: Código fuente de React.
    - `ui/src/context/`: `SimulationContext` (estado centralizado de la simulación, WS y parsing).
    - `ui/src/components/`: Componentes modulares visuales de cada fase, de navegación y consola.
    - `ui/src/pages/`: Vistas independientes del Dashboard y las fases 1 a 4.

---

## Variables de Entorno

**No es necesario configurar ninguna variable de entorno** para que el proyecto funcione con sus valores predeterminados. El servidor arrancará por defecto en el puerto `5000`.

De forma opcional, puedes definir:
- `PORT`: El puerto de red en el que escuchará el servidor backend (ej. `PORT=8080`).

---

## Requisitos Previos

1. **Windows con WSL2**: Tener instalado WSL2 con una distribución de Linux (ej. Ubuntu).
2. **Herramientas de Compilación en WSL2**: Asegúrate de tener `g++` y `make` instalados en tu WSL2 (`sudo apt update && sudo apt install build-essential`).
3. **Node.js en Windows**: Tener Node.js instalado en tu sistema Windows (versión 18 o superior).

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
