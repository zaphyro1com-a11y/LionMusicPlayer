# Lion Music Player (LMPlayer)

Reproductor de música de consola en C++ para Windows. Reproduce `.mp3` directamente
con **miniaudio**, y `.m4a` decodificándolo primero con **FFmpeg (libavformat /
libavcodec / libswresample)** a PCM crudo, que luego se le entrega a miniaudio.

## ¿Por qué dos caminos distintos para mp3 y m4a?

miniaudio sabe leer mp3 nativamente (trae su propio decodificador integrado), pero
no sabe leer m4a/AAC. Para esos archivos hace falta un decodificador externo:
FFmpeg. El resultado de FFmpeg es un montón de muestras de audio en crudo (PCM),
que luego se le "inyectan" a miniaudio para que las reproduzca por la tarjeta de
sonido — miniaudio no sabe ni le importa que ese PCM vino originalmente de un m4a.

```
archivo.mp3  ──────────────────────────────► miniaudio (ma_engine/ma_sound) ──► altavoces
archivo.m4a  ──► AudioDecoder (FFmpeg) ──► PCM ──► ring buffer ──► miniaudio (ma_device) ──► altavoces
```

## Estructura de archivos

| Archivo             | Responsabilidad                                                        |
|----------------------|-------------------------------------------------------------------------|
| `main.cpp`           | Punto de entrada, menú de consola                                       |
| `LMPlayer.h/.cpp`    | Lógica del reproductor: listar canciones, elegir aleatoria, reproducir  |
| `audioDecoder.h/.cpp`| Envoltorio (wrapper) de FFmpeg: abre un archivo y entrega frames PCM    |

---

## `audioDecoder.h` / `audioDecoder.cpp`

Esta clase es **solo un traductor**: recibe una ruta de archivo comprimido
(m4a/AAC, aunque en teoría FFmpeg soporta muchos más formatos) y entrega, frame a
frame, audio PCM sin comprimir en formato `int16` (S16), que es lo que miniaudio
sabe reproducir.

### `struct PCMFrame`

El "paquete" de datos que entrega el decodificador cada vez que se le pide un
frame:

- `data`: el buffer de bytes con las muestras de audio ya en PCM S16.
- `sampleRate`: frecuencia de muestreo del archivo (ej. 44100 Hz).
- `channels`: número de canales (1 = mono, 2 = estéreo).
- `format`: siempre `AV_SAMPLE_FMT_S16` en este proyecto.

### `class AudioDecoder`

**`AudioDecoder()`** — constructor. Reserva memoria para un `AVFrame` (donde
FFmpeg deja el frame decodificado) y un `AVPacket` (donde deja un "paquete" de
datos comprimidos leído del archivo, antes de decodificar).

**`~AudioDecoder()`** — destructor. Llama a `close()` y libera el frame/packet.

**`bool openFile(const std::string& path)`** — abre el archivo y prepara todo
para poder empezar a decodificar. Pasos, en orden:

1. `avformat_open_input`: abre el contenedor del archivo (el ".m4a" en sí, que
   puede tener video, audio, subtítulos, carátula, etc.).
2. `avformat_find_stream_info`: FFmpeg analiza el archivo para saber qué streams
   (pistas) tiene y con qué codec está cada una.
3. `av_find_best_stream`: de todos los streams del archivo, elige el de audio
   (ignora video/carátula si los hay).
4. Busca el decodificador adecuado para el codec de ese stream
   (`avcodec_find_decoder`) y crea un contexto de decodificación
   (`avcodec_alloc_context3` + `avcodec_parameters_to_context`).
5. `avcodec_open2`: abre/inicializa el decodificador.
6. `swr_alloc_set_opts2` + `swr_init`: prepara el **resampler** (`SwrContext`),
   que convierte el formato de muestra original del archivo (que puede ser float,
   planar, etc.) a `int16` entrelazado, que es lo que miniaudio espera. **Importante:**
   `swr_alloc_set_opts2` solo *configura* las opciones; sin el `swr_init()`
   posterior el contexto nunca queda realmente listo y `swr_convert` falla en
   silencio (este fue justo el bug que arreglamos).

**`bool getNextFrame(PCMFrame& frame)`** — decodifica y entrega **un** frame de
audio PCM. Se llama repetidamente en un bucle hasta que devuelve `false` (fin del
archivo). Por dentro:

1. `av_read_frame`: lee el siguiente paquete comprimido del archivo (puede ser de
   audio, video o lo que sea que tenga el contenedor).
