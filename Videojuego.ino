#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <SD.h>

// ==========================================
// TFT ST7789
// ==========================================

#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

// MICRO SD
#define SD_CS 27

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);

const int buzzer = 15;
const int buzzerMusica = 16;
// ==========================================
// JOYSTICK
// ==========================================

#define JOY_HOR 34
#define JOY_VER 35
#define JOY_SW  32
#define PAUSE_BUTTON 13

// TECLADO

int filaTeclado = 0;
int columnaTeclado = 0;

bool modoMenu = true;

bool modoTeclado = false;
String textoIngresado = "";
String inicialesJugador = "";

// ==========================================
// PUNTUACION
// ==========================================
// puntosBase: puntos obtenidos solo por sobrevivir. Maximo 9000.
// puntosBonus: puntos extra de futuros power-ups (x2, onda expansiva, etc.).
// score: total que se muestra y se guarda en el TOP.
unsigned long puntosBase = 0;
unsigned long puntosBonus = 0;
unsigned long score = 0;

// ==========================================
// ESTADOS DE POWER-UPS
// ==========================================

// X2
unsigned long puntosExtraX2 = 0;
unsigned long ultimoCalculoX2 = 0;
bool multiplicadorX2 = false;
unsigned long x2Hasta = 0;

// GLITCH
long penalizacionGlitch = 0;

// Indicador temporal de penalizacion GLITCH en pantalla.
bool mostrarGlitchMenos250 = false;
unsigned long glitchMenos250Hasta = 0;

// ESCUDO
bool escudoActivo = false;

// CAMARA LENTA
bool camaraLenta = false;
unsigned long camaraLentaHasta = 0;

const unsigned long DURACION_PARTIDA_MS = 90000;
const unsigned long PUNTOS_BASE_MAX = 9000;

unsigned long inicioPartida = 0;

// ========================================
// MUSICA ESPACIAL NO BLOQUEANTE - BUZZER 2
// ========================================
bool musicaJuegoActiva = false;
bool musicaJuegoPausada = false;
int indiceNotaMusica = 0;
unsigned long proximoCambioMusica = 0;
unsigned long silenciarMusicaHasta = 0;

const int notasMusica[] = {
  220,330,440,330, 262,392,523,392,
  196,294,370,294, 233,349,466,
  392,330,262,220, 0
};

const int duracionesMusica[] = {
  200,150,200,150, 170,170,230,140,
  250,160,210,150, 190,190,250,
  150,150,180,330, 180
};

const int TOTAL_NOTAS_MUSICA = sizeof(notasMusica) / sizeof(notasMusica[0]);

void iniciarMusicaJuego();
void detenerMusicaJuego();
void pausarMusicaJuego();
void reanudarMusicaJuego();
void actualizarMusicaJuego();
void destacarEfectoSobreMusica(unsigned long ms);

// Actualiza los puntos de supervivencia sin permitir que superen 9000.
// Los bonus se suman aparte y SI pueden hacer que el score total supere 9000.
void actualizarPuntuacion() {

 unsigned long ahora = millis();

  unsigned long tiempoJugado = ahora - inicioPartida;

  if (tiempoJugado > DURACION_PARTIDA_MS) {
    tiempoJugado = DURACION_PARTIDA_MS;
  }

  // ========================================
  // PUNTOS BASE
  // ========================================

  puntosBase = tiempoJugado / 10;

  if (puntosBase > PUNTOS_BASE_MAX) {
    puntosBase = PUNTOS_BASE_MAX;
  }


  // ========================================
  // MULTIPLICADOR X2
  // ========================================

  if (multiplicadorX2) {

    // Durante x2 agregamos aproximadamente
    // otros 100 puntos por segundo.
    unsigned long tiempoDesdeUltimo =
      ahora - ultimoCalculoX2;

    puntosExtraX2 += tiempoDesdeUltimo / 10;

    ultimoCalculoX2 = ahora;

    // Terminar despues de 5 segundos
if (ahora >= x2Hasta) {

  multiplicadorX2 = false;

  // Borrar indicador X2
  tft.fillRect(
    180,
    40,
    28,
    16,
    ST77XX_BLACK
  );

  Serial.println("X2 TERMINADO");
}
  }
  else {

    // Evita acumular tiempo mientras x2 esta apagado
    ultimoCalculoX2 = ahora;
  }


  // ========================================
  // SCORE TOTAL
  // ========================================

  long total =
    (long)puntosBase +
    (long)puntosBonus +
    (long)puntosExtraX2 -
    (long)penalizacionGlitch;

  // El corrupto nunca puede dejar puntos negativos
  if (total < 0) {
    total = 0;
  }

  score = (unsigned long)total;
}

struct RegistroScore {
  String iniciales;
  unsigned long puntos;
};

const int MAX_SCORES = 4;
RegistroScore topScores[MAX_SCORES];
int cantidadScores = 0;


unsigned long ultimoMovimientoTeclado = 0;
const unsigned long retrasoMovimientoTeclado = 180;

const char* teclado[4][10] = {
  {"Q","W","E","R","T","Y","U","I","O","P"},
  {"A","S","D","F","G","H","J","K","L",""},
  {"Z","X","C","V","B","N","M","","",""},
  {"<"," "," "," "," "," "," "," "," ","ENTER"}
};

// PANTALLA DE INCIO

int opcionMenu = 0;

// ESTADOS DE PANTALLAS / MENUS
bool modoScores = false;
bool pausa = false;
int opcionPausa = 0;
unsigned long inicioPausa = 0;


// ==========================================
// JUGADOR
// ==========================================

const int jugadorAncho = 22;
const int jugadorAlto  = 28;
const int velocidadJugador = 10;

const bool jugadorInvenciblePrueba = false;


int jugadorX;
int jugadorY;


// ==========================================
// PROYECTILES
// ==========================================

#define MAX_PROYECTILES 12

// Tamaño base del cohete horizontal.
// Los cohetes gigantes del nivel 6 usan exactamente x3 este tamaño.
const int proyectilAncho = 14;
const int proyectilAlto = 9;
const int escalaProyectilGigante = 3;
const int probabilidadGiganteNivel6 = 15; // 15% de probabilidad por cohete

// Direcciones disponibles según el nivel.
enum DireccionProyectil {
  DESDE_IZQUIERDA,
  DESDE_ARRIBA,
  DESDE_ABAJO
};

int proyectilX[MAX_PROYECTILES];
int proyectilY[MAX_PROYECTILES];

bool proyectilActivo[MAX_PROYECTILES];
bool proyectilGigante[MAX_PROYECTILES];
DireccionProyectil proyectilDireccion[MAX_PROYECTILES];

unsigned long tiempoProyectil[MAX_PROYECTILES];
unsigned long ultimoDisparo = 0;

// POTENCIADORES / TRAMPA
enum TipoPowerUp {
  POWER_ESCUDO,
  POWER_X2,
  POWER_LENTO,
  POWER_ONDA,
  POWER_GLITCH
};

class PowerUp {
  
  public:
  
  int x;
  int y;

  int ancho = 14;
  int alto = 14;

  TipoPowerUp tipo;

  uint16_t colorVisual;

  bool activo = false;

// Movimiento
  int velocidad = 2;
  unsigned long ultimoMovimiento = 0;
  unsigned long intervaloMovimiento = 35;

// Aparicion
  unsigned long ultimoSpawn = 0;
  unsigned long intervaloSpawn = 10000;

// Evita que salga exactamente el mismo power-up dos veces seguidas.
  int ultimoTipoElegido = -1;


    // --------------------------------------
    // CONSTRUCTOR
    // --------------------------------------

    PowerUp() {
      x = -20;
      y = 0;

      activo = false;

      ultimoSpawn = millis();
    }


    // --------------------------------------
    // CREAR POWER-UP
    // --------------------------------------

void prepararSiguienteSpawn() {

  // Esperar entre 8 y 13 segundos
  intervaloSpawn = random(8000, 13001);

  // La espera comienza desde este momento
  ultimoSpawn = millis();
}

void aparecer() {

  // Si ya existe uno, no crear otro
  if (activo) {
    return;
  }

  // Esperar antes de crear el siguiente
  if (millis() - ultimoSpawn < intervaloSpawn) {
    return;
  }

  // Elegir tipo aleatorio, pero NUNCA repetir el mismo dos veces seguidas.
  // Esto evita rachas como ESCUDO, ESCUDO, ESCUDO...
  int nuevoTipo;
  do {
    nuevoTipo = random(0, 5);
  } while (nuevoTipo == ultimoTipoElegido);

  tipo = (TipoPowerUp)nuevoTipo;
  ultimoTipoElegido = nuevoTipo;


  // ========================================
  // ELEGIR COLOR
  // ========================================

  switch (tipo) {

    case POWER_ESCUDO:
      colorVisual = ST77XX_BLUE;
      break;

    case POWER_X2:
      colorVisual = ST77XX_MAGENTA;
      break;

    case POWER_LENTO:
      colorVisual = ST77XX_GREEN;
      break;

    case POWER_ONDA:
      colorVisual = 0xFD20;
      break;

    case POWER_GLITCH:

      // Se disfraza de UN SOLO color
      switch (random(0, 4)) {

        case 0:
          colorVisual = ST77XX_BLUE;
          break;

        case 1:
          colorVisual = ST77XX_MAGENTA;
          break;

        case 2:
          colorVisual = ST77XX_GREEN;
          break;

        default:
          colorVisual = 0xFD20;
          break;
      }

      break;
  }


  // ========================================
  // APARECER POR EL LADO IZQUIERDO
  // ========================================

  x = -ancho;

  // Evitar HUD superior y borde inferior
  y = random(45, 215 - alto);

  activo = true;

  ultimoMovimiento = millis();

  dibujar();
}


