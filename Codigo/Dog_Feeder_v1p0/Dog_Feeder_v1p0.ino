/*
 * GLIK Costa Rica
 * Hecho por Daniel Castro Sánchez
 * 
 * Reloj con programas para mover un servomotor
 * Se maneja con cuatro pushbutton.
 * Permite guardar varias programaciones, con 
 * la cantidad de ciclos deseados del servomotor
 */

#include <LiquidCrystal.h>
#include <Servo.h>

const int maximoProgramas = 14; //El maximo realmente es maximoProgramas+1

int hora = 12; //Hora del reloj principal
int minuto = 0; //Minutos del reloj proncipal
int seg = 0; //Segundos del reloj principal
bool flag = true; // AM-PM: AM true, PM false
int menuGeneral = 0; //Opciones del menu principal
bool menuInterno = false; //Entrar en la opcion seleccionada del menu principal
int horaPrograma[maximoProgramas]; //hora de todos los programas guardados
int minutoPrograma[maximoProgramas]; //minutos de todos los programas guardados
int flagPrograma[maximoProgramas]; //AM/PM de todos los programas guardados
int porciones[maximoProgramas]; //Porciones de alimento para cada vez que se activan los programas
int programasGuardados = 0; //Cantidad de programas guardados de 0 a 10
unsigned long tiempoEspera = 70000; // contador para apagar el LCD

/*En proto:
const int servomot = 11;
const int botonArriba = 10, botonAbajo = 12, botonSiguiente = 7, botonAtras = 2;
const int rs = 3, en = 5, d4 = 4, d5 = 6, d6 = 8, d7 = 9;
*/
const int lcdLight = A3; //Control de la luz del LCD. LOW encendido y HIGH apagado
const int servomot = 6;
const int buzzer = A0;
const int botonArriba = 7, botonAbajo = 5, botonSiguiente = 12, botonAtras = 4;
const int rs = 2, en = 3, d4 = 8, d5 = 9, d6 = 10, d7 = 11;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
Servo elServo;

byte arrowUp[8] =
{
  B00100,
  B01110,
  B11111,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
};

byte arrowDown[8] =
{
  B00100,
  B00100,
  B00100,
  B00100,
  B11111,
  B11111,
  B01110,
  B00100,
};

void setup()
{
  pinMode(botonArriba, INPUT_PULLUP);
  pinMode(botonAbajo, INPUT_PULLUP);
  pinMode(botonSiguiente, INPUT_PULLUP);
  pinMode(botonAtras, INPUT_PULLUP);
  pinMode(lcdLight, OUTPUT);
  digitalWrite(lcdLight, LOW);
  
  lcd.createChar(0, arrowUp); //Crea los simbolos de flecha
  lcd.createChar(1, arrowDown);
  lcd.begin(16, 2); // configuracion del LCD con 16 columnas y 2 filas
  
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  
  lcd.setCursor(3,0);
  lcd.print("Dog Feeder");
  lcd.setCursor(6,1);
  lcd.print("GLIK");

  elServo.attach(servomot);
  elServo.write(180); //Mueve el servo al origen

  //Suena el buzzer tres veces
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(900);
  digitalWrite(buzzer, LOW);
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(900);
  digitalWrite(buzzer, LOW);
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(2900);

  lcd.setCursor(1,0);
  lcd.print("Ajuste la hora");
  lcd.setCursor(0,1);
  lcd.print("Oprima un boton");
  while (true)
  {
    if((!digitalRead(botonSiguiente)) || (!digitalRead(botonAtras)) || (!digitalRead(botonArriba)) || (!digitalRead(botonAbajo)))
    {
      delay(200);
      lcd.clear();
      break;
    }
  }
  cambiarHora();
  
  /*horaPrograma[0]=12;//Datos de prueba
  minutoPrograma[0]=1;
  flagPrograma[0] = true;
  porciones[0] = 1;
  programasGuardados = 1;*/
}


