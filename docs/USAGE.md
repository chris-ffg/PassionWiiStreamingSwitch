# 📖 DangoPlayer Switch — Manual de Usuario

Guía completa para instalar, configurar y disfrutar de **DangoPlayer Switch** en consolas Nintendo Switch con custom firmware **Atmosphère**.

---

## ⚠️ Requisito Fundamental: Title Override Mode (Full RAM)

Para reproducir videos en alta definición (1080p / 720p) con decodificación por hardware o streams IPTV en vivo, la aplicación **debe ejecutarse con acceso a la memoria RAM completa (3.2 GB)**.

> [!CAUTION]
> **No abras DangoPlayer desde el Álbum (Applet Mode).**
> En Applet Mode, el sistema operativo Horizon reserva solo ~400 MB de memoria, lo cual provocará congelamientos o falta de memoria al decodificar video de alta tasa de bits.

### Cómo iniciar en Title Override Mode:
1. En el menú principal de la consola (HOME Menu), sitúate sobre **cualquier juego instalado**.
2. **Mantén presionado el botón R** del Joy-Con derecho.
3. Sin soltar el botón **R**, presiona **A** para abrir el juego.
4. Se abrirá el **Homebrew Menu** en modo *Full RAM*.
5. Selecciona **DangoPlayer Switch** de la lista.

*(Opcional)*: Puedes configurar Atmosphère para que el botón de override esté invertido o asignado por defecto editando `sdmc:/atmosphere/config/override_config.ini`.

---

## 📁 Instalación en la Tarjeta SD

Copia el archivo `DangoPlayerSwitch.nro` en tu tarjeta microSD en la siguiente ruta:

```
sdmc:/switch/DangoPlayerSwitch/DangoPlayerSwitch.nro
```

Estructura de directorios creada automáticamente en la SD:
```
sdmc:/switch/DangoPlayer/
├── config.txt           # Configuración de volumen, reanudación y temas
├── history.txt          # Historial de reproducción reciente
├── favorites.txt        # Canales y videos marcados como favoritos
├── playlists.txt        # Índice de listas IPTV registradas
├── resume_cache.txt     # Puntos exactos de reanudación de cada video
├── cache/               # Archivos temporales y listas descargadas
└── fonts/               # Fuentes personalizadas (opcional)
```

---

## 🎬 Reproducción de Archivos Locales

1. En el menú principal, selecciona **Archivos Locales**.
2. El explorador abrirá por defecto la raíz de tu tarjeta microSD (`sdmc:/`).
3. Navega por las carpetas hasta encontrar tus videos (`.mp4`, `.mkv`, `.avi`, `.ts`, `.webm`, `.flv`, etc.).
4. Presiona **A** o toca la pantalla para iniciar la reproducción.
5. **Subtítulos automáticos**: Si colocas un archivo `.srt` o `.ass` con el mismo nombre que el archivo de video en la misma carpeta, el reproductor lo cargará y sincronizará de forma automática.
6. **Almacenamiento USB**: Si conectas una unidad USB (formateada en FAT32, exFAT o NTFS) al dock de la consola o al puerto USB-C mediante un adaptador OTG, presiona el botón **Y** para alternar inmediatamente entre la tarjeta SD y el USB (`ums0:/`).

---

## 📺 Configuración y Uso de IPTV

DangoPlayer Switch te permite cargar y organizar listas de canales de televisión por streaming mediante dos métodos:

### Método 1: Descargar lista desde una URL remota
1. En el menú principal, selecciona **IPTV**.
2. Selecciona la opción **+ Añadir Lista por URL (Teclado)**.
3. Se abrirá el teclado nativo de Nintendo Switch en pantalla.
4. Escribe la dirección web de tu lista (por ejemplo: `http://mi-proveedor.com/lista.m3u`).
5. Presiona **+ (Aceptar)** en el teclado. La lista se descargará, parseará y guardará en tu lista de listas frecuentes.

### Método 2: Cargar lista desde la tarjeta SD
1. Copia tus archivos `.m3u`, `.m3u8` o `.pls` en cualquier carpeta de tu tarjeta SD.
2. Puedes abrir la lista directamente desde el explorador de **Archivos Locales** o elegir **Cargar lista desde la tarjeta SD** en la sección IPTV.

### Navegación de canales:
- **Cambio de categorías**: Usa los gatillos superiores **L / R** o toca la píldora de categoría en la barra superior para alternar entre grupos (por ejemplo: *Deportes*, *Noticias*, *Cine*, etc.).
- **Búsqueda**: Presiona el botón **Y** para buscar cualquier canal por su nombre.
- **Favoritos**: Presiona el gatillo **ZL** sobre cualquier canal para añadirlo a tu lista de favoritos.

---

## 🔄 Reanudación Automática (Resume Playback)

DangoPlayer Switch recuerda automáticamente la posición en la que dejaste cualquier video o película.
- Si sales de la aplicación o detienes la reproducción pasados los primeros 15 segundos, la próxima vez que abras el archivo continuará exactamente donde lo dejaste.
- Puedes activar o desactivar esta función en la pantalla de **Configuración**.