    // --------------------------------------
    // DIBUJAR
    // --------------------------------------

void dibujar() {

  if (!activo) {
    return;
  }

  // El color ya fue elegido cuando aparecio
  tft.fillRect(
    x,
    y,
    ancho,
    alto,
    colorVisual
  );

  // Centro blanco
  tft.fillRect(
    x + 5,
    y + 5,
    4,
    4,
    ST77XX_WHITE
  );
}

void mover() {

  if (!activo) {
    return;
  }

  unsigned long ahora = millis();

  if (ahora - ultimoMovimiento < intervaloMovimiento) {
    return;
  }

  ultimoMovimiento = ahora;


  // Borrar posicion anterior
  borrar();


  // Mover hacia la derecha
  x += velocidad;


  // ========================================
  // SALIO DE LA PANTALLA
  // ========================================

if (x > 320) {

  activo = false;

  // Esperar nuevamente entre 8 y 13 segundos
  prepararSiguienteSpawn();

  return;
}


  // Dibujar nueva posicion
  dibujar();
}

    // --------------------------------------
    // BORRAR
    // --------------------------------------

    void borrar() {

      tft.fillRect(
        x,
        y,
        ancho,
        alto,
        ST77XX_BLACK
      );
    }
};

// Crear el objeto real
PowerUp powerUp;

void animacionOndaExpansiva() {

  int centroX = jugadorX + (jugadorAncho / 2);
  int centroY = jugadorY + (jugadorAlto / 2);

  // Tres circulos consecutivos dan sensacion de expansion
  for (int radio = 20; radio <= 100; radio += 20) {

    tft.drawCircle(
      centroX,
      centroY,
      radio,
      0xFD20   // Naranja
    );

    delay(20);
  }

  delay(40);

  // Limpiar solamente los circulos dibujados
  for (int radio = 20; radio <= 100; radio += 20) {

    tft.drawCircle(
      centroX,
      centroY,
      radio,
      ST77XX_BLACK
    );
  }

  // Restaurar jugador
  dibujarJugador(jugadorX, jugadorY);

  // Restaurar los cohetes que siguen activos
  for (int i = 0; i < MAX_PROYECTILES; i++) {

    if (proyectilActivo[i]) {
      dibujarProyectil(i);
    }
  }
}


// ========================================
// EFECTOS DE SONIDO AGREGADOS - BUZZER GPIO15
// ========================================
// Estos sonidos se agregan sin reemplazar los sonidos originales del juego.

// Power-up: efecto brillante tipo moneda retro/arcade.
void sonidoPowerUp() {
  // Suena sobre la musica; NO detenemos GPIO16.
  tone(buzzer, 1500, 55);
  delay(60);
  tone(buzzer, 2100, 60);
  delay(65);
  tone(buzzer, 2800, 100);
  delay(105);
  noTone(buzzer);
}

// GLITCH: golpe corto y seco, distinto al sonido de derrota.
void sonidoGolpeGlitch() {
  // Golpe de GLITCH sobre la musica; NO detenemos GPIO16.
  tone(buzzer, 420, 65);
  delay(70);
  tone(buzzer, 240, 80);
  delay(85);
  tone(buzzer, 130, 120);
  delay(125);
  noTone(buzzer);
}

// Confirmacion al presionar SW del joystick.
void sonidoConfirmar() {
  destacarEfectoSobreMusica(180);
  tone(buzzer, 1050, 55);
  delay(60);
  tone(buzzer, 1750, 90);
  delay(95);
  noTone(buzzer);
}

// Boton fisico de pausa: efecto mas grave y distinto al de confirmacion.
void sonidoPausa() {
  tone(buzzer, 800, 80);
  delay(85);
  tone(buzzer, 520, 110);
  delay(115);
  tone(buzzer, 330, 150);
  delay(155);
  noTone(buzzer);
}

// Onda expansiva: explosion retro.
void sonidoExplosionOnda() {
  // Explosion sobre la musica; NO detenemos GPIO16.
  tone(buzzer, 650, 60);
  delay(65);
  tone(buzzer, 420, 75);
  delay(80);
  tone(buzzer, 260, 95);
  delay(100);
  tone(buzzer, 140, 150);
  delay(155);
  noTone(buzzer);
}

// Derrota: sonido grave y descendente.
void sonidoDerrota() {
  noTone(buzzerMusica);
  tone(buzzer, 440, 120);
  delay(130);
  tone(buzzer, 350, 140);
  delay(150);
  tone(buzzer, 280, 170);
  delay(180);
  tone(buzzer, 210, 220);
  delay(230);
  tone(buzzer, 150, 350);
  delay(360);
  noTone(buzzer);
}

// Victoria: ascenso brillante y final largo.
void sonidoVictoria() {
  noTone(buzzerMusica);
  tone(buzzer, 523, 110); delay(120);
  tone(buzzer, 659, 110); delay(120);
  tone(buzzer, 784, 140); delay(150);
  tone(buzzer, 1046, 180); delay(190);
  tone(buzzer, 1318, 320); delay(330);
  noTone(buzzer);
}

void revisarPowerUp() {

  if (!powerUp.activo) {
    return;
  }

  // Colision rectangular jugador vs power-up
  bool colision =
    jugadorX < powerUp.x + powerUp.ancho &&
    jugadorX + jugadorAncho > powerUp.x &&
    jugadorY < powerUp.y + powerUp.alto &&
    jugadorY + jugadorAlto > powerUp.y;

  if (!colision) {
    return;
  }

  // Borrar cubo
  powerUp.borrar();
  powerUp.activo = false;
  powerUp.prepararSiguienteSpawn();

  // ========================================
  // APLICAR EFECTO
  // ========================================

  // Los power-ups buenos usan sonido tipo moneda.
  // El GLITCH usa su propio sonido de golpe.
  if (powerUp.tipo != POWER_GLITCH) {
    sonidoPowerUp();
  }

  switch (powerUp.tipo) {

    // ------------------------------
    // ESCUDO AZUL
    // ------------------------------
    case POWER_ESCUDO:

      escudoActivo = true;

      Serial.println("POWER-UP: ESCUDO");

      // Después conectaremos aquí:
      // sonidoEscudo();
      dibujarEscudo();
      break;


    // ------------------------------
    // MULTIPLICADOR X2
    // ------------------------------
    case POWER_X2:

      multiplicadorX2 = true;
      x2Hasta = millis() + 5000;
      ultimoCalculoX2 = millis();

      dibujarIndicadorX2();

      Serial.println("POWER-UP: X2 - 5 segundos");

      // Después:
      // sonidoPowerUpX2();

      break;


    // ------------------------------
    // CAMARA LENTA
    // ------------------------------
    case POWER_LENTO:

      camaraLenta = true;
      camaraLentaHasta = millis() + 3000;

      dibujarIndicadorSlow();

      Serial.println("POWER-UP: CAMARA LENTA - 3 segundos");

      // Después:
      // sonidoCamaraLenta();

      break;

    // ------------------------------
    // ONDA EXPANSIVA
    // ------------------------------

    case POWER_ONDA: {
      
      int destruidos = 0;

      // Centro del jugador
      int centroX = jugadorX + (jugadorAncho / 2);
      int centroY = jugadorY + (jugadorAlto / 2);

      for (int i = 0; i < MAX_PROYECTILES; i++) {
        if (!proyectilActivo[i]) {
          continue;
        }
        
        // Centro aproximado del cohete
        int coheteX = proyectilX[i];
        int coheteY = proyectilY[i];

        int dx = coheteX - centroX;
        int dy = coheteY - centroY;

        // 100 pixeles de radio
        if ((dx * dx) + (dy * dy) <= 10000) {
          
          borrarProyectil(i);
          proyectilActivo[i] = false;

          destruidos++;
        }
      }
  // +100 por cada cohete destruido
  puntosBonus += destruidos * 100;


  // Mostrar la onda DESPUES de destruirlos
  animacionOndaExpansiva();
  sonidoExplosionOnda();


  Serial.print("POWER-UP: ONDA - Cohetes destruidos: ");
  Serial.println(destruidos);

  Serial.print("BONUS: +");
  Serial.println(destruidos * 100);

  break;
}


    // ------------------------------
    // GLITCH / CORRUPTO
    // ------------------------------
    case POWER_GLITCH:

      penalizacionGlitch += 250;

      // Mostrar -250 temporalmente en el HUD.
      mostrarGlitchMenos250 = true;
      glitchMenos250Hasta = millis() + 900;

      // Sonido de golpe/corrupcion, NO sonido de moneda ni derrota.
      sonidoGolpeGlitch();

      Serial.println("POWER-UP CORRUPTO!");
      Serial.println("PENALIZACION: -250");

      break;
  }
}

// ==========================================
// SONIDITO
// ==========================================

void nota(int frecuencia, int duracion)
{
    tone(buzzer, frecuencia, duracion);
    delay(duracion);
    noTone(buzzer);
}

void paso() {
    // SONIDO ORIGINAL DE PASO:
    // 500 Hz durante 25 ms y despues 750 Hz durante 35 ms.
    //
    // IMPORTANTE:
    // Este efecto usa EXCLUSIVAMENTE el buzzer de GPIO15.
    // La melodia continua en el segundo buzzer (GPIO16).
    // No se llama detenerMusicaJuego(), pausarMusicaJuego()
    // ni noTone(buzzerMusica), por lo que ambos buzzers pueden sonar juntos.

    tone(buzzer, 500);
    delay(25);

    tone(buzzer, 750);
    delay(35);

    noTone(buzzer);
}

// ==========================================
// CONTROL DE MUSICA ESPACIAL SIN DELAY
// ==========================================
void iniciarMusicaJuego() {
  musicaJuegoActiva = true;
  musicaJuegoPausada = false;
  indiceNotaMusica = 0;
  silenciarMusicaHasta = 0;
  proximoCambioMusica = 0;
}

void detenerMusicaJuego() {
  musicaJuegoActiva = false;
  musicaJuegoPausada = false;
  noTone(buzzerMusica);
}