void loop()
{
  if (!menuInterno)
  {
    reloj(true);
    imprimirMenuGeneral();
  }
  else
  {
    switch (menuGeneral)
    {
      case 0:
        imprimirProgramacion();
        break;
       case 1:
        crearPrograma();
        break;
       case 2:
        eliminarPrograma();
        break;
       case 3:
        lcd.setCursor(3,0);
        lcd.print("Sirviendo");
        lcd.setCursor(4,1);
        lcd.print("Porcion");
        servirPorcion();
        menuInterno = false;
        lcd.clear();
        break;
       case 4:
        cambiarHora();
        break;
       default:
        break;
    }
  }
  
  horaDeComer();

  if (!menuInterno)
  {
    if (digitalRead(lcdLight))
    {
      if (!digitalRead(botonArriba) || !digitalRead(botonAbajo) || !digitalRead(botonSiguiente || !digitalRead(botonAtras)))
      {
        digitalWrite(lcdLight, LOW);
        delay(200);
        tiempoEspera = millis();
      }
    }
    
    if (!digitalRead(botonArriba))
    {
      menuGeneral++;
      if(menuGeneral > 4) menuGeneral = 0;
      delay(200);
      tiempoEspera = millis();
      lcd.clear();
    }
    if (!digitalRead(botonAbajo))
    {
      menuGeneral--;
      if(menuGeneral < 0) menuGeneral = 4;
      delay(200);
      tiempoEspera = millis();
      lcd.clear();
    }
    if (!digitalRead(botonSiguiente))
    {
      menuInterno = true;
      delay(200);
      tiempoEspera = millis();
      lcd.clear();
    }
    if (!digitalRead(botonAtras))
    {
      menuGeneral = 0;
      digitalWrite(buzzer, LOW);
      delay(200);
      tiempoEspera = millis();
      digitalWrite(buzzer, HIGH);
      lcd.clear();
    }
  }

  apagarLCD();
}


void reloj(bool imprim) //Imprime y controla el reloj
{
  static bool flagRefer = false; // AM/PM del reloj principal que va de 0 a 24
  String tiempoString, segString;
  static int tiempoRef, tiempoReloj = 0;

  tiempoString = String(millis(), DEC); //copia el numero millis en un string para manejarlo más facilmente
  segString = tiempoString.charAt(tiempoString.length()-4); //toma el numero de la posicion length -4 de tiempoString y lo mete en segString
  tiempoRef = segString.toInt();
    
  if (tiempoRef > tiempoReloj)
  {
    seg = seg + (tiempoRef - tiempoReloj);
    tiempoReloj = tiempoRef;
  }
  else if (tiempoRef < tiempoReloj)
  {
    seg = seg + ((tiempoRef + 10)-tiempoReloj);
    tiempoReloj = tiempoRef;
  }

  if (seg >= 60)
  {
    seg = seg - 60;
    minuto++;
  }
  if (minuto >= 60)
  {
    minuto = minuto - 60;
    hora++;
  }
  if (hora >= 13)
  {
    hora = hora - 12;
    
  }

  if ((hora >= 12) && flagRefer)
  {
    flag = !flag;
    flagRefer = false;
  }
  if (hora < 12) flagRefer = true;
  
  if (imprim)
  {
    lcd.setCursor(0,1);
    lcd.print("Hora: ");
    if (hora <= 9) lcd.print("0");
    lcd.print(hora);
    lcd.print(":");
    if (minuto <= 9) lcd.print("0");
    lcd.print(minuto);
    lcd.print(":");
    if (seg <= 9) lcd.print("0");
    lcd.print(seg);
    if (flag) lcd.print("AM");
    else if (!flag) lcd.print("PM");
  }
}

