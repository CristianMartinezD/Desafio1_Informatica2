#include <LiquidCrystal.h>  // Librería para el control de la pantalla LCD

// Configuración de los pines para la pantalla LCD (12, 11, 5, 4, 3, 2)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Definimos los pines y variables globales
const int pinEntradaAnalogica = A0;  // Entrada analógica para la señal
const int pinBotonInicio = 7;        // Botón para iniciar la adquisición de datos
const int pinBotonMostrar = 8;       // Botón para mostrar información

// Variables para la adquisición de señal
float* amplitudes;   // Puntero para el arreglo dinámico de amplitudes
int tamanoMuestras = 40;  // Número de muestras para la señal
float amplitudMax = 0;
float amplitudMin = 1023;
float amplitud = 0;
float frecuencia = 0;
unsigned long tiempoAnterior = 0;
String formaOnda = "Desconocida";  // Variable para guardar la forma de la onda

// Variables de control de pantalla
bool mostrarCaracteristicas = true;  // Control para alternar entre características

// Función para inicializar el sistema
void setup() {
  // Inicialización de la pantalla LCD
  lcd.begin(16, 2);  // Configura una pantalla de 16 columnas y 2 filas
  lcd.print("Esperando...");  // Muestra un mensaje de espera

  // Configuración de los pines de los botones con resistencias pull-up internas
  pinMode(pinBotonInicio, INPUT_PULLUP);
  pinMode(pinBotonMostrar, INPUT_PULLUP);

  // Inicializar memoria dinámica para las amplitudes
  amplitudes = new float[tamanoMuestras];  // Reserva memoria para el arreglo dinámico
  
  // Inicializar comunicación serial
  Serial.begin(9600);  // Configura la velocidad de comunicación serial a 9600 baudios
}



// Función principal que corre en bucle
void loop() {
  // Verificar si el botón de inicio fue presionado (se leerá LOW cuando esté presionado)
  if (digitalRead(pinBotonInicio) == LOW) {
    lcd.clear();
    lcd.print("Adquiriendo...");
    adquirirSenal();  // Ahora adquirimos las 40 muestras cuando se presiona el botón
    delay(200);  // Pequeño retardo para evitar lecturas múltiples al presionar el botón
  }

  // Verificar si el botón de mostrar fue presionado
  if (digitalRead(pinBotonMostrar) == LOW) {
    mostrarResultados();  // Mostrar los resultados en pantalla
    delay(200);  // Pequeño retardo para evitar lecturas múltiples
  }
}




// Función para la adquisición de la señal analógica
void adquirirSenal() {
  // Leer 40 muestras consecutivamente
  for (int i = 0; i < tamanoMuestras; i++) {
    // Lee el valor de la señal analógica y lo almacena en el arreglo
    amplitudes[i] = analogRead(pinEntradaAnalogica);

    // Actualizar el valor máximo y mínimo de la señal
    if (amplitudes[i] > amplitudMax) {
      amplitudMax = amplitudes[i];  // Actualizar valor máximo
    }
    if (amplitudes[i] < amplitudMin) {
      amplitudMin = amplitudes[i];  // Actualizar valor mínimo
    }

    // Calcular el tiempo entre picos (para la frecuencia)
    unsigned long tiempoActual = millis();
    if (amplitudes[i] > 512 && tiempoActual - tiempoAnterior > 20) {  // Detectar un cruce por cero
      frecuencia = 1000.0 / (tiempoActual - tiempoAnterior);  // Calcular frecuencia en Hz
      tiempoAnterior = tiempoActual;

      // Imprimir la frecuencia en el monitor serial (Esto es para ver que todo va bien)
      Serial.print("Frecuencia: ");
      Serial.print(frecuencia);
      Serial.println(" Hz");
    }

    // Introducir un pequeño retardo entre lecturas para evitar que se tomen todas al instante
    delay(10);
  }
}

// Función para identificar la forma de la onda
void identificarFormaOnda() {
  int cambiosAbruptos = 0;
  int pendientesPositivas = 0;
  int pendientesNegativas = 0;
  bool pendienteConstante = true;  // Para detectar pendientes lineales (triangular)

  // Analizamos las muestras en el arreglo para detectar la forma de la onda
  for (int i = 1; i < tamanoMuestras; i++) {
    float pendiente = amplitudes[i] - amplitudes[i - 1];

    // Contar cuántas veces hay un cambio de pendiente brusco
    if (abs(pendiente) > 200) {
      cambiosAbruptos++;
    }

    // Contar cuántas veces la pendiente es positiva o negativa
    if (pendiente > 0) {
      pendientesPositivas++;
    } else if (pendiente < 0) {
      pendientesNegativas++;
    }

    // Detectar si la pendiente es constante, lo que indicaría una onda triangular
    if (i > 1) {
      float pendienteAnterior = amplitudes[i - 1] - amplitudes[i - 2];
      if (abs(pendiente - pendienteAnterior) > 10) {  // Detectar si la pendiente cambia drásticamente
        pendienteConstante = false;
      }
    }
  }

  // Identificar forma de onda basándonos en los cambios en las pendientes
  if (cambiosAbruptos > tamanoMuestras * 0.5) {
    formaOnda = "Cuadrada";  // Muchos cambios bruscos indican onda cuadrada
  } else if (pendienteConstante && pendientesPositivas == pendientesNegativas) {
    formaOnda = "Triangular";  // Si la pendiente es constante y sube/baja de manera equilibrada
  } else {
    formaOnda = "Senoidal";  // Si las pendientes cambian suavemente, es onda senoidal
  }
}

// Función para mostrar las características de la señal cíclicamente en la pantalla LCD
void mostrarCaracteristicasCiclicas() {
  if (mostrarCaracteristicas) {
    // Mostrar la forma de la onda
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Forma Onda: ");
    lcd.setCursor(0, 1);
    lcd.print(formaOnda);
  } else {
    // Mostrar la amplitud y la frecuencia
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Amp: ");
    lcd.print(amplitud);
    lcd.print("V");
    
    lcd.setCursor(0, 1);
    lcd.print("Freq: ");
    lcd.print(frecuencia);
    lcd.print(" Hz");
  }

  mostrarCaracteristicas = !mostrarCaracteristicas;  // Alternar entre características
  delay(2000);  // Pausa de 2 segundos antes de cambiar la pantalla
}

// Función para mostrar los resultados en la pantalla LCD
void mostrarResultados() {

  // Identificar la forma de la onda
  identificarFormaOnda();

  // Calcular amplitud pico a pico
  amplitud = (amplitudMax - amplitudMin) * (5.0 / 1023.0);  // Convertir a voltios

  // Mostrar características cíclicamente hasta que el usuario pulse el botón de inicio
  int contador = 0;
  while (digitalRead(pinBotonInicio) == HIGH && contador != 6) {
    contador = contador+1;
    mostrarCaracteristicasCiclicas();
  }
  
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pulsa el boton:");
    
    lcd.setCursor(0, 1);
    lcd.print("INICIO");

  // Reiniciar las variables de adquisición
  amplitudMax = 0;
  amplitudMin = 1023;
  tiempoAnterior = 0;
}




// Función para liberar la memoria asignada dinámicamente al finalizar el programa
void finalizar() {
  delete[] amplitudes; 
}