void pausarMusicaJuego() {
  musicaJuegoPausada = true;
  noTone(buzzerMusica);
}

void reanudarMusicaJuego() {
  if (musicaJuegoActiva) {
    musicaJuegoPausada = false;
    proximoCambioMusica = 0;
  }
}

void destacarEfectoSobreMusica(unsigned long ms) {
  if (!musicaJuegoActiva) return;
  noTone(buzzerMusica);
  silenciarMusicaHasta = millis() + ms;
}

void actualizarMusicaJuego() {
  if (!musicaJuegoActiva || musicaJuegoPausada) return;

  unsigned long ahoraMusica = millis();
  if (ahoraMusica < silenciarMusicaHasta) return;
  if (ahoraMusica < proximoCambioMusica) return;

  int notaActual = notasMusica[indiceNotaMusica];
  int duracionActual = duracionesMusica[indiceNotaMusica];

  // SLOW tambien ralentiza la musica mientras dura el power-up.
  if (camaraLenta && ahoraMusica < camaraLentaHasta) {
    duracionActual *= 2;
  }

  if (notaActual == 0) noTone(buzzerMusica);
  else tone(buzzerMusica, notaActual);

  proximoCambioMusica = ahoraMusica + duracionActual;
  indiceNotaMusica++;
  if (indiceNotaMusica >= TOTAL_NOTAS_MUSICA) indiceNotaMusica = 0;
}

void cancion()
{
  // PRIMERA PARTE
  tone(buzzer, 1046);
  delay(250);

  tone(buzzer, 1244);
  delay(250);

  tone(buzzer, 1400);
  delay(250);

  tone(buzzer, 1510);
  delay(250);

  tone(buzzer, 1400);
  delay(250);

  tone(buzzer, 1244);
  delay(250);

  tone(buzzer, 1046);
  delay(250);

  noTone(buzzer);
  delay(500);


  tone(buzzer, 932);
  delay(125);

  tone(buzzer, 1174);
  delay(125);

  tone(buzzer, 1046);
  delay(250);

  noTone(buzzer);
  delay(500);


  // SEGUNDA PARTE
  tone(buzzer, 780);
  delay(250);

  tone(buzzer, 525);
  delay(250);

  noTone(buzzer);
  delay(250);


  // TERCERA PARTE
  tone(buzzer, 1046);
  delay(250);

  tone(buzzer, 1244);
  delay(250);

  tone(buzzer, 1400);
  delay(250);

  tone(buzzer, 1510);
  delay(250);

  tone(buzzer, 1400);
  delay(250);

  tone(buzzer, 1244);
  delay(250);

  tone(buzzer, 1400);
  delay(250);

  noTone(buzzer);
  delay(750);


  // PARTE RAPIDA
  tone(buzzer, 1510);
  delay(200);

  tone(buzzer, 1400);
  delay(200);

  tone(buzzer, 1244);
  delay(200);

  tone(buzzer, 1510);
  delay(200);

  tone(buzzer, 1400);
  delay(200);

  tone(buzzer, 1244);
  delay(200);

  tone(buzzer, 1510);
  delay(200);

  tone(buzzer, 1400);
  delay(200);

  tone(buzzer, 1244);
  delay(200);



  tone(buzzer, 1046);
  delay(250);

  noTone(buzzer);
  delay(500);
}

// ==========================================
// CANCION DEL MENU SIN DELAY
// ==========================================

const int notasCancionMenu[] = {
  1046, 1244, 1400, 1510, 1400, 1244, 1046, 0,
  932, 1174, 1046, 0,
  780, 525, 0,
  1046, 1244, 1400, 1510, 1400, 1244, 1400, 0,
  1510, 1400, 1244,
  1510, 1400, 1244,
  1510, 1400, 1244,
  1046, 0
};

const int duracionesCancionMenu[] = {
  250, 250, 250, 250, 250, 250, 250, 500,
  125, 125, 250, 500,
  250, 250, 250,
  250, 250, 250, 250, 250, 250, 250, 750,
  200, 200, 200,
  200, 200, 200,
  200, 200, 200,
  250, 500
};

const int TOTAL_NOTAS_MENU =
  sizeof(notasCancionMenu) / sizeof(notasCancionMenu[0]);

int indiceCancionMenu = 0;

unsigned long siguienteNotaMenu = 0;

bool cancionMenuActiva = false;


void iniciarCancionMenu() {

  indiceCancionMenu = 0;

  siguienteNotaMenu = 0;

  cancionMenuActiva = true;
}


void actualizarCancionMenu() {

  if (!cancionMenuActiva) return;

  unsigned long ahora = millis();

  if (ahora < siguienteNotaMenu) return;

  int frecuencia = notasCancionMenu[indiceCancionMenu];

  int duracion = duracionesCancionMenu[indiceCancionMenu];


  if (frecuencia == 0) {

    noTone(buzzer);

  }
  else {

    tone(buzzer, frecuencia, duracion);

  }

  siguienteNotaMenu = ahora + duracion;

  indiceCancionMenu++;


  if (indiceCancionMenu >= TOTAL_NOTAS_MENU) {

    indiceCancionMenu = 0;
  }
}


void detenerCancionMenu() {

  cancionMenuActiva = false;

  noTone(buzzer);
}


void melodiaEspacial()
{
  // INTRO ESPACIAL
  tone(buzzerMusica, 220);
  delay(200);

  tone(buzzerMusica, 330);
  delay(150);

  tone(buzzerMusica, 440);
  delay(200);

  tone(buzzerMusica, 330);
  delay(150);


  // ASCENSO DE TENSION
  tone(buzzerMusica, 262);
  delay(170);

  tone(buzzerMusica, 392);
  delay(170);

  tone(buzzerMusica, 523);
  delay(230);

  tone(buzzerMusica, 392);
  delay(140);


  // PARTE OSCURA
  tone(buzzerMusica, 196);
  delay(250);

  tone(buzzerMusica, 294);
  delay(160);

  tone(buzzerMusica, 370);
  delay(210);

  tone(buzzerMusica, 294);
  delay(150);


  // ALERTA ESPACIAL
  tone(buzzerMusica, 233);
  delay(190);

  tone(buzzerMusica, 349);
  delay(190);

  tone(buzzerMusica, 466);
  delay(250);


  // DESCENSO
  tone(buzzerMusica, 392);
  delay(150);

  tone(buzzerMusica, 330);
  delay(150);

  tone(buzzerMusica, 262);
  delay(180);

  tone(buzzerMusica, 220);
  delay(330);


  noTone(buzzerMusica);
  delay(180);
}
// ==========================================
// NIVEL
// ==========================================

bool derrota = false;
bool victoria = false;
int opcionVictoria = 0;
int opcionDerrota = 0;

unsigned long inicioNivel = 0;

const unsigned long duracionNivel = 15000;

int nivelActual = 1;
const int totalNiveles = 6;

int segundoAnterior = -1;


// Declaracion anticipada
void dibujarEstrellas();


void dibujarEscudo() {

  if (!escudoActivo) {
    return;
  }

  int centroX = jugadorX + (jugadorAncho / 2);
  int centroY = jugadorY + (jugadorAlto / 2);

  // Dos circulos para que el escudo sea mas visible
  tft.drawCircle(centroX, centroY, 18, ST77XX_BLUE);
  tft.drawCircle(centroX, centroY, 19, ST77XX_BLUE);
}

void dibujarIndicadorSlow() {

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);

  tft.setCursor(140, 40);
  tft.print("SLOW");
}

void dibujarIndicadorX2() {

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);

  tft.setCursor(180, 40);
  tft.print("X2");
}

// ==========================================
// INDICADORES DE POWERS SIEMPRE POR ENCIMA
// ==========================================
// Esta funcion se llama al FINAL del frame, despues de dibujar cohetes.
// Asi, si un cohete pasa por la zona del texto, visualmente queda debajo.
void dibujarIndicadoresPowersSobrepuestos() {

  if (camaraLenta) {
    dibujarIndicadorSlow();
  }

  if (multiplicadorX2) {
    dibujarIndicadorX2();
  }

  if (mostrarGlitchMenos250 && millis() < glitchMenos250Hasta) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(220, 40);
    tft.print("-250");
  }
}

// Muestra brevemente la penalizacion del cubo GLITCH.
void actualizarIndicadorGlitch() {

  if (!mostrarGlitchMenos250) {
    return;
  }

  if (millis() < glitchMenos250Hasta) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(220, 40);
    tft.print("-250");
  } else {
    tft.fillRect(220, 40, 50, 16, ST77XX_BLACK);
    mostrarGlitchMenos250 = false;
  }
}


// ==========================================
// DIBUJAR PERSONAJE
// ==========================================

void dibujarJugador(int x, int y) {

  // Mochila - ahora del lado DERECHO
  tft.fillRoundRect(
    x + 16,
    y + 9,
    6,
    15,
    2,
    ST77XX_RED
  );

  // Cuerpo
  tft.fillRoundRect(
    x + 1,
    y + 3,
    17,
    21,
    5,
    ST77XX_RED
  );

  // Visor - ahora hacia la IZQUIERDA
  tft.fillRoundRect(
    x,
    y + 6,
    12,
    7,
    3,
    ST77XX_CYAN
  );

  // Pierna izquierda
  tft.fillRect(
    x + 1,
    y + 20,
    6,
    8,
    ST77XX_RED
  );

  // Pierna derecha
  tft.fillRect(
    x + 10,
    y + 20,
    6,
    8,
    ST77XX_RED
  );
}

// ==========================================
// BORRAR PERSONAJE
// ==========================================

void borrarJugador(int x, int y) {

  tft.fillRect(
    x,
    y,
    jugadorAncho,
    jugadorAlto,
    ST77XX_BLACK
  );

  // Restaurar las estrellas que pudo tapar el jugador
  dibujarEstrellas();
}


