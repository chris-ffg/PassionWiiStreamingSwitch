# 🌌 PassionWiiStreaming Nintendo Switch Edition

**Cliente y reproductor nativo de streaming, películas, series, animes y doramas para Nintendo Switch (Horizon OS)**

---

## ✨ Características Principales

- 🎬 **Catálogo Integrado de PassionWii**:
  - Conexión preconfigurada y optimizada con el catálogo oficial de **PassionWiiStreaming**:
    `https://listas-iptv-wii.github.io/dango/passionwii.m3u`
  - Exploración por estanterías temáticas (Novedades en el Streaming, Películas, Series, Animes, Doramas).
  - Descarga y renderizado en alta resolución de carátulas y pósters oficiales en caché local (`sdmc:/switch/PassionWiiStreaming/cache`).
  - Navegación fluida por carpetas anidadas con soporte táctil y controles físicos.

- ⚡ **Rendimiento de Video Acelerado por Hardware**:
  - Motor multimedia basado en **libmpv** y **FFmpeg** con decodificación acelerada por hardware **NVDEC** sobre el procesador NVIDIA Tegra X1.
  - Reproducción continua sin cortes ni congelamientos.
  - Manejo inteligente del estado de suspensión de Horizon OS: la consola no se suspende mientras reproduces contenido.

- 🎨 **Interfaz y Personalización Temática**:
  - Fondo galáctico con estética cósmica oficial de PassionWii.
  - Ícono nativo 256x256 optimizado para el Menú Home de Switch, forwarders (Sphaira) y hbmenu.
  - Menú lateral moderno con accesos directos rápidos a Inicio, Streaming, Favoritos, Búsqueda y Ajustes.

- 📺 **Reproducción Local y USB**:
  - Soporte para reproducir videos locales desde la tarjeta microSD (`sdmc:/`) o unidades USB externas (`ums0:/` a `ums3:/`).
  - Formatos soportados: `.mp4`, `.mkv`, `.avi`, `.mov`, `.wmv`, `.flv`, `.webm`, `.ts`, `.m2ts`.

---

## 🎮 Controles

| Botón | Acción en Navegador | Acción en Reproductor |
|---|---|---|
| **D-Pad / Palanca Izquierda** | Navegar por carátulas y categorías | Adelantar / Retroceder video |
| **A** | Entrar en carpeta / Reproducir contenido | Pausar / Reanudar |
| **B** | Volver atrás / Subir de carpeta | Salir del reproductor |
| **X** | Añadir / Quitar de Favoritos | Cambiar pista de Audio |
| **Y** | Búsqueda por teclado Horizon OS | Activar / Cambiar Subtítulos |
| **L / R** | Cambiar rápidamente de categoría | Salto de 1 minuto |
| **+ (Plus)** | Salir de la aplicación | Mostrar / Ocultar controles |
| **Pantalla táctil** | Toque directo en carátulas o botones | Controles táctiles completos |

---

## 🛠️ Compilación

El proyecto utiliza el toolchain de **devkitPro** (`devkitA64`) junto con `libnx` y `switch-sdl2`.

Se incluye integración continua automática con **GitHub Actions** que genera los archivos `.nro` y `.nacp` en cada commit.
