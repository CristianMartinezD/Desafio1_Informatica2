#include <LiquidCrystal.h>  // Librería para el control de la pantalla LCD

// Configuración de los pines para la pantalla LCD (12, 11, 5, 4, 3, 2)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Definimos los pines y variables globales
const int pinEntradaAnalogica = A0;  // Entrada analógica para la señal
const int pinBotonInicio = 7;        // Botón para iniciar la adquisición de datos
const int pinBotonMostrar = 8;       // Botón para mostrar información

// Variables para la adquisición de señal
float* amplitudes;   // Puntero para el arreglo dinámico de amplitudes
float* segundasDerivadas; // Puntero para almacenar las segundas derivadas
int tamanoMuestras = 60;  // Número de muestras para la señal
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

  // Inicializar memoria dinámica para las amplitudes y segundas derivadas
  amplitudes = new float[tamanoMuestras];  // Reserva memoria para el arreglo dinámico
  segundasDerivadas = new float[tamanoMuestras - 2]; // Reserva para las segundas derivadas
  
  // Inicializar comunicación serial
  Serial.begin(9600);  // Configura la velocidad de comunicación serial a 9600 baudios
}

// Función principal que corre en bucle
void loop() {
  // Verificar si el botón de inicio fue presionado (se leerá LOW cuando esté presionado)
  if (digitalRead(pinBotonInicio) == LOW) {
    lcd.clear();
    lcd.print("Adquiriendo...");
    adquirirSenal();  // Ahora adquirimos las 60 muestras cuando se presiona el botón
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
  // Leer 60 muestras consecutivamente
  for (int i = 0; i < tamanoMuestras; i++) {
    // Lee el valor de la señal analógica y lo almacena en el arreglo
    amplitudes[i] = analogRead(pinEntradaAnalogica);
    Serial.println(amplitudes[i]);  // Imprimir las amplitudes en el monitor serial

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
    }

    // Introducir un pequeño retardo entre lecturas para evitar que se tomen todas al instante
    delay(10);
  }
}

// Función para calcular la segunda derivada (en valor absoluto) y contar cuántos elementos son menores que 5
void identificarFormaOndaSegundaDerivada() {
  int countMenores5 = 0;  // Contador para elementos menores que 5

  // Calcular la segunda derivada en valor absoluto
  for (int i = 2; i < tamanoMuestras; i++) {
    // Calcular la segunda derivada como la magnitud absoluta de las diferencias
    float segundaDerivada = abs(amplitudes[i]) - 2 * abs(amplitudes[i - 1]) + abs(amplitudes[i - 2]);
    segundasDerivadas[i - 2] = abs(segundaDerivada);  // Guardar la segunda derivada en valor absoluto

    // Si la segunda derivada es menor que 5, incrementar el contador
    if (abs(segundaDerivada) < 5) {
      countMenores5++;
    }
  }

  // Imprimir las segundas derivadas en el monitor serial
  Serial.println("Segundas derivadas:");
  for (int i = 0; i < tamanoMuestras - 2; i++) {
    Serial.print(segundasDerivadas[i]);
    if (i < tamanoMuestras - 3) Serial.print(", ");  // Imprimir coma entre elementos
  }
  Serial.println();  // Nueva línea al final del arreglo

  // Identificar la forma de la onda basada en el número de elementos menores que 5
  if (countMenores5 > 10) {
    formaOnda = "Triangular";
  } else {
    formaOnda = "Senoidal";
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
  // Identificar la forma de la onda usando la segunda derivada
  identificarFormaOndaSegundaDerivada();

  // Calcular amplitud pico a pico
  amplitud = (amplitudMax - amplitudMin) * (5.0 / 1023.0);  // Convertir a voltios

  // Mostrar características cíclicamente hasta que el usuario pulse el botón de inicio
  int contador = 0;
  while (digitalRead(pinBotonInicio) == HIGH && contador != 6) {
    contador++;
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
  delete[] segundasDerivadas;
}