// ==========================================
// DIBUJAR CONTADOR
// ==========================================

void dibujarContador(int segundos) {

  // Borrar solamente la zona del contador
  tft.fillRect(
    125,
    0,
    70,
    32,
    ST77XX_BLACK
  );

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);

  // Centrar aproximadamente
  if (segundos >= 10) {
    tft.setCursor(143, 5);
  }
  else {
    tft.setCursor(152, 5);
  }

  tft.print(segundos);
}


// ==========================================
// ESCENARIO - PASILLO DE NAVE ESPACIAL
// ==========================================

void dibujarEscenario() {

  tft.fillScreen(ST77XX_BLACK);

  dibujarEstrellas();
}
void dibujarEstrellas() {

  tft.drawPixel(15, 45, ST77XX_WHITE);
  tft.drawPixel(42, 90, ST77XX_WHITE);
  tft.drawPixel(75, 55, ST77XX_WHITE);
  tft.drawPixel(105, 130, ST77XX_WHITE);
  tft.drawPixel(135, 75, ST77XX_WHITE);
  tft.drawPixel(165, 180, ST77XX_WHITE);
  tft.drawPixel(195, 110, ST77XX_WHITE);
  tft.drawPixel(225, 60, ST77XX_WHITE);
  tft.drawPixel(260, 155, ST77XX_WHITE);
  tft.drawPixel(300, 85, ST77XX_WHITE);

  tft.drawPixel(25, 190, ST77XX_WHITE);
  tft.drawPixel(60, 160, ST77XX_WHITE);
  tft.drawPixel(90, 210, ST77XX_WHITE);
  tft.drawPixel(145, 205, ST77XX_WHITE);
  tft.drawPixel(205, 200, ST77XX_WHITE);
  tft.drawPixel(245, 215, ST77XX_WHITE);
  tft.drawPixel(285, 190, ST77XX_WHITE);

  // Estrellas brillantes
  tft.drawPixel(50, 120, ST77XX_CYAN);
  tft.drawPixel(51, 120, ST77XX_CYAN);
  tft.drawPixel(50, 121, ST77XX_CYAN);

  tft.drawPixel(180, 55, ST77XX_CYAN);
  tft.drawPixel(181, 55, ST77XX_CYAN);
  tft.drawPixel(180, 56, ST77XX_CYAN);

  tft.drawPixel(275, 125, ST77XX_CYAN);
  tft.drawPixel(276, 125, ST77XX_CYAN);
  tft.drawPixel(275, 126, ST77XX_CYAN);
}


// ==========================================
// BORRAR PROYECTIL
// ==========================================

int escalaActualProyectil(int i) {
  return proyectilGigante[i] ? escalaProyectilGigante : 1;
}

int anchoActualProyectil(int i) {
  int escala = escalaActualProyectil(i);

  // Al girar el cohete verticalmente se intercambian ancho y alto.
  if (proyectilDireccion[i] == DESDE_IZQUIERDA) {
    return proyectilAncho * escala;
  }

  return proyectilAlto * escala;
}

int altoActualProyectil(int i) {
  int escala = escalaActualProyectil(i);

  if (proyectilDireccion[i] == DESDE_IZQUIERDA) {
    return proyectilAlto * escala;
  }

  return proyectilAncho * escala;
}

void borrarProyectil(int i) {

  tft.fillRect(
    proyectilX[i],
    proyectilY[i],
    anchoActualProyectil(i) + 1,
    altoActualProyectil(i) + 1,
    ST77XX_BLACK
  );
}


// ==========================================
// REINICIAR
// ==========================================

void reiniciarJuego() {

puntosBase = 0;
puntosBonus = 0;
puntosExtraX2 = 0;
penalizacionGlitch = 0;
mostrarGlitchMenos250 = false;
glitchMenos250Hasta = 0;

multiplicadorX2 = false;
x2Hasta = 0;
ultimoCalculoX2 = millis();

// Reiniciar efectos de power-ups
escudoActivo = false;
camaraLenta = false;
camaraLentaHasta = 0;

score = 0;
  nivelActual = 1;
  inicioPartida = millis();
  iniciarMusicaJuego();

  // Limpiar completamente la pantalla
  tft.fillScreen(ST77XX_BLACK);

  // Desactivar proyectiles anteriores
  for (int i = 0; i < MAX_PROYECTILES; i++) {
    proyectilActivo[i] = false;
    proyectilGigante[i] = false;
    proyectilDireccion[i] = DESDE_IZQUIERDA;
    proyectilX[i] = 0;
    proyectilY[i] = 0;
    tiempoProyectil[i] = 0;
  }

  // ========================================
// REINICIAR POWER-UP
// ========================================

// Borrar cualquier power-up de la partida anterior
if (powerUp.activo) {
  powerUp.borrar();
}

powerUp.activo = false;
powerUp.x = -powerUp.ancho;

// Reiniciar temporizador.
// El primer power-up tardara en aparecer.
powerUp.prepararSiguienteSpawn();

  // Volver a colocar al jugador en el centro
  jugadorX = (tft.width() - jugadorAncho) / 2;
  jugadorY = (tft.height() - jugadorAlto) / 2;

  // Reiniciar tiempos
  inicioNivel = millis();
  ultimoDisparo = millis();
  segundoAnterior = -1;

  // IMPORTANTE:
  // volver al estado normal del juego
  pausa = false;
  derrota = false;
  victoria = false;
  modoTeclado = false;
  modoMenu = false;
  modoScores = false;

  // Dibujar nuevamente el juego
  dibujarEscenario();
  dibujarJugador(jugadorX, jugadorY);
  dibujarContador(15);

  Serial.println("NIVEL 1 REINICIADO");


}



// ==========================================
// TECLADO VIRTUAL
// ==========================================

void dibujarTeclado() {

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 5);
  tft.println("INGRESA TUS INICIALES");

  // Área donde aparece el texto
  tft.drawRect(8, 27, 304, 30, ST77XX_WHITE);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(14, 34);

  if (textoIngresado.length() > 18) {
    tft.print(textoIngresado.substring(textoIngresado.length() - 18));
  }
  else {
    tft.print(textoIngresado);
  }

  const int inicioX = 5;
  const int inicioY = 65;
  const int anchoTecla = 30;
  const int altoTecla = 38;

  for (int fila = 0; fila < 4; fila++) {

    for (int columna = 0; columna < 10; columna++) {

      int x = inicioX + columna * anchoTecla;
      int y = inicioY + fila * altoTecla;

      if (fila == filaTeclado && columna == columnaTeclado) {

        tft.fillRect(
          x + 1, y + 1,
          anchoTecla - 2,
          altoTecla - 2,
          ST77XX_BLUE
        );

        tft.drawRect(
          x, y,
          anchoTecla,
          altoTecla,
          ST77XX_YELLOW
        );
      }
      else {
        tft.drawRect(
          x, y,
          anchoTecla,
          altoTecla,
          ST77XX_WHITE
        );
      }

      tft.setTextColor(ST77XX_WHITE);

      if (fila == 3 && columna == 9) {
        tft.setTextSize(1);
        tft.setCursor(x + 3, y + 15);
        tft.print("ENTER");
      }
      else if (fila == 3 && columna > 0 && columna < 9) {
        tft.setTextSize(1);
        tft.setCursor(x + 8, y + 15);
        tft.print("ESP");
      }
      else if (fila == 3 && columna == 0) {
        tft.setTextSize(2);
        tft.setCursor(x + 8, y + 10);
        tft.print("<");
      }
      else if (teclado[fila][columna][0] != '\0') {
        tft.setTextSize(2);
        tft.setCursor(x + 10, y + 10);
        tft.print(teclado[fila][columna]);
      }
    }
  }

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 225);
  tft.print("Joystick: mover | Boton: seleccionar");
}


void seleccionarTecla() {

  // ENTER
  if (filaTeclado == 3 && columnaTeclado == 9) {

    //No permitir sin seleccionar las Iniciales
    if (textoIngresado.length() == 0){
      tone(buzzer, 200, 150);
      return;
    }

    inicialesJugador = textoIngresado;

    Serial.print("Jugador: ");
    Serial.println(inicialesJugador);

    Serial.print("Texto ingresado: ");
    Serial.println(textoIngresado);

    modoTeclado = false;
    reiniciarJuego();

    return;
  }

  // BORRAR
  if (filaTeclado == 3 && columnaTeclado == 0) {

    if (textoIngresado.length() > 0) {
      textoIngresado.remove(textoIngresado.length() - 1);
    }

    tone(buzzer, 350, 80);
    dibujarTeclado();
    return;
  }

  // ESPACIO
  if (filaTeclado == 3 &&
      columnaTeclado > 0 &&
      columnaTeclado < 9) {

    textoIngresado += " ";

    tone(buzzer, 700, 80);
    dibujarTeclado();
    return;
  }

  // LETRA
  const char* tecla = teclado[filaTeclado][columnaTeclado];

  if (tecla[0] != '\0') {

    // Solo permitir 3 iniciales para el TOP SCORES
    if (textoIngresado.length() < 3) {
      textoIngresado += tecla[0];
    }

    tone(buzzer, 800, 80);
    dibujarTeclado();
  }
}


void controlarTeclado() {

  int horizontal = analogRead(JOY_HOR);
  int vertical = analogRead(JOY_VER);

  unsigned long ahora = millis();

  if (ahora - ultimoMovimientoTeclado >= retrasoMovimientoTeclado) {

    bool seMovio = false;

    if (horizontal < 1000) {
      columnaTeclado--;

      if (columnaTeclado < 0) {
        columnaTeclado = 9;
      }

      seMovio = true;
    }
    else if (horizontal > 3000) {
      columnaTeclado++;

      if (columnaTeclado > 9) {
        columnaTeclado = 0;
      }

      seMovio = true;
    }

    if (vertical < 1200) {
      filaTeclado--;

      if (filaTeclado < 0) {
        filaTeclado = 3;
      }

      seMovio = true;
    }
    else if (vertical > 2800) {
      filaTeclado++;

      if (filaTeclado > 3) {
        filaTeclado = 0;
      }

      seMovio = true;
    }

    if (seMovio) {
      ultimoMovimientoTeclado = ahora;
      tone(buzzer, 500, 30);
      dibujarTeclado();
    }
  }

  if (digitalRead(JOY_SW) == LOW) {

    delay(150);

    while (digitalRead(JOY_SW) == LOW) {
      delay(10);
    }

    sonidoConfirmar();
    seleccionarTecla();
  }
}