2. Si el paquete es del stream de audio (`avPacket->stream_index == streamIndex`),
   se lo pasa al decodificador (`avcodec_send_packet`) y pide el resultado
   (`avcodec_receive_frame`) — un decodificador puede necesitar varios paquetes
   antes de soltar un frame, o soltar varios frames por un solo paquete; por eso el
   `while` interno.
3. Una vez hay un `AVFrame` decodificado, se usa `swr_convert` para pasarlo de su
   formato original a PCM S16 y se copia a `frame.data`.
4. Devuelve `true` con el frame listo. Si se acaba el archivo sin encontrar más
   audio, devuelve `false`.

**`void close()`** — libera el contexto del codec, el contenedor y el resampler.
Se puede llamar varias veces sin problema (revisa que cada puntero no sea nulo
antes de liberarlo).

---

## `LMPlayer.h` / `LMPlayer.cpp`

Contiene toda la lógica de "negocio" del reproductor: listar canciones, elegir
cuál sigue, y reproducirla (ya sea mp3 directo o m4a vía `AudioDecoder`).

### Funciones públicas (declaradas en el `.h`, usadas desde `main.cpp`)

**`std::string toString(const wchar_t* wstr)`** — Windows entrega los nombres de
archivo como `wchar_t*` (UTF-16) al listar una carpeta; esta función los convierte
a `std::string` en UTF-8, que es lo que usa el resto del programa.

**`int listarCanciones(const std::string& carpeta, std::vector<std::string>** lista)`**
— recorre la carpeta de música buscando primero todos los `.mp3` y luego todos los
`.m4a`, y guarda cada nombre de archivo (sin ruta) en un vector nuevo que crea con
`new`. Recibe un `vector<string>**` (puntero a puntero) porque necesita
*reemplazar* el puntero que tiene `main.cpp`, no solo modificar un vector
existente. Devuelve cuántas canciones encontró.

**`int seleccionarCancion(int total, std::deque<int>& historial)`** — elige un
índice aleatorio entre `0` y `total-1`, evitando repetir cualquiera de los índices
que estén en `historial` (las últimas 15 canciones reproducidas). Guarda el nuevo
índice en el historial y, si ya tiene más de 15, descarta el más viejo
(`pop_front`) — así el historial funciona como una "memoria" de tamaño fijo.

**`void reproducirCancion(const std::string& ruta)`** — punto de entrada único
para reproducir cualquier canción. Solo decide, según la extensión del archivo, si
llamar a `reproducirM4A` o `reproducirMP3`.

**`void modoAleatorio(std::vector<std::string>* canciones)`** — bucle infinito:
elige una canción al azar (`seleccionarCancion`) y la reproduce
(`reproducirCancion`), una y otra vez. Si la lista está vacía o es nula, no hace
nada.

### Funciones internas (`static`, solo usadas dentro de `LMPlayer.cpp`)

No están en el `.h` a propósito: son detalles de implementación de cómo se
reproduce el audio, nada fuera de este archivo necesita llamarlas directamente.

**`reproducirMP3(const std::string& ruta)`** — usa el motor de alto nivel de
miniaudio (`ma_engine` + `ma_sound`), que ya sabe decodificar mp3 solo. Inicia la
reproducción y espera en un bucle (`while (ma_sound_is_playing(...))`) a que
termine antes de liberar los recursos y devolver el control (así el modo aleatorio
puede pasar a la siguiente canción).

**`reproducirM4A(const std::string& ruta)`** — la parte más compleja del
programa. Aquí es donde se conecta `AudioDecoder` (FFmpeg) con miniaudio. Ver la
sección siguiente para el detalle de cómo funciona.

**`struct PlaybackCtx`** — un paquete pequeño con lo que el callback de audio
necesita ver: el puntero al ring buffer y cuántos bytes ocupa un "frame" de audio
(un frame = una muestra por cada canal, ej. 4 bytes en estéreo S16). Se le pasa a
miniaudio como `pUserData` porque el callback de audio no tiene forma de recibir
parámetros normales — es una función con firma fija que llama miniaudio por su
cuenta.

**`audioDataCallback(...)`** — la función que miniaudio llama automáticamente,
en su propio hilo, cada vez que la tarjeta de sonido necesita más audio para
reproducir (típicamente varias veces por segundo). Su trabajo es simple: sacar
datos del ring buffer y copiarlos a `pOutput` (el buffer que espera la tarjeta de
sonido). Si no hay suficientes datos listos todavía (el decodificador se está
atrasando), rellena el resto con silencio en vez de dejar basura o trabarse.

**`escribirFrameEnBuffer(...)`** — lo contrario del callback: mete un `PCMFrame`
ya decodificado dentro del ring buffer. Si el buffer está lleno (el decodificador
va más rápido que la reproducción), espera un poco (`Sleep(5)`) y reintenta.