void horaDeComer()
{
  for (int temp = 0; temp < programasGuardados; temp++)
  {
    if ((horaPrograma[temp]==hora)&&(minutoPrograma[temp]==minuto)&&(seg==0)&&(flag==flagPrograma[temp]))
    {
      digitalWrite(lcdLight, LOW);
      tiempoEspera = millis();
      lcd.clear();
      lcd.setCursor(3,0);
      lcd.print("Sirviendo:");
      digitalWrite(buzzer, LOW); // Suena el buzzer por un segundo
      delay(1000);
      digitalWrite(buzzer, HIGH);
      
      for (int servido = 1; servido <= porciones[temp]; servido++)
      {
        lcd.setCursor(0,1);
        lcd.print("Porcion ");
        if (servido < 10) lcd.print("0");
        lcd.print(servido);
        lcd.print(" de ");
        if (porciones[temp] < 10) lcd.print("0");
        lcd.print(porciones[temp]);
        servirPorcion();
        reloj(false);
        delay(2000);
      }
      lcd.clear();
    }
  }
}

void imprimirMenuGeneral() //Imprime el menu general
{
  lcd.setCursor(0,0);
  if (menuGeneral == 0) lcd.print("Ver Programacion"); //Serial.print("Ver Programacion");
  if (menuGeneral == 1) lcd.print("Crear Programa"); //Serial.print("Crear Programa");
  if (menuGeneral == 2) lcd.print("Borrar Programa"); //Serial.print("Eliminar Programa");
  if (menuGeneral == 3) lcd.print("Servir porcion"); //Serial.print("Servir una porcion");
  if (menuGeneral == 4) lcd.print("Cambiar Hora"); //Serial.print("Cambiar Hora");
}

void imprimirProgramacion()
{
  int temp = 0;

  if(programasGuardados <= 0)
  {
    lcd.setCursor(0,0);
    lcd.print("No hay programas");
    lcd.setCursor(3,1);
    lcd.print("guardados");
    delay(2000);
    menuInterno = false;
    lcd.clear();
  }
  
  while (menuInterno)
  {
    reloj(false); //continúa corriendo el reloj sin imprimir
    
    lcd.setCursor(0,0); //Imprime la hora del programa numero temp
    lcd.print("Hora:");
    if (horaPrograma[temp] <= 9) lcd.print("0");
    lcd.print(horaPrograma[temp]);
    lcd.print(":");
    if (minutoPrograma[temp] <= 9) lcd.print("0");
    lcd.print(minutoPrograma[temp]);
    if (flagPrograma[temp]) lcd.print(" AM");
    else if (!flagPrograma[temp]) lcd.print(" PM");
    lcd.setCursor(15,0);
    lcd.write(byte(0));
  
    lcd.setCursor(0,1); //Imprime las porciones del programa numero temp
    lcd.print("Porciones: ");
    if (porciones[temp] <= 9) lcd.print("0");
    lcd.print(porciones[temp]);
    lcd.setCursor(15,1);
    lcd.write(byte(1));
    
    if((!digitalRead(botonSiguiente)) || (!digitalRead(botonAtras)))
    {
      menuInterno = false;
      delay(200);
      lcd.clear();
    }
    if (!digitalRead(botonArriba))
    {
      temp++;
      delay(200);
      if (temp >= programasGuardados) temp = 0;
      lcd.clear();
    }
    if (!digitalRead(botonAbajo))
    {
      temp--;
      delay(200);
      if (temp < 0) temp = programasGuardados - 1;
      lcd.clear();
    }
  }
  tiempoEspera = millis();
}