// MENÚ
// PRINCUPAL


// ==========================================
// IMAGENES BMP DESDE MICROSD PARA EL MENU
// ==========================================
// Compatible con BMP de 24 bits sin compresion.
// /fondo.bmp = 320x240
// /logo.bmp  = 110x110

uint16_t leer16BMP(File &f) {
  uint16_t resultado;
  ((uint8_t *)&resultado)[0] = f.read();
  ((uint8_t *)&resultado)[1] = f.read();
  return resultado;
}

uint32_t leer32BMP(File &f) {
  uint32_t resultado;
  ((uint8_t *)&resultado)[0] = f.read();
  ((uint8_t *)&resultado)[1] = f.read();
  ((uint8_t *)&resultado)[2] = f.read();
  ((uint8_t *)&resultado)[3] = f.read();
  return resultado;
}

bool dibujarBMP(const char *nombre, int16_t x, int16_t y) {

  File bmpFile = SD.open(nombre, FILE_READ);

  if (!bmpFile) {
    Serial.print("ERROR: No se encontro ");
    Serial.println(nombre);
    return false;
  }

  // Firma BMP: letras B y M
  if (leer16BMP(bmpFile) != 0x4D42) {
    Serial.print("ERROR: Archivo BMP invalido: ");
    Serial.println(nombre);
    bmpFile.close();
    return false;
  }

  // Saltamos tamano del archivo y campos reservados
  leer32BMP(bmpFile);
  leer32BMP(bmpFile);

  uint32_t offsetDatos = leer32BMP(bmpFile);

  // Cabecera DIB
  leer32BMP(bmpFile);
  int32_t anchoBMP = (int32_t)leer32BMP(bmpFile);
  int32_t altoBMP  = (int32_t)leer32BMP(bmpFile);

  uint16_t planos = leer16BMP(bmpFile);
  uint16_t bitsPorPixel = leer16BMP(bmpFile);
  uint32_t compresion = leer32BMP(bmpFile);

  if (planos != 1 || bitsPorPixel != 24 || compresion != 0) {
    Serial.print("ERROR: ");
    Serial.print(nombre);
    Serial.println(" debe ser BMP 24-bit sin compresion.");
    bmpFile.close();
    return false;
  }

  bool deAbajoHaciaArriba = true;

  if (altoBMP < 0) {
    altoBMP = -altoBMP;
    deAbajoHaciaArriba = false;
  }

  if (anchoBMP <= 0 || altoBMP <= 0) {
    bmpFile.close();
    return false;
  }

  // Cada fila BMP se completa a multiplo de 4 bytes.
  uint32_t tamFila = (anchoBMP * 3 + 3) & ~3;

  // Buffer para dibujar una fila completa en RGB565.
  uint16_t *fila565 = (uint16_t *)malloc(anchoBMP * sizeof(uint16_t));

  if (fila565 == NULL) {
    Serial.println("ERROR: No hay memoria para cargar fila BMP.");
    bmpFile.close();
    return false;
  }

  for (int32_t fila = 0; fila < altoBMP; fila++) {

    int32_t filaArchivo;

    if (deAbajoHaciaArriba) {
      filaArchivo = altoBMP - 1 - fila;
    } else {
      filaArchivo = fila;
    }

    uint32_t posicion = offsetDatos + (uint32_t)filaArchivo * tamFila;
    bmpFile.seek(posicion);

    for (int32_t columna = 0; columna < anchoBMP; columna++) {

      // BMP guarda B, G, R
      uint8_t b = bmpFile.read();
      uint8_t g = bmpFile.read();
      uint8_t r = bmpFile.read();

      fila565[columna] = tft.color565(r, g, b);
    }

    // Solo dibujamos filas que entren en pantalla.
    if ((y + fila) >= 0 && (y + fila) < tft.height()) {

      int16_t inicioX = x;
      int32_t columnaInicial = 0;
      int32_t cantidad = anchoBMP;

      if (inicioX < 0) {
        columnaInicial = -inicioX;
        cantidad -= columnaInicial;
        inicioX = 0;
      }

      if (inicioX + cantidad > tft.width()) {
        cantidad = tft.width() - inicioX;
      }

      if (cantidad > 0) {
        tft.drawRGBBitmap(
          inicioX,
          y + fila,
          fila565 + columnaInicial,
          cantidad,
          1
        );
      }
          actualizarCancionMenu();
    }
  }

  free(fila565);
  bmpFile.close();
  return true;
}

void actualizarOpcionesMenu() {

  tft.setTextSize(3);

  // START
  if (opcionMenu == 0) {
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.setCursor(80, 125);
    tft.println("> START");
  }
  else {
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setCursor(80, 125);
    tft.println("  START");
  }

  // SCORES
  if (opcionMenu == 1) {
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.setCursor(80, 163);
    tft.println("> SCORES");
  }
  else {
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setCursor(80, 163);
    tft.println("  SCORES");
  }
}

void menuPrincipal(){

  // Fondo completo de 320x240 desde la MicroSD.
  // Si por alguna razon no puede cargarse, dejamos fondo negro
  // para que el menu siga siendo utilizable.
  tft.fillScreen(ST77XX_BLACK);

  iniciarCancionMenu();

  bool fondoOK = dibujarBMP("/fondo.bmp", 0, 0);

  if (!fondoOK) {
    Serial.println("MENU: usando fondo negro.");
  }

  // Logo cuadrado 110x110 centrado:
  // (320 - 110) / 2 = 105
  bool logoOK = dibujarBMP("/logo.bmp", 105, 4);

  // Respaldo visual si falta el logo.
  if (!logoOK) {
    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(3);
    tft.setCursor(88, 42);
    tft.print("AVOID US");
  }
  detenerCancionMenu();

  actualizarOpcionesMenu();

  // Texto inferior original.
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(55, 211);
  tft.println("PRESIONA EL BOTON");
}

// CONTROLAR
// MENU

void controlarMenu() {
//mover hacia arriba 
  int vertical = analogRead(JOY_VER);

  if (vertical < 1200) {

    if (opcionMenu > 0) {
      opcionMenu--;

      tone(buzzer, 500, 30);

      actualizarOpcionesMenu();
    }
    delay(200);
  }
// MOVER ABAJO
  else if (vertical > 2800){
    if (opcionMenu < 1){
      opcionMenu++;

      tone(buzzer, 500, 30);

      actualizarOpcionesMenu();
    }
    delay(200);
  }

  // Botón del joystick
  if (digitalRead(JOY_SW) == LOW) {

    delay(200);

    while (digitalRead(JOY_SW) == LOW) {
      delay(10);
    }

    sonidoConfirmar();

    // START
    if (opcionMenu == 0) {

      Serial.println("START");

      modoMenu = false;
      modoTeclado = true;

      dibujarTeclado();

      // Aquí posteriormente iniciaremos el teclado
    }

    // SCORES
    if (opcionMenu == 1) {

      Serial.println("SCORES");
      modoMenu = false;
      modoScores = true;
      mostrarScores();
    }
  }
  
}

// ==========================================
// SETUP
// ==========================================

// ==========================================
// DIBUJAR PROYECTIL TIPO COHETE
// ==========================================

void dibujarProyectil(int i) {

  int x = proyectilX[i];
  int y = proyectilY[i];
  int e = escalaActualProyectil(i);

  // =====================================================
  // COHETE QUE VIENE DESDE LA IZQUIERDA ->
  // =====================================================
  if (proyectilDireccion[i] == DESDE_IZQUIERDA) {

    // Punta blanca
    tft.fillTriangle(
      x + 14 * e, y + 4 * e,
      x + 8  * e, y,
      x + 8  * e, y + 8 * e,
      ST77XX_WHITE
    );

    // Cuerpo rojo
    tft.fillRect(
      x + 3 * e,
      y + 2 * e,
      7 * e,
      5 * e,
      ST77XX_RED
    );

    // Fuego amarillo
    tft.fillTriangle(
      x + 3 * e, y + 2 * e,
      x,         y + 4 * e,
      x + 3 * e, y + 7 * e,
      ST77XX_YELLOW
    );
  }

  // =====================================================
  // COHETE QUE VIENE DESDE ARRIBA v
  // =====================================================
  else if (proyectilDireccion[i] == DESDE_ARRIBA) {

    // Punta blanca orientada hacia abajo
    tft.fillTriangle(
      x + 4 * e, y + 14 * e,
      x,         y + 8  * e,
      x + 8 * e, y + 8  * e,
      ST77XX_WHITE
    );

    // Cuerpo rojo
    tft.fillRect(
      x + 2 * e,
      y + 3 * e,
      5 * e,
      7 * e,
      ST77XX_RED
    );

    // Fuego amarillo arriba
    tft.fillTriangle(
      x + 2 * e, y + 3 * e,
      x + 4 * e, y,
      x + 7 * e, y + 3 * e,
      ST77XX_YELLOW
    );
  }

  // =====================================================
  // COHETE QUE VIENE DESDE ABAJO ^
  // =====================================================
  else {

    // Punta blanca orientada hacia arriba
    tft.fillTriangle(
      x + 4 * e, y,
      x,         y + 6 * e,
      x + 8 * e, y + 6 * e,
      ST77XX_WHITE
    );

    // Cuerpo rojo
    tft.fillRect(
      x + 2 * e,
      y + 5 * e,
      5 * e,
      7 * e,
      ST77XX_RED
    );

    // Fuego amarillo abajo
    tft.fillTriangle(
      x + 2 * e, y + 11 * e,
      x + 4 * e, y + 14 * e,
      x + 7 * e, y + 11 * e,
      ST77XX_YELLOW
    );
  }
}