### ¿Por qué hace falta un "ring buffer" para el m4a y no para el mp3?

Con mp3, miniaudio decodifica *internamente* cuando lo necesita — el callback de
audio y el decodificador son la misma cosa, escondidos dentro de la librería.

Con m4a, **nosotros** somos el decodificador (usamos FFmpeg desde afuera). El
problema es que:

- Decodificar con FFmpeg pasa en nuestro hilo normal (el mismo que corre
  `modoAleatorio`).
- Pero miniaudio reproduce llamando a `audioDataCallback` desde **su propio
  hilo interno**, en tiempo real, muchas veces por segundo.

No podemos decodificar directamente *dentro* del callback (sería lento e
impredecible, y arruinaría el audio con cortes). Entonces usamos un **ring buffer**
(`ma_pcm_rb`, incluido en miniaudio) como intermediario seguro entre los dos
hilos:

```
[hilo principal]                    [hilo de audio de miniaudio]
AudioDecoder → PCMFrame  ──escribe──►  ring buffer  ──lee──► audioDataCallback → tarjeta de sonido
```

El hilo principal decodifica todo el archivo lo más rápido que puede y va
"llenando" el buffer; el hilo de audio va "vaciándolo" al ritmo real de
reproducción. Si el buffer se llena, el hilo principal espera un poco; si se
vacía, el callback rellena con silencio en vez de trabarse.

### Flujo completo de `reproducirM4A`, paso a paso

1. Abre el archivo con `AudioDecoder::openFile`.
2. Decodifica el primer frame para **descubrir** el sample rate y los canales
   reales del archivo (antes el código tenía esto fijo a 44100 Hz / 2 canales, lo
   cual solo funcionaba por casualidad con algunos archivos).
3. Crea el ring buffer (`ma_pcm_rb_init`) con capacidad para ~1 segundo de audio.
4. Configura y arranca el dispositivo de audio (`ma_device`) usando el sample
   rate/canales reales del archivo.
5. Escribe el primer frame (ya decodificado en el paso 2) al ring buffer.
6. Sigue llamando a `getNextFrame` en bucle, escribiendo cada frame al buffer,
   hasta que se acabe el archivo.
7. Espera a que el ring buffer se vacíe del todo (`ma_pcm_rb_available_read == 0`)
   — si no se esperara, la canción se cortaría antes de terminar de sonar.
8. Libera el dispositivo, el ring buffer y cierra el decodificador.

---

## `main.cpp`

Es el punto de entrada. Muestra un menú simple en consola y llama a las funciones
de `LMPlayer` según lo que elija el usuario:

- Al arrancar, carga la biblioteca automáticamente (`listarCanciones`) para no
  depender de que el usuario recuerde usar la opción 4 primero.
- **`(1) Random Mode`** → llama a `modoAleatorio`. Ojo: como `modoAleatorio` es un
  bucle infinito, una vez que entrás ahí no volvés al menú (para eso haría falta
  agregar una forma de interrumpirlo, por ejemplo escuchando una tecla en otro
  hilo).
- **`(2) PlayLists`** → todavía no implementado.
- **`(3) Exit`** → sale del bucle principal y termina el programa.
- **`(4) Update Library`** → vuelve a escanear la carpeta de música.

El bucle principal es un `do...while(userInput != 3)`: repite el menú hasta que
el usuario elige explícitamente Salir.

---

## Glosario rápido de términos de FFmpeg/miniaudio usados

| Término                | Qué es                                                                 |
|--------------------------|-------------------------------------------------------------------------|
| `AVFormatContext`         | Representa el archivo/contenedor abierto (m4a, mp4, etc.)              |
| `AVCodecContext`          | El decodificador de audio ya configurado y abierto                     |
| `AVPacket`                | Un trozo de datos **comprimidos**, tal cual vienen en el archivo        |
| `AVFrame`                 | Un frame de audio ya **decodificado**, en el formato original del codec|
| `SwrContext`              | El "conversor de formato" que pasa de un formato de muestra a otro     |
| `PCM`                     | Audio sin comprimir, muestra por muestra (lo que entiende la tarjeta)  |
| `ma_engine` / `ma_sound`  | API de alto nivel de miniaudio (usada para mp3, decodifica sola)       |
| `ma_device`               | API de bajo nivel de miniaudio (usada para m4a, solo reproduce PCM crudo, nosotros decodificamos)|
| `ma_pcm_rb`               | Ring buffer de miniaudio, usado para pasar audio de forma segura entre hilos |