void crearPrograma()
{
  bool finalizado = false;
  int seleccion = 0;
  int horaTemp = 12, minutoTemp = 0, porcionTemp = 1; //Hora, minutos y porciones para crear un nuevo programa
  bool flagTemp = true; //AM/PM para crear un nuevo programa

  if(programasGuardados >= (maximoProgramas + 1))
  {
    lcd.setCursor(0,0);
    lcd.print("Maximo de progra-");
    lcd.setCursor(1,1);
    lcd.print("mas alcanzado");
    delay(2000);
    finalizado = true;
    lcd.clear();
  }
  
  while (!finalizado)
  {
    reloj(false);

    if (digitalRead(botonSiguiente) == LOW)
    {
      seleccion++;
      delay(200);
      lcd.clear();
    }
    if ((seleccion != 3)&&(seleccion != 4)&&(seleccion != 5))
    {
      lcd.setCursor(0,0);
      lcd.print("Programa:");
      if (horaTemp <= 9) lcd.print("0");
      lcd.print(horaTemp);
      lcd.print(":");
      if (minutoTemp <= 9) lcd.print("0");
      lcd.print(minutoTemp);
      if (flagTemp) lcd.print("AM");
      else lcd.print("PM");
    }
    if (seleccion == 0) //Cambiar la hora
    {
      lcd.setCursor(0,1);
      lcd.print("         ^^");
      if (digitalRead(botonArriba) == LOW)
      {
        horaTemp++;
        delay(200);
        lcd.clear();
      }
      if (digitalRead(botonAbajo) == LOW) 
      {
        horaTemp--;
        delay(200);
        lcd.clear();
      }
      if (horaTemp > 12) horaTemp = horaTemp - 12;
      if (horaTemp < 1) horaTemp = horaTemp + 12;
    }
    else if (seleccion == 1) //Cambiar los minutos
    {
      lcd.setCursor(0,1);
      lcd.print("            ^^");
      if (digitalRead(botonArriba) == LOW)
      {
        minutoTemp++;
        delay(200);
        lcd.clear();
      }
      if (digitalRead(botonAbajo) == LOW) 
      {
        minutoTemp--;
        delay(200);
        lcd.clear();
      }
      if (minutoTemp > 59) minutoTemp = minutoTemp - 60;
      if (minutoTemp < 0) minutoTemp = minutoTemp + 60;
    }
    else if (seleccion == 2) //Cambiar AM-PM
    {
      lcd.setCursor(0,1);
      lcd.print("              ^^");
      if ((digitalRead(botonArriba) == LOW)||(digitalRead(botonAbajo) == LOW))
      {
        flagTemp = !flagTemp;
        delay(200);
        lcd.clear();
      }
    }
    else if (seleccion == 3) //Cambiar cantidad de porciones
    {
      lcd.setCursor(0,0);
      lcd.print("Porciones: ");
      if (porcionTemp <= 9) lcd.print("0");
      lcd.print(porcionTemp);
      lcd.setCursor(0,1);
      lcd.print("           ^^");
      if (digitalRead(botonArriba) == LOW)
      {
        porcionTemp++;
        delay(200);
        lcd.clear();
      }
      if ((digitalRead(botonAbajo) == LOW)&&(porcionTemp > 1)) 
      {
        porcionTemp--;
        delay(200);
        lcd.clear();
      }
      if (porcionTemp > 99) porcionTemp = 1;
    }
    else if (seleccion == 4) //Aceptar programa
    {
      lcd.setCursor(0,0);
      lcd.print("Ok para guardar");
      lcd.setCursor(0,1);
      if (horaTemp <= 9) lcd.print("0");
      lcd.print(horaTemp);
      lcd.print(":");
      if (minutoTemp <= 9) lcd.print("0");
      lcd.print(minutoTemp);
      if (flagTemp) lcd.print("AM");
      else lcd.print("PM");
      lcd.print(" Porc:");
      if (porcionTemp <= 9) lcd.print("0");
      lcd.print(porcionTemp);
      /*if (!digitalRead(botonArriba))
      {
        lcd.scrollDisplayLeft();
        delay(200);
      }
      if (!digitalRead(botonAbajo))
      {
        lcd.scrollDisplayRight();
        delay(200);
      }*/
    }
    else if (seleccion == 5) //Guardar programa
    {
      horaPrograma[programasGuardados] = horaTemp;
      minutoPrograma[programasGuardados] = minutoTemp;
      porciones[programasGuardados] = porcionTemp;
      flagPrograma[programasGuardados] = flagTemp;
      programasGuardados++;
      finalizado = true;
      lcd.clear();
      lcd.setCursor(4,0);
      lcd.print("Programa");
      lcd.setCursor(4,1);
      lcd.print("Guardado");
      delay(1000);
      menuInterno = false;
    }
    if ((digitalRead(botonAtras) == LOW) && (seleccion == 0))
    {
      menuInterno = false;
      finalizado = true;
      delay(200);
      lcd.clear();
    }
    else if (digitalRead(botonAtras) == LOW)
    {
      seleccion--;
      delay(200);
      lcd.clear();
    }
  }
  tiempoEspera = millis();
}