void guardarScore() {

  // Guardamos el mejor score de cada jugador/iniciales.
  // Esto permite guardar al completar cada nivel sin llenar
  // el TOP 4 con seis registros del mismo jugador.
  int indiceExistente = -1;

  for (int i = 0; i < cantidadScores; i++) {
    if (topScores[i].iniciales == inicialesJugador) {
      indiceExistente = i;
      break;
    }
  }

  if (indiceExistente >= 0) {

    // Solo reemplazar si este score es mejor que el anterior.
    if (score > topScores[indiceExistente].puntos) {
      topScores[indiceExistente].puntos = score;
    }

  } else {

    RegistroScore nuevoScore;
    nuevoScore.iniciales = inicialesJugador;
    nuevoScore.puntos = score;

    // Si todavía hay espacio en el TOP.
    if (cantidadScores < MAX_SCORES) {
      topScores[cantidadScores] = nuevoScore;
      cantidadScores++;
    }

    // Si el TOP ya está lleno, reemplazar el último solo si mejora.
    else if (score > topScores[MAX_SCORES - 1].puntos) {
      topScores[MAX_SCORES - 1] = nuevoScore;
    }
  }

  // Ordenar de mayor a menor.
  for (int i = 0; i < cantidadScores - 1; i++) {
    for (int j = i + 1; j < cantidadScores; j++) {
      if (topScores[j].puntos > topScores[i].puntos) {
        RegistroScore temporal = topScores[i];
        topScores[i] = topScores[j];
        topScores[j] = temporal;
      }
    }
  }

  guardarScoresSD();
  mostrarScoresSerial();
}

void mostrarScores() {

  tft.fillScreen(ST77XX_BLACK);

  // Título
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(3);
  tft.setCursor(65, 20);
  tft.println("TOP SCORES");

  tft.setTextSize(2);

  // Mostrar los scores guardados
  for (int i = 0; i < cantidadScores; i++) {

    int y = 70 + (i * 28);

    // Posición
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(45, y);
    tft.print(i + 1);
    tft.print(". ");

    // Iniciales
    tft.setTextColor(ST77XX_WHITE);
    tft.print(topScores[i].iniciales);

    // Score
    tft.setCursor(190, y);
    tft.setTextColor(ST77XX_GREEN);
    tft.print(topScores[i].puntos);
  }

  // Si todavía no existen scores
  if (cantidadScores == 0) {
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(65, 110);
    tft.println("SIN SCORES");
  }

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);

  tft.setCursor(110, 210);
  tft.println("> VOLVER");
}

void controlarScores() {

  // Presionar SW para volver
  if (digitalRead(JOY_SW) == LOW) {

    delay(50);

    if (digitalRead(JOY_SW) == LOW) {

      while (digitalRead(JOY_SW) == LOW) {
        delay(10);
      }

      sonidoConfirmar();
      tone(buzzer, 500, 30);

      modoScores = false;
      modoMenu = true;
      opcionMenu = 0;

      menuPrincipal();
    }
  }
}

void menuVictoria() {

  tft.fillScreen(ST77XX_BLACK);

  // VICTORIA
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(4);
  tft.setCursor(65, 25);
  tft.println("VICTORIA!");

  // PUNTUACION FINAL COMPLETA
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(90, 90);
  tft.println("PUNTUACION");

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(115, 120);
  tft.println(score);

  // UNICA OPCION
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(105, 185);
  tft.println("> VOLVER");
}


void volverAlMenu() {
  detenerMusicaJuego();
  pausa = false;
  victoria = false;
  derrota = false;
  modoTeclado = false;
  modoScores = false;
  modoMenu = true;
  opcionMenu = 0;
  opcionVictoria = 0;
  opcionPausa = 0;
  textoIngresado = "";
  inicialesJugador = "";
  for (int i = 0; i < MAX_PROYECTILES; i++) proyectilActivo[i] = false;
  menuPrincipal();
}

void menuPausa() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(4);
  tft.setCursor(100, 20);
  tft.println("PAUSA");

  tft.setTextSize(3);

  // REANUDAR
  tft.setTextColor(opcionPausa == 0 ? ST77XX_YELLOW : ST77XX_WHITE);
  tft.setCursor(55, 85);
  tft.println(opcionPausa == 0 ? "> REANUDAR" : "  REANUDAR");

  // REINICIAR
  tft.setTextColor(opcionPausa == 1 ? ST77XX_YELLOW : ST77XX_WHITE);
  tft.setCursor(55, 130);
  tft.println(opcionPausa == 1 ? "> REINICIAR" : "  REINICIAR");

  // VOLVER
  tft.setTextColor(opcionPausa == 2 ? ST77XX_YELLOW : ST77XX_WHITE);
  tft.setCursor(55, 175);
  tft.println(opcionPausa == 2 ? "> VOLVER" : "  VOLVER");
}


void controlarPausa() {

  int vertical = analogRead(JOY_VER);

  // ARRIBA
  if (vertical < 1200) {
    if (opcionPausa > 0) {
      opcionPausa--;
      tone(buzzer, 500, 30);
      menuPausa();
    }
    delay(200);
  }

  // ABAJO
  else if (vertical > 2800) {
    if (opcionPausa < 2) {
      opcionPausa++;
      tone(buzzer, 500, 30);
      menuPausa();
    }
    delay(200);
  }

  // SELECCIONAR
  if (digitalRead(JOY_SW) == LOW) {

    delay(200);

    while (digitalRead(JOY_SW) == LOW) {
      delay(10);
    }

    sonidoConfirmar();

    // ========================================
    // REANUDAR
    // ========================================
    if (opcionPausa == 0) {

      unsigned long ahoraReanudar = millis();
      unsigned long tiempoPausado = ahoraReanudar - inicioPausa;

      // Congelar realmente el reloj del nivel y del score.
      inicioNivel += tiempoPausado;
      inicioPartida += tiempoPausado;

      // Congelar disparos y movimiento de cohetes.
      ultimoDisparo += tiempoPausado;

      for (int i = 0; i < MAX_PROYECTILES; i++) {
        if (proyectilActivo[i]) {
          tiempoProyectil[i] += tiempoPausado;
        }
      }

      // Congelar aparición/movimiento del power-up.
      powerUp.ultimoSpawn += tiempoPausado;

      if (powerUp.activo) {
        powerUp.ultimoMovimiento += tiempoPausado;
      }

      // Congelar temporizadores de los power-ups.
      if (multiplicadorX2) {
        x2Hasta += tiempoPausado;
        ultimoCalculoX2 += tiempoPausado;
      }

      if (camaraLenta) {
        camaraLentaHasta += tiempoPausado;
      }

      if (mostrarGlitchMenos250) {
        glitchMenos250Hasta += tiempoPausado;
      }

      pausa = false;
      reanudarMusicaJuego();

      // Restaurar escenario.
      dibujarEscenario();
      dibujarJugador(jugadorX, jugadorY);

      if (escudoActivo) {
        dibujarEscudo();
      }

      int restantes = 15 - ((millis() - inicioNivel) / 1000);

      if (restantes < 0) {
        restantes = 0;
      }

      segundoAnterior = restantes;
      dibujarContador(restantes);

      return;
    }

    // ========================================
    // REINICIAR
    // ========================================
    if (opcionPausa == 1) {

      pausa = false;
      Serial.println("PAUSA: REINICIAR");
      reiniciarJuego();
      return;
    }

    // ========================================
    // VOLVER
    // ========================================
    Serial.println("PAUSA: VOLVER AL MENU");
    volverAlMenu();
  }
}


void controlarVictoria() {

  // La pantalla de victoria solo tiene una opcion:
  // > VOLVER
  if (digitalRead(JOY_SW) == LOW) {

    delay(200);

    while (digitalRead(JOY_SW) == LOW) {
      delay(10);
    }

    sonidoConfirmar();
    Serial.println("VICTORIA: VOLVER AL MENU");
    volverAlMenu();
  }
}

// ==========================================
// DIFICULTAD POR NIVEL
// ==========================================
// La dificultad se controla con dos tiempos distintos:
// 1) intervaloDisparoActual(): cada cuanto aparece un cohete.
// 2) intervaloMovimientoActual(): cada cuanto avanza 5 pixeles.
// Un intervalo MENOR significa mas cohetes o mas velocidad.

unsigned long intervaloDisparoActual() {

  switch (nivelActual) {
    case 1: return 430; // Moderado: accesible, pero ya obliga a moverse.
    case 2: return 270; // Muchos mas cohetes.
    case 3: return 470; // Menos cohetes que nivel 2; ahora vienen de 2 lados.
    case 4: return 470; // Misma cantidad del nivel 3, pero mas rapidos.
    case 5: return 470; // Tres direcciones; la dificultad viene de rodear al jugador.
    case 6: return 270; // Misma frecuencia de aparicion que nivel 2.
    default: return 430;
  }
}

unsigned long intervaloMovimientoActual() {

  unsigned long intervalo;

  switch (nivelActual) {
    case 1: intervalo = 55; break; // ~91 px/s
    case 2: intervalo = 42; break; // ~119 px/s
    case 3: intervalo = 42; break; // MISMA velocidad que nivel 2.
    case 4: intervalo = 32; break; // ~156 px/s
    case 5: intervalo = 32; break; // Misma velocidad; ahora llegan desde tres lados.
    case 6: intervalo = 25; break; // ~200 px/s
    default: intervalo = 55; break;
  }

  // CAMARA LENTA: durante 3 segundos los cohetes avanzan
  // aproximadamente a la mitad de su velocidad normal.
  if (camaraLenta) {
if (millis() >= camaraLentaHasta) {

  camaraLenta = false;

  // Borrar indicador SLOW
  tft.fillRect(
    140,
    40,
    50,
    16,
    ST77XX_BLACK
  );

  Serial.println("CAMARA LENTA TERMINADA");

} else {

  intervalo *= 2;
}
  }

  return intervalo;
}

