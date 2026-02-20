# Frontend — Interfaz gráfica de usuario

**Directorio:** `Frontend/`  
**Tecnologías:** ImGui (docking branch) · SDL3 · OpenGL 3.3 · ImGuiFileDialog

## Descripción general

El frontend proporciona una **interfaz gráfica multi-ventana** basada en Dear ImGui. Permite controlar la emulación, visualizar el estado del hardware en tiempo real y cargar programas binarios. Cada componente del hardware tiene su propia ventana de visualización.

## Estructura

```
Frontend/
├── main.cpp                  ← Punto de entrada de la aplicación
├── UpdateChecker.h/cpp       ← Comprobador de actualizaciones (HTTPS)
├── Control/
│   ├── AppState.h            ← Estado global de la aplicación
│   └── Console.h/cpp         ← Búfer de texto de la consola
└── GUI/
    ├── ControlWindow         ← Controles principales
    ├── ConsoleWindow         ← Consola de texto
    ├── RegistersWindow       ← Visor de registros CPU
    ├── LCDWindow             ← Pantalla LCD
    ├── VRAMViewerWindow      ← Visor de VRAM
    ├── SIDViewerWindow       ← Visor del chip SID
    └── UpdatePopup           ← Popup de actualización disponible
```

## Ventanas de la GUI

### ControlWindow — Controles de emulación

Permite al usuario:
- **Cargar un binario** usando el diálogo de archivos (ImGuiFileDialog)
- **Iniciar / Pausar / Reanudar / Detener** la emulación
- **Paso a paso** (`Step`) — ejecuta una instrucción
- Configurar la **velocidad** (IPS — instrucciones por segundo)
- Activar/desactivar el **modo ciclo-exacto**
- Activar/desactivar la **GPU**
- Mostrar la **velocidad actual** (IPS real)

### ConsoleWindow — Consola de texto

Muestra el texto enviado por la ACIA (puerto serie del 65c02). También permite **escribir** texto que se inyecta como entrada al emulador (`InjectKey`).

Características:
- Búfer circular de texto
- Scroll automático al final
- Copia al portapapeles

### RegistersWindow — Visor de registros

Muestra en tiempo real el estado de la CPU:

| Campo | Descripción |
|-------|-------------|
| `PC` | Program Counter (hex) |
| `SP` | Stack Pointer (hex) |
| `A` | Acumulador |
| `X` | Registro X |
| `Y` | Registro Y |
| `N V - B D I Z C` | Flags de estado individuales |
| Ciclos restantes | En modo ciclo-exacto |

### LCDWindow — Pantalla LCD

Visualiza el contenido actual de la pantalla LCD 2×16 con una fuente monoespaciada, simulando una pantalla LCD física.

### VRAMViewerWindow — Visor de VRAM

Renderiza el contenido de la VRAM (100×75 píxeles) como una textura OpenGL a escala. Se actualiza en cada frame del renderizador.

### SIDViewerWindow — Visor del SID

Muestra el estado de los 3 osciladores del chip SID:
- Frecuencia programada de cada voz
- Ancho de pulso
- Estado de la envolvente ADSR (barra de nivel)
- Formas de onda activas (Triangle / Sawtooth / Pulse / Noise)

### UpdatePopup — Actualización

Al iniciar la aplicación, `UpdateChecker` consulta la última versión en GitHub vía HTTPS. Si hay una versión más reciente disponible, muestra un popup con un enlace a la página de releases.

```cpp
UpdateChecker checker;
checker.CheckAsync([](const std::string& latestVersion) {
    // Mostrar popup si latestVersion > currentVersion
});
```

## AppState — Estado global

`Control/AppState.h` define la estructura de estado compartida entre la GUI y la capa de control. Contiene referencias al emulador, flags de estado (pausado, en ejecución, etc.) y la ruta del binario cargado.

## Ciclo de renderizado

El bucle principal (`main.cpp`) sigue el patrón estándar de ImGui + SDL3 + OpenGL:

```
while (running):
    1. Procesar eventos SDL3 (teclado, ratón, cierre)
    2. ImGui::NewFrame()
    3. Renderizar todas las ventanas (ControlWindow, ConsoleWindow, ...)
    4. ImGui::Render()
    5. glClear + ImGui_ImplOpenGL3_RenderDrawData()
    6. SDL_GL_SwapWindow()
```

## Dependencias externas

| Librería | Versión | Uso |
|----------|---------|-----|
| SDL3 | 3.4.x | Ventana, eventos, audio |
| ImGui | docking branch | Interfaz gráfica |
| ImGuiFileDialog | 0.6.4 | Diálogo de apertura de archivos |
| GLAD | 2.0.8 | Cargador de OpenGL |
| nlohmann/json | 3.12.0 | Parseo de JSON (respuesta de GitHub API) |
| cpp-httplib | 0.15.3 | Cliente HTTP/HTTPS para el update checker |
| OpenSSL | — | TLS para las peticiones HTTPS |