void eliminarPrograma()
{
  int temp = 0;
  int Seleccion = 0;

  if(programasGuardados <= 0)
  {
    lcd.setCursor(0,0);
    lcd.print("No hay programas");
    lcd.setCursor(3,1);
    lcd.print("guardados");
    delay(2000);
    menuInterno = false;
    lcd.clear();
  }
  else
  {
    lcd.setCursor(1,0);
    lcd.print("Seleccione el");
    lcd.setCursor(0,1);
    lcd.print("horario a borrar");
    delay(2000);
    lcd.clear();
  }
  
  while (menuInterno)
  {
    reloj(false); //continúa corriendo el reloj sin imprimir

    if(!digitalRead(botonSiguiente))
    {
      Seleccion++;
      delay(200);
      lcd.clear();
    }
    if(!digitalRead(botonAtras))
    {
      Seleccion--;
      delay(200);
      lcd.clear();
    }
    
    switch (Seleccion)
    {
      case 0:
        lcd.setCursor(0,0); //Imprime la hora del programa numero temp
        lcd.print("Hora:");
        if (horaPrograma[temp] <= 9) lcd.print("0");
        lcd.print(horaPrograma[temp]);
        lcd.print(":");
        if (minutoPrograma[temp] <= 9) lcd.print("0");
        lcd.print(minutoPrograma[temp]);
        if (flagPrograma[temp]) lcd.print(" AM");
        else if (!flagPrograma[temp]) lcd.print(" PM");
        lcd.setCursor(15,0);
        lcd.write(byte(0));
      
        lcd.setCursor(0,1); //Imprime las porciones del programa numero temp
        lcd.print("Porciones: ");
        if (porciones[temp] <= 9) lcd.print("0");
        lcd.print(porciones[temp]);
        lcd.setCursor(15,1);
        lcd.write(byte(1));
        
        if (!digitalRead(botonArriba))
        {
          temp++;
          delay(200);
          if (temp >= programasGuardados) temp = 0;
          lcd.clear();
        }
        if (!digitalRead(botonAbajo))
        {
          temp--;
          delay(200);
          if (temp < 0) temp = programasGuardados - 1;
          lcd.clear();
        }
        if(!digitalRead(botonAtras))
        {
          menuInterno = false;
          delay(200);
          lcd.clear();
        }
        break;
      case 1:
        lcd.setCursor(1,0);
        lcd.print("Borrar horario");
        lcd.setCursor(1,1);
        lcd.print("seleccionado?");
        break;
       case 2:
        while(temp<(programasGuardados-1))
        {
          horaPrograma[temp] = horaPrograma[temp+1];
          minutoPrograma[temp] = minutoPrograma[temp+1];
          flagPrograma[temp] = flagPrograma[temp+1];
          porciones[temp] = porciones[temp+1];
          temp++;
        }
        programasGuardados--;
        menuInterno = false;
        lcd.setCursor(4,0);
        lcd.print("Programa");
        lcd.setCursor(4,1);
        lcd.print("borrado");
        delay(2000);
        break;
    }
  }
  tiempoEspera = millis();
}

void servirPorcion()
{
  for (int pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    elServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(10);                       // waits 15ms for the servo to reach the position
  }
  delay(2000);
  for ( int pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    elServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(10);                       // waits 15ms for the servo to reach the position
  }  
  tiempoEspera = millis();
}