void configurarNuevoProyectil(int i, unsigned long ahora) {

  proyectilActivo[i] = true;

  // Solo el nivel 6 puede generar cohetes gigantes x3.
  proyectilGigante[i] = (
    nivelActual == 6 &&
    random(100) < probabilidadGiganteNivel6
  );

  // Elegir las direcciones permitidas para el nivel actual.
  if (nivelActual <= 2) {
    proyectilDireccion[i] = DESDE_IZQUIERDA;
  }
  else if (nivelActual <= 4) {
    // Nivel 3 y 4: izquierda o arriba.
    proyectilDireccion[i] = (random(2) == 0)
      ? DESDE_IZQUIERDA
      : DESDE_ARRIBA;
  }
  else {
    // Nivel 5 y 6: izquierda, arriba o abajo.
    int direccionAleatoria = random(3);

    if (direccionAleatoria == 0) {
      proyectilDireccion[i] = DESDE_IZQUIERDA;
    }
    else if (direccionAleatoria == 1) {
      proyectilDireccion[i] = DESDE_ARRIBA;
    }
    else {
      proyectilDireccion[i] = DESDE_ABAJO;
    }
  }

  int ancho = anchoActualProyectil(i);
  int alto = altoActualProyectil(i);

  // Posicion inicial segun el borde por el que entra.
  if (proyectilDireccion[i] == DESDE_IZQUIERDA) {
    proyectilX[i] = 0;
    proyectilY[i] = random(35, tft.height() - alto);
  }
  else if (proyectilDireccion[i] == DESDE_ARRIBA) {
    proyectilX[i] = random(0, tft.width() - ancho);
    proyectilY[i] = 31; // Debajo del HUD.
  }
  else {
    proyectilX[i] = random(0, tft.width() - ancho);
    proyectilY[i] = tft.height() - alto;
  }

  tiempoProyectil[i] = ahora;
}

void menuDerrota() {

  tft.fillScreen(ST77XX_BLACK);

  // Título
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(3);
  tft.setCursor(105, 35);
  tft.println("SCORE");

  // Puntuación
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(4);
  tft.setCursor(105, 80);
  tft.println(score);

  // Opciones
  tft.setTextSize(2);

  tft.setTextColor(
    opcionDerrota == 0 ? ST77XX_YELLOW : ST77XX_WHITE
  );
  tft.setCursor(75, 145);
  tft.println(
    opcionDerrota == 0 ? "> REINTENTAR" : "  REINTENTAR"
  );

  tft.setTextColor(
    opcionDerrota == 1 ? ST77XX_YELLOW : ST77XX_WHITE
  );
  tft.setCursor(75, 180);
  tft.println(
    opcionDerrota == 1 ? "> VOLVER" : "  VOLVER"
  );
}

void controlarDerrota() {

  int valorY = analogRead(JOY_VER);

  // ARRIBA
  if (valorY < 1000) {

    if (opcionDerrota > 0) {
      opcionDerrota--;
      tone(buzzer, 500, 30);
      menuDerrota();
    }

    delay(180);
  }

  // ABAJO
  if (valorY > 3000) {

    if (opcionDerrota < 1) {
      opcionDerrota++;
      tone(buzzer, 500, 30);
      menuDerrota();
    }

    delay(180);
  }

  // PRESIONAR JOYSTICK
  if (digitalRead(JOY_SW) == LOW) {

    delay(50);

    if (digitalRead(JOY_SW) == LOW) {

      while (digitalRead(JOY_SW) == LOW) {
        delay(10);
      }

      sonidoConfirmar();

      // REINTENTAR
      if (opcionDerrota == 0) {

        derrota = false;
        reiniciarJuego();
      }

      // VOLVER
      else {

        derrota = false;
        modoMenu = true;
        opcionMenu = 0;
        menuPrincipal();
      }
    }
  }
}

// ==========================================
// GUARDAR SCORES EN JSON - MICRO SD
// ==========================================

void guardarScoresSD() {

  // "w" reemplaza el JSON anterior por el ranking actualizado
  File archivo = SD.open("/scores.json", "w");

  if (!archivo) {
    Serial.println("ERROR: No se pudo abrir scores.json");
    return;
  }

  archivo.println("{");
  archivo.println("  \"scores\": [");

  for (int i = 0; i < cantidadScores; i++) {

    archivo.print("    {\"iniciales\":\"");
    archivo.print(topScores[i].iniciales);
    archivo.print("\",\"puntos\":");
    archivo.print(topScores[i].puntos);
    archivo.print("}");

    // Todos menos el ultimo necesitan coma
    if (i < cantidadScores - 1) {
      archivo.print(",");
    }

    archivo.println();
  }

  archivo.println("  ]");
  archivo.println("}");

  archivo.close();

  Serial.println("Scores guardados en /scores.json");
}

// ==========================================
// CARGAR SCORES DESDE JSON - MICRO SD
// ==========================================

void cargarScoresSD() {

  cantidadScores = 0;

  File archivo = SD.open("/scores.json", FILE_READ);

  if (!archivo) {
    Serial.println("No existe scores.json todavia");
    return;
  }

  while (archivo.available() && cantidadScores < MAX_SCORES) {

    String linea = archivo.readStringUntil('\n');
    linea.trim();

    // Buscar solamente las lineas que contienen un score
    if (linea.indexOf("\"iniciales\"") >= 0 &&
        linea.indexOf("\"puntos\"") >= 0) {

      // ------------------------------
      // LEER INICIALES
      // ------------------------------

      int inicioIniciales =
        linea.indexOf("\"iniciales\":\"");

      if (inicioIniciales < 0) {
        continue;
      }

      inicioIniciales += 13;

      int finIniciales =
        linea.indexOf("\"", inicioIniciales);

      if (finIniciales < 0) {
        continue;
      }

      String iniciales =
        linea.substring(inicioIniciales, finIniciales);

      // ------------------------------
      // LEER PUNTOS
      // ------------------------------

      int inicioPuntos =
        linea.indexOf("\"puntos\":");

      if (inicioPuntos < 0) {
        continue;
      }

      inicioPuntos += 9;

      int finPuntos =
        linea.indexOf("}", inicioPuntos);

      if (finPuntos < 0) {
        continue;
      }

      unsigned long puntos =
        linea.substring(inicioPuntos, finPuntos).toInt();

      // ------------------------------
      // GUARDAR EN RAM
      // ------------------------------

      topScores[cantidadScores].iniciales = iniciales;
      topScores[cantidadScores].puntos = puntos;

      cantidadScores++;
    }
  }

  archivo.close();

  Serial.print("Scores cargados desde JSON: ");
  Serial.println(cantidadScores);

  mostrarScoresSerial();
}

void mostrarScoresSerial(){

  Serial.println();
  Serial.println("======================");
  Serial.println("   AVOID US - SCORES");
  Serial.println("======================");

  for (int i = 0; i < MAX_SCORES; i++) {

    Serial.print(i + 1);
    Serial.print(". ");

    if (topScores[i].puntos > 0) {

      Serial.print(topScores[i].iniciales);
      Serial.print(" - ");
      Serial.println(topScores[i].puntos);

    } else {

      Serial.println("---");
    }
  }

  Serial.println("======================");
}

void setup() {

  Serial.begin(115200);

// ==========================================
// INICIAR MICRO SD
// ==========================================

if (!SD.begin(SD_CS)) {
  Serial.println("ERROR: No se pudo iniciar la MicroSD");
} 
else {
  Serial.println("MicroSD detectada correctamente");

  cargarScoresSD();
}


  // ----------------------------------------
  // TFT
  // ----------------------------------------

  tft.init(240, 320);

  tft.invertDisplay(false);

  // Horizontal = 320 x 240
  tft.setRotation(1);

  // ----------------------------------------
  // JOYSTICK
  // ----------------------------------------

  pinMode(JOY_HOR, INPUT);
  pinMode(JOY_VER, INPUT);
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(PAUSE_BUTTON, INPUT_PULLUP);

  // ----------------------------------------
  // JUGADOR EN EL CENTRO
  // ----------------------------------------

  jugadorX =
    (tft.width() - jugadorAncho) / 2;

  jugadorY =
    (tft.height() - jugadorAlto) / 2;


  // ----------------------------------------
  // PROYECTILES
  // ----------------------------------------

  for (int i = 0; i < MAX_PROYECTILES; i++) {

    proyectilActivo[i] = false;
  }


  // ----------------------------------------
  // INICIAR TECLADO VIRTUAL
  // ----------------------------------------

  filaTeclado = 0;
  columnaTeclado = 0;
  textoIngresado = "";
  modoMenu = true;
  modoTeclado = false;

  // Mantener el menu visible al iniciar
  menuPrincipal();

  Serial.println("MENU INICIADO");
}


