#include <Wire.h> //I2C Arduino Library

#define address 0x1E //0011110b, I2C 7bit address of HMC5883

#include <Servo.h> 

#define DIFF    107    // diferenca do zero da bussola eletronica em relacao ao N verd.
#define DIFFINT 3   // intervalo de seguranca
 
 //Angulo de 160° para servo da Radio Shack (28/05/2015) DGPP
Servo myservo;  // create servo object to control a servo 
                // twelve servo objects can be created on most boards
 
int angle_zero = 0;    // variable to store the servo position

//Conexões:
// SDA -> A4 
// SCL -> A5 

// 08 01 2016
// Este programa foi testado junto com magsensor.ino (este usa lib adafruit pura) e ambos
//  nao funcionam como esperado! 
// Ao girar o sensor 360 graus, nao ocorre variacao de 360 na medida em angulos (ocorre 220~330)
// Apos 2 dias de debug: nao resolvido, comprar nova placa sensor e testar.
// 
// 26 01 2016 
//  Foi instalado novo sensor magnetometro comprado em www.tato.ind.br e funciona
//  melhor, pelo menos o zero graus sempre esta no mesmo lugar e ocorre transicao completa. Porem
//  os outros pontos cardeais não estão adequados, nem o N na verdade, mas já podemos saber por 
//  comparacao
//
// 02 02 2016
// Calibrado e funcionando bem com piramide para N verdadeiro com DIFF == 107,
//  ou seja, precisa somar 107 ao N da bussola eletronica para dar N verdadeiro...
// Tambem nas rotinas de setpoint, tem valores empiricos que correpondem ao 
//  zero graus do servo (bussola == ~315 graus) e 180 graus do servo (bus== ~200 graus)
// Isso é feio e desanimador, mas o datasheet da bussola nao é completo para calibra-la bem.
// Mas ok para a prova de conceito um suporte de piramide auto-alinhavel!!


void setup(){

  int pos;

  //Initialize Serial and I2C communications
  Serial.begin(9600);
  Wire.begin();

  Serial.println("TESTE ");
  
  //Put the HMC5883 IC into the correct operating mode
  Wire.beginTransmission(address); //open communication with HMC5883
  Wire.write(0x02); //select mode register
  Wire.write(0x00); //continuous measurement mode
  Wire.endTransmission();
  
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object 

  //Colocar na posicao de descando do servo (meio do curso, 90 graus)
  for(pos = myservo.read(); pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(100);                       // waits 15ms for the servo to reach the position 
  } 
  

  Serial.print("Posicao Inicial: ");
  Serial.println(myservo.read());
}


//A rotina loop trava na leitura zero (N, 0 graus) da bussola eletronica
// Entretanto:
// Esta deslocado em 60 para W, anti-horario. tem que fazer essa correcao ainda....
// Mas jah esta fazendo o lock.

void loop(){
      
  int angle, old_angle_x, angle_x;  //control loop for servo

  int servo_angle;

  int locked = 0;
  int last_lock, first_adjust;

  
  int x,y,z; //triple axis data
  //Obs: para esse sensor (comprado na Tato, eixo z nao funciona)

  while(1) { //while aqui para usar continue

  //Tell the HMC5883L where to begin reading data
  Wire.beginTransmission(address);
  Wire.write(0x03); //select register 3, X MSB register
  Wire.endTransmission();
  
 
 //Read data from each axis, 2 registers per axis
  Wire.requestFrom(address, 6);
  if(6<=Wire.available()){
    x = Wire.read()<<8; //X msb
    x |= Wire.read(); //X lsb
    z = Wire.read()<<8; //Z msb
    z |= Wire.read(); //Z lsb
    y = Wire.read()<<8; //Y msb
    y |= Wire.read(); //Y lsb
  }
  
  //Print out values of each axis
  //Serial.print("x: ");
  //Serial.print(x);
  //Serial.print("  y: ");
  //Serial.print(y);
  //Serial.print("  z: ");
  //Serial.print(z);
  
  //Transforma a leitura em graus
  // Na internet tem essa formula da atan. No DS nao... por isso nao sabemos se eh exata
  float heading = atan2((float)y * 0.92, (float)x * 0.92);
   if(heading < 0)
      heading += 2*PI;
   float headingDegrees = heading * 180/M_PI;
   Serial.print("--> Angulo: ");
   Serial.println(headingDegrees);


  delay(250);

  
  // VErifica na posição que foi ligado, a distancia de N (sendo que N==0).
  angle_zero = headingDegrees;  //107 eh o fator de correcao mais a declinacao


  //Dentro de uma margem consideramos que está travada, pois o sensor oscila
  //if (angle_zero < 4 || angle_zero > 356) {
  if (angle_zero >= DIFF - DIFFINT && angle_zero <= DIFF + DIFFINT) {  //107+-4 de acordo com calibração ao Nverd em 02-02-16
    Serial.println(" Locked!");
    locked = 1;
    
    } else {
      locked = 0;
      
      //Logica para permitir novo ajuste ao mover a base, apos locked.
      if (last_lock == 1) {
        last_lock = 0;
        first_adjust = 1;
      }
      
    }

    if (locked == 1) {
      last_lock = 1;
      continue;

      } else {

        //Na primeira vez, apos sair do lock, coloca novamente
        // na posição de descanso dessa nova posicao do servo para comecar de novo
        if (first_adjust == 1) {
          first_adjust = 0;

          //for(int pos = myservo.read(); pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
          //{                                  // in steps of 1 degree u
          //  myservo.write(pos);              // tell servo to go to position in variable 'pos' 
          //  delay(50);                       // waits 15ms for the servo to reach the position 
          //} 

        }

      }

  // Precisa mover manualmente enquanto angle_zero não for menor que a faixa de 
  // rotacao segura do servo, para girar sem chegar no limite e provocar reset
  // TODO: fazer essa verificacao tambem pela propria posicao do servo (0 e 180 graus)
  if (angle_zero > 320 || angle_zero < 195) {
    Serial.print("   Ajuste angulo inicial:   ");
    Serial.print(angle_zero);
    } else {
      Serial.println("Fora do intervalo de angulo, favor mover a base até que funcione.");
      continue;
  }


  angle_x = angle_zero;

  //Precisa guardar a posicao absoluta para nao zerar o servo

  Serial.print(" Delta: ");
  Serial.println(angle_x);

  //Vamos rotacionar sentido horario, para aumentar os angulos
  if (angle_x > 320 || angle_x < DIFF) { 
                              
      servo_angle = myservo.read();
      Serial.print (" - Lado (w), Read :"); Serial.println(servo_angle);
      servo_angle -= 1;
      myservo.write(servo_angle);              
      delay(25);                       

  //Vamos rotacionar sentido anti-horario, para diminuir os angulos
  } else if (angle_x < 195 && angle_x >= DIFF) {                               

      servo_angle = myservo.read();
      Serial.print (" - (lado E), Read: "); Serial.println(servo_angle);
      servo_angle += 1;
      myservo.write(servo_angle);               
      delay(25);                       

    } 
  
  } //while 1
  
} //Loop()