void cambiarHora()
{
  bool finalizado = false;
  int seleccion = 0;
  int horaTemp = hora, minutoTemp = minuto; //Hora, minutos para cambiar la hora
  bool flagTemp = flag; //AM/PM para cambiar la hora
  
  while (!finalizado)
  {
    reloj(false);

    if (digitalRead(botonSiguiente) == LOW)
    {
      seleccion++;
      delay(200);
      lcd.clear();
    }
    if ((seleccion != 3)&&(seleccion != 4))
    {
      lcd.setCursor(0,0);
      lcd.print("Hora: ");
      if (horaTemp <= 9) lcd.print("0");
      lcd.print(horaTemp);
      lcd.print(":");
      if (minutoTemp <= 9) lcd.print("0");
      lcd.print(minutoTemp);
      if (flagTemp) lcd.print(" AM");
      else lcd.print(" PM");
    }
    if (seleccion == 0) //Cambiar la hora
    {
      lcd.setCursor(0,1);
      lcd.print("      ^^");
      if (digitalRead(botonArriba) == LOW)
      {
        horaTemp++;
        delay(200);
        lcd.clear();
      }
      if (digitalRead(botonAbajo) == LOW) 
      {
        horaTemp--;
        delay(200);
        lcd.clear();
      }
      if (horaTemp > 12) horaTemp = horaTemp - 12;
      if (horaTemp < 1) horaTemp = horaTemp + 12;
    }
    else if (seleccion == 1) //Cambiar los minutos
    {
      lcd.setCursor(0,1);
      lcd.print("         ^^");
      if (digitalRead(botonArriba) == LOW)
      {
        minutoTemp++;
        delay(200);
        lcd.clear();
      }
      if (digitalRead(botonAbajo) == LOW) 
      {
        minutoTemp--;
        delay(200);
        lcd.clear();
      }
      if (minutoTemp > 59) minutoTemp = minutoTemp - 60;
      if (minutoTemp < 0) minutoTemp = minutoTemp + 60;
    }
    else if (seleccion == 2) //Cambiar AM-PM
    {
      lcd.setCursor(0,1);
      lcd.print("            ^^");
      if ((digitalRead(botonArriba) == LOW)||(digitalRead(botonAbajo) == LOW))
      {
        flagTemp = !flagTemp;
        delay(200);
        lcd.clear();
      }
    }
    else if (seleccion == 3) //Aceptar programa
    {
      lcd.setCursor(0,0);
      lcd.print("Ok para guardar");
      lcd.setCursor(0,1);
      lcd.print(" Hora: ");
      if (horaTemp <= 9) lcd.print("0");
      lcd.print(horaTemp);
      lcd.print(":");
      if (minutoTemp <= 9) lcd.print("0");
      lcd.print(minutoTemp);
      if (flagTemp) lcd.print(" AM");
      else lcd.print(" PM");
    }
    else if (seleccion == 4) //Guardar programa
    {
      hora = horaTemp;
      minuto = minutoTemp;
      flag = flagTemp;
      finalizado = true;
      lcd.clear();
      lcd.setCursor(6,0);
      lcd.print("Hora");
      lcd.setCursor(2,1);
      lcd.print("Actualizada");
      delay(1000);
      seg = 0;
      menuInterno = false;
    }
    if ((digitalRead(botonAtras) == LOW) && (seleccion == 0))
    {
      menuInterno = false;
      finalizado = true;
      delay(200);
      lcd.clear();
    }
    else if (digitalRead(botonAtras) == LOW)
    {
      seleccion--;
      delay(200);
      lcd.clear();
    }
  }
  tiempoEspera = millis();
}

void apagarLCD()
{
  unsigned long x = millis() - tiempoEspera;
  
  if (x > 60000)
  {
    digitalWrite(lcdLight, HIGH);
  }
  else digitalWrite(lcdLight, LOW);
}