// ==========================================
// LOOP
// ==========================================

  void loop() {

      if (modoMenu) {
      controlarMenu();
      return;
    }

    if (modoScores) {
      if (digitalRead(JOY_SW) == LOW) {
        delay(200);
        while (digitalRead(JOY_SW) == LOW) delay(10);
        sonidoConfirmar();
        modoScores = false;
        modoMenu = true;
        opcionMenu = 0;
        menuPrincipal();
      }
      return;
    }
  // ==========================================
  // TECLADO VIRTUAL

    if (modoTeclado) {
      controlarTeclado();
      return;
    }

    // ==========================================
  // BOTON DE PAUSA
  // ==========================================

  if (!pausa && !victoria && !derrota) {

    if (digitalRead(PAUSE_BUTTON) == LOW) {

      delay(50);  // Antirrebote

      if (digitalRead(PAUSE_BUTTON) == LOW) {

        // Esperar a soltar el boton
        while (digitalRead(PAUSE_BUTTON) == LOW) {
          delay(10);
        }

        pausarMusicaJuego();
        sonidoPausa();

        pausa = true;
        inicioPausa = millis();
        opcionPausa = 0;

        menuPausa();

        return;
      }
    }
  }


  // ==========================================
  // DERROTA
  // ==========================================

if (derrota) {
  controlarDerrota();
  return;
}


// ==========================================
// VICTORIA
// ==========================================

if (victoria) {
  controlarVictoria();
  return;
}

if (pausa) {
  controlarPausa();
  return;
}

  unsigned long ahora = millis();
  actualizarMusicaJuego();

// ==========================================
// GENERAR POWER-UPS
// ==========================================

powerUp.aparecer();
powerUp.mover();
revisarPowerUp();

  // ========================================
  // CONTADOR 15 → 0
  // ========================================

  unsigned long transcurrido =
    ahora - inicioNivel;

  int segundosRestantes =
    15 - (transcurrido / 1000);

    
  if (segundosRestantes < 0) {
    segundosRestantes = 0;
  }


  actualizarPuntuacion();

  // HUD NIVEL, TIEMPO Y SCORE

  tft.fillRect(0, 0, 320, 30, ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  //NIVEL
  tft.setCursor(5, 7);
tft.print("N:");
tft.print(nivelActual);

// TIEMPO
tft.setCursor(155, 7);
tft.print("T:");
tft.print(segundosRestantes);

// SCORE
tft.setCursor(235, 7);
tft.print("P:");
tft.print(score);

// Indicador temporal del GLITCH (-250).
actualizarIndicadorGlitch();


  // HUD NIVEL / TIEMPO / SCORES
if (segundosRestantes != segundoAnterior) {

  segundoAnterior = segundosRestantes;

  Serial.print("Nivel: ");
  Serial.print(nivelActual);

  Serial.print(" | Tiempo: ");
  Serial.print(segundosRestantes);

  Serial.print(" | Score: ");
  Serial.println(score);
}


  // ========================================
  // VICTORIA
  // ========================================

if (segundosRestantes <= 0) {

  // Si todavía faltan niveles
  if (nivelActual < totalNiveles) {

    // Guardar el marcador al completar cada nivel.
    // guardarScore() conserva solo el mejor valor de estas iniciales.
    actualizarPuntuacion();
    guardarScore();

    nivelActual++;
    inicioNivel = millis();

    Serial.print("NIVEL COMPLETADO. Ahora inicia nivel ");
    Serial.println(nivelActual);

  }
  // Si terminó el nivel 6
  else {

    // La victoria depende de sobrevivir los 6 niveles, NO de llegar a 9000 puntos.
    // Al completar los 90 segundos, los puntos base quedan exactamente en 9000.
    // Calcular el SCORE COMPLETO: base + bonus + X2 - penalizaciones.
    actualizarPuntuacion();

    // Asegurar que los 90 segundos completos aporten los 9000 puntos base.
    puntosBase = PUNTOS_BASE_MAX;
    long totalFinal =
      (long)puntosBase +
      (long)puntosBonus +
      (long)puntosExtraX2 -
      (long)penalizacionGlitch;
    if (totalFinal < 0) totalFinal = 0;
    score = (unsigned long)totalFinal;

    guardarScore();

    detenerMusicaJuego();
    sonidoVictoria();

    victoria = true;
    opcionVictoria = 0;

    // Mostrar la pantalla de victoria.
    menuVictoria();
  }

  return;
}


  // ========================================
  // LEER JOYSTICK
  // ========================================

  int horizontal = analogRead(JOY_HOR);
  int vertical = analogRead(JOY_VER);


  int nuevaX = jugadorX;
  int nuevaY = jugadorY;



  // ========================================
  // MOVIMIENTO HORIZONTAL
  // ========================================

  if (horizontal < 1200) {

    nuevaX -= velocidadJugador;
  }

  else if (horizontal > 2800) {

    nuevaX += velocidadJugador;
  }


  // ========================================
  // MOVIMIENTO VERTICAL
  // ========================================

  if (vertical < 1200) {

    nuevaY -= velocidadJugador;
  }

  else if (vertical > 2800) {

    nuevaY += velocidadJugador;
  }


  // ========================================
  // LÍMITES
  // ========================================

  // X: toda la pantalla
  nuevaX = constrain(
    nuevaX,
    0,
    tft.width() - jugadorAncho
  );


  // Y: dejamos 35 píxeles arriba
  // reservados para el contador
  nuevaY = constrain(
    nuevaY,
    35,
    tft.height() - jugadorAlto
  );


// ========================================
// ACTUALIZAR JUGADOR
// ========================================

if (
  nuevaX != jugadorX ||
  nuevaY != jugadorY
) {

  // Borrar el escudo de la posicion anterior
  if (escudoActivo) {

    int centroXAnterior = jugadorX + (jugadorAncho / 2);
    int centroYAnterior = jugadorY + (jugadorAlto / 2);

    tft.drawCircle(
      centroXAnterior,
      centroYAnterior,
      18,
      ST77XX_BLACK
    );

    tft.drawCircle(
      centroXAnterior,
      centroYAnterior,
      19,
      ST77XX_BLACK
    );
  }

  // Borrar jugador de la posicion anterior
  borrarJugador(
    jugadorX,
    jugadorY
  );

  // Actualizar posicion
  jugadorX = nuevaX;
  jugadorY = nuevaY;

  // Dibujar jugador en la nueva posicion
  dibujarJugador(
    jugadorX,
    jugadorY
  );

  // Dibujar el escudo alrededor de la nueva posicion
  if (escudoActivo) {
    dibujarEscudo();
  }

  // Sonido cada que el jugador se mueve
  paso(); // DESACTIVADO temporalmente
}

  // ========================================
  // CREAR PROYECTIL
  // ========================================

  if (
    ahora - ultimoDisparo
    >= intervaloDisparoActual()
  ) {

    for (int i = 0; i < MAX_PROYECTILES; i++) {

      if (!proyectilActivo[i]) {
        configurarNuevoProyectil(i, ahora);
        break;
      }
    }

    ultimoDisparo = ahora;
  }


  // ========================================
  // MOVER PROYECTILES
  // ========================================

  for (int i = 0; i < MAX_PROYECTILES; i++) {

    if (proyectilActivo[i]) {

      if (
        ahora - tiempoProyectil[i]
        >= intervaloMovimientoActual()
      ) {

        // Borrar proyectil anterior
        borrarProyectil(i);

        // Mover 5 pixeles en la direccion correspondiente.
        if (proyectilDireccion[i] == DESDE_IZQUIERDA) {
          proyectilX[i] += 5;
        }
        else if (proyectilDireccion[i] == DESDE_ARRIBA) {
          proyectilY[i] += 5;
        }
        else {
          proyectilY[i] -= 5;
        }

        int ancho = anchoActualProyectil(i);
        int alto = altoActualProyectil(i);

        // ==================================
        // COLISIÓN
        // ==================================

        if (
          proyectilX[i] < jugadorX + jugadorAncho &&
          proyectilX[i] + ancho > jugadorX &&
          proyectilY[i] < jugadorY + jugadorAlto &&
          proyectilY[i] + alto > jugadorY
        ) {
          
          // PRUEBAS
          if (jugadorInvenciblePrueba){
            proyectilActivo[i] = false;
            dibujarJugador(jugadorX, jugadorY);
            
            continue;
          }
          // ==================================
          // ESCUDO
          // ==================================
          // Protege de UN cohete normal. Los cohetes gigantes del nivel 6
          // atraviesan el escudo y provocan derrota directamente.
          if (escudoActivo && !proyectilGigante[i]) {
            escudoActivo = false;

            // Centro actual del jugador
            int centroX = jugadorX + (jugadorAncho / 2);
            int centroY = jugadorY + (jugadorAlto / 2);

            // Borrar visualmente el circulo del escudo
            tft.drawCircle(centroX, centroY, 18, ST77XX_BLACK);
            tft.drawCircle(centroX, centroY, 19, ST77XX_BLACK);

            // Destruir el cohete que golpeo
            borrarProyectil(i);
            proyectilActivo[i] = false;

            Serial.println("ESCUDO: IMPACTO BLOQUEADO");

            // Restaurar al jugador
            dibujarJugador(jugadorX, jugadorY);

            continue;
          }

          // Si no tenia escudo, pierde
          actualizarPuntuacion();
          guardarScore();

          derrota = true;
          opcionDerrota = 0;

          Serial.print("SCORE: ");
          Serial.println(score);

          detenerMusicaJuego();
          sonidoDerrota();
          menuDerrota();
          return;
        }

        // ==================================
        // SALIÓ DE LA PANTALLA
        // ==================================

        bool salioPantalla = false;

        if (proyectilDireccion[i] == DESDE_IZQUIERDA) {
          salioPantalla = proyectilX[i] >= tft.width();
        }
        else if (proyectilDireccion[i] == DESDE_ARRIBA) {
          salioPantalla = proyectilY[i] >= tft.height();
        }
        else {
          // Se elimina al llegar al limite inferior del HUD.
          salioPantalla = proyectilY[i] + alto <= 31;
        }

        if (salioPantalla) {
          proyectilActivo[i] = false;
        }
        else {
          dibujarProyectil(i);
        }

        tiempoProyectil[i] = ahora;
      }
    }
  }

  // ==========================================
  // CAPA SUPERIOR DEL HUD DE POWER-UPS
  // ==========================================
  // Se dibuja DESPUES de jugador y cohetes.
  // Por eso los cohetes pasan visualmente por debajo del texto.
  dibujarIndicadoresPowersSobrepuestos();

  delay(20);
}