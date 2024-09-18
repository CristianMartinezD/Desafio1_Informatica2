#include <LiquidCrystal.h>  // Librería para el control de la pantalla LCD

// Configuración de los pines para la pantalla LCD (12, 11, 5, 4, 3, 2)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Definimos los pines y variables globales
const int pinEntradaAnalogica = A0;  // Entrada analógica para la señal
const int pinBotonInicio = 7;
const int pinBotonMostrar = 8;

// Variables para la adquisición de señal
int* amplitudes;   // Puntero para el un arreglo dinámico de amplitudes
int tamanoMuestras = 60;
int amplitudMax = 0;
int amplitudMin = 1023;
float amplitud = 0;
float frecuencia = 0;
float amplitudPromedio = 0;  // Variable para almacenar el promedio de la señal
bool cruceDetectado = false;  // Indicar si un cruce fue detectado
unsigned long tiempoAnterior = 0;
String formaOnda = "Desconocida";  // Variable para guardar la forma de la onda
bool mostrarYa = false;


bool mostrarCaracteristicas = true;  // Control para alternar entre características

// Arreglo para almacenar las segundas derivadas
int* segundasDerivadas; 


void setup() {
  // Inicializamos la pantalla LCD
  lcd.begin(16, 2);
  lcd.print("Esperando...");  // Muestra un mensaje de espera

  // Configuramos los pines de los botones con resistencias pull-up internas
  pinMode(pinBotonInicio, INPUT_PULLUP);
  pinMode(pinBotonMostrar, INPUT_PULLUP);

  // Inicializamos memoria dinámica para las amplitudes y segundas derivadas
  amplitudes = new int[tamanoMuestras];
  segundasDerivadas = new int[tamanoMuestras - 2];
  
  // Inicializamos la comunicación serial
  Serial.begin(9600);
}


// Función principal que corre en bucle
void loop() {
  // Verificamos si el botón de inicio fue presionado (se leerá LOW cuando esté presionado)
  if (digitalRead(pinBotonInicio) == LOW) {
    lcd.clear();
    lcd.print("Adquiriendo...");
    adquirirSenal();  // Ahora adquirimos las 60 muestras cuando se presiona el botón
    obtenerFR();
    mostrarYa = true;
    delay(200);  // Pequeño retardo para evitar lecturas múltiples al presionar el botón
  }

  // Verificamos si el botón de mostrar fue presionado
  if (digitalRead(pinBotonMostrar) == LOW && mostrarYa == true) {
    mostrarResultados();  // Mostrar los resultados en pantalla
    mostrarYa = false;
    delay(200);  // Pequeño retardo para evitar lecturas múltiples
  }
}



// Función para la adquisición de la señal analógica
void adquirirSenal() {
  // Leer 60 muestras consecutivamente
  for (int i = 0; i < tamanoMuestras; i++) {
    // Lee el valor de la señal analógica y lo almacena en el arreglo
    amplitudes[i] = analogRead(pinEntradaAnalogica);
    Serial.println(amplitudes[i]);
    
  }
}




// Función para el cálculo de la frecuencia
void obtenerFR() {
  amplitudMax = 0;  // Reiniciar valores máximos y mínimos para esta adquisición
  amplitudMin = 1023;
  int* muestras;
  muestras = new int[tamanoMuestras];
  
  // Leer 60 muestras consecutivamente
  for (int i = 0; i < tamanoMuestras; i++) {
    muestras[i] = analogRead(pinEntradaAnalogica);

    // Actualizar el valor máximo y mínimo de la señal
    if (muestras[i] > amplitudMax) amplitudMax = muestras[i];
    if (muestras[i] < amplitudMin) amplitudMin = muestras[i];

    amplitudPromedio = (amplitudMax + amplitudMin) / 2.0;  // Calcular el promedio de la señal

    // Detección de cruce por el promedio (mejora en cruce por cero)
    if (!cruceDetectado && muestras[i] > amplitudPromedio) {
      cruceDetectado = true;  // Cruce detectado
      unsigned long tiempoActual = millis();  // Obtener el tiempo actual
      
      if (tiempoAnterior != 0) {
        // Calcular el tiempo de un ciclo completo (frecuencia)
        unsigned long duracionCiclo = tiempoActual - tiempoAnterior;
        frecuencia = 1000.0 / duracionCiclo;  // Convertir a Hz
      }
      tiempoAnterior = tiempoActual;  // Guardar el tiempo del cruce actual
    }

    // Restablecer el estado de cruce detectado al bajar de nuevo
    if (muestras[i] < amplitudPromedio) {
      cruceDetectado = false;
    }

    // Pequeño retardo entre lecturas para evitar que se tomen todas al instante
    delay(10);
  }
  delete[] muestras; 
}



// Función para identificar la forma de la onda basada en el análisis de la segunda derivada
void identificarFormaOndaSegundaDerivada() {
  int countMenores5 = 0;
  int countCero = 0;      // Contador para segundas derivadas cercanas a 0

  // Calculamos la segunda derivada en valor absoluto
  for (int i = 2; i < tamanoMuestras; i++) {
    float segundaDerivada = abs(amplitudes[i]) - 2 * abs(amplitudes[i - 1]) + abs(amplitudes[i - 2]);
    segundasDerivadas[i - 2] = abs(segundaDerivada);  // Guardamos la segunda derivada en valor absoluto

    if (abs(segundaDerivada) < 5) countMenores5++;
    if (abs(segundaDerivada) < 0.01) countCero++;
  }

  Serial.println("Segundas derivadas:");
  for (int i = 0; i < tamanoMuestras - 2; i++) {
    Serial.print(segundasDerivadas[i]);
    if (i < tamanoMuestras - 3) Serial.print(", ");
  }
  Serial.println();
  int umbral = frecuencia*3; //Sirve para determinar el minimo # de elementos menores a 5 que debe contener el arreglo de las segundas derivadas para determinar la forma de la onda.

  if (countCero > 0.9 * (tamanoMuestras - 2)) {
    formaOnda = "Cuadrada";
  } else if (countMenores5 >= umbral) {
    formaOnda = "Triangular";
  } else if (countMenores5 < umbral && countCero <= 0.9 * (tamanoMuestras - 2)) {
    formaOnda = "Senoidal";
  } else {
    formaOnda = "Desconocida";
  }
}



// Función para mostrar las características de la señal cíclicamente en la pantalla LCD
void mostrarCaracteristicasCiclicas() {
  if (mostrarCaracteristicas) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Forma Onda: ");
    lcd.setCursor(0, 1);
    lcd.print(formaOnda);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Freq: ");
    lcd.print(frecuencia);
    lcd.print("Hz");
    
    lcd.setCursor(0, 1);
    lcd.print("Amp: ");
    lcd.print(amplitud);
    lcd.print("V");
  }

  mostrarCaracteristicas = !mostrarCaracteristicas;
  delay(2000);
}



// Función para mostrar los resultados en la pantalla LCD
void mostrarResultados() {
  identificarFormaOndaSegundaDerivada();

  amplitud = (amplitudMax - amplitudMin) * (5.0 / 1023.0);  // Cálculamos la amplitud y convertimos a voltios

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

  amplitudMax = 0;
  amplitudMin = 1023;
  tiempoAnterior = 0;
}

// Función para liberar la memoria asignada dinámicamente al finalizar el programa
void finalizar() {
  delete[] amplitudes; 
  delete[] segundasDerivadas;
}
