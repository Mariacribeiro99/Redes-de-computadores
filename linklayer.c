#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "linklayer.h"

//
volatile int STOP = FALSE;

struct applicationLayer
{
  int fileDescriptor; /*Descritor correspondente à porta série*/
  int status;         /*TRANSMITTER | RECEIVER*/
};

struct linkLayer
{
  char port[20];                           /*Dispositivo /dev/ttySx, x = 0, 1*/
  int baudRate;                            /*Velocidade de transmissão*/
  unsigned int sequenceNumber;             /*Número de sequência da trama: 0, 1*/
  unsigned int timeout;                    /*Valor do temporizador: 1 s*/
  unsigned int numTransmissions;           /*Número de tentativas em caso de falha*/
  char frame[MAX_RETRANSMISSIONS_DEFAULT]; /*Trama*/
};
//FLAGS
#define Flag 0x7e
#define A_R 0x01 //Recetor
#define A_T 0x05 //Transmissor
#define UA 0x07  
#define BCC_R A_R^UA
#define BCC_T A_T^SET   
#define SET 0x03



// Inicia comunicaçao entre transmitter(util=0) e receiver(util=1)
// returns data connection id, or negative value in case of failure/error
int llopen1(int porta, int util)
{

  int state = 0, i, res, fd, role, k;
  char *door = malloc(sizeof(char));
  struct applicationLayer app;
  struct linkLayer layer;
  struct termios oldtio, newtio;
  char buf[5], input[5];

  // define port
  door[0] = '0' + porta;
  strcpy(layer.port, "/dev/ttyS");
  if (porta < 10)
  {
    strcat(layer.port, door);
  }
  else if (porta == 10)
  {
    strcat(layer.port, "10");
  }
  else
    strcat(layer.port, "11");

  layer.baudRate = BAUDRATE_DEFAULT;
  layer.timeout = 3;

  // COnfirma se inserimos bem no kernel
  if ((strcmp("/dev/ttyS10", layer.port) != 0) &&
      (strcmp("/dev/ttyS11", layer.port) != 0))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS11\n");
    exit(1);
  }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.*/

  {
    fd = open(layer.port, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
      perror(layer.port);
      exit(-1);
    }

    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE_DEFAULT | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;  /* blocking read until 1 chars received */

    /*
      VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
      leitura do(s) próximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
  }

  printf("antes switch \n");

  if (role == 0) // trans
  {
    state = 0;
    input[0] = Flag;
    input[1] = A_T;
    input[2] = SET;
    input[3] = BCC_T;
    input[4] = Flag;

    // Maquina de estados para verificação da mensagem
    k = 0;
    while (k <= layer.numTransmissions && state != 6)
    {
      switch (state)
      {

      // escrever informaçao e verificar
      case 0:
        alarm(0);
        printf("Estamos em state=0 pela %d vez\n", k);
        if (k > layer.numTransmissions)
        {
          printf("Nao recebeste dados nenhuns amigo, problems de envio\n");
          break;
        }
        k++;
        res = write(fd, input, 5);
        printf("%d Bytes written\n", res);
        state = 1;
        alarm(layer.timeout);
        break;

      // receber informaçao
      case 1:
        printf("ENtramos na funçao read\n");
        while (!read(fd, &input[0], 1))
        {
        }
        printf("saimos do primeiro read\n");
        if (input[0] == Flag)
        {
          printf("Lemos a promeira flag\n");
          state = 2;
        }
        else
          state = 0;

        break;
      // verificação da mensagem recebida, ja paramos o timer de rececao, porque somos fixes e recebemos tudo
      case 2:
        while (!read(fd, &input[1], 1))
        {
        }
        if (input[1] == A_T)
        {
          printf("Address very well received!\n");
          state = 3;
        }
        else
          state = 0;

        break;

      case 3:
        while (!read(fd, &input[2], 1))
        {
        }
        if (input[2] == UA)
        {
          printf("UA IS HERE WITH US TOOOOO!\n");
          state = 4;
        }
        else
          state = 0;

        break;
      case 4:
        while (!read(fd, &input[3], 1))
        {
        }
        if (input[3] == BCC_R)
        {
          printf("What, os bits de verificaçao(BCC) tambem estao bem? Crazyy\n");
          state = 5;
        }
        else
          state = 0;
        printf("Erros de transmissao, repeat please bro!\n");

        break;
      case 5:
        while (!read(fd, &input[4], 1))
        {
        }
        if (input[4] == Flag)
        {
          printf("RECEBEMOS TUDO EM CONDIÇOES, CONGRATULATIONS\n");
          state = 6;
        }
        else
        {
          state = 0;
          printf("Erros de transmissao, repeat please bro!\n");
        }

        break;
      }
    }
    printf("WE DID IT MOTHERFUCKERS\n");
  }

  else if (role == 1)
  {
    // Flags recetor
    input[0] = Flag;
    input[1] = A_R;
    input[2] = UA;
    input[3] = BCC_R;
    input[4] = Flag;
    // control field definition
    while (state != 6)
    {
      // Receiver
      switch (state)
      {
      case 0:
        printf("ENtramos na funçao read\n");
        while (!state)
        {
          res = read(fd, &buf[0], 1);
          printf("%d Bytes Read\n", res);
          if (res == 1)
            break;
        }
        printf("saimos do primeiro read\n");
        if (buf[0] == Flag)
        {
          printf("Lemos a primeira flag\n");
          state = 1;
        }
        else
          state = 0;
        break;

        // verificação da mensagem recebida, ja paramos o timer de rececao, porque somos fixes e recebemos tudo
      case 1://verificare (state && (!res))
        {
          res = read(fd, &buf[1], 1);
        }
        printf("%d Bytes Read\n", res);
        if (buf[1] == A_R)
        {
          printf("Address very well received!\n");
          state = 2;
        }
        else
        {
          state = 0;
          printf("N recebemos o Address\n");
        }

        break;

      case 2:
        res = 0;
        while ((state == 2) && (!res))
        {
          res = read(fd, &buf[2], 1);
        }
        if (buf[2] == SET)
        {
          printf("SET IS HERE WITH US TOOOOO!\n");
          state = 3;
        }
        else
          state = 0;

        break;
      case 3:
        res = 0;
        while ((state == 3) && (!res))
        {
          res = read(fd, &buf[3], 1);
        }
        if (buf[3] == (SET ^ A_R))
        {
          printf("What, os bits de verificaçao(BCC) tambem estao bem? Crazyy\n");
          state = 4;
        }
        else
          state = 0;
        printf("Erros de transmissao, repeat please bro!\n");

        break;
      case 4:
        res = 0;
        while ((state == 4) && (!res))
        {
          res = read(fd, &buf[4], 1);
        }
        if (buf[4] == Flag)
        {
          printf("RECEBEMOS TUDO EM CONDIÇOES, CONGRATULATIONS\n");
          state = 5;
          break;
        }
        else
          state = 0;
        printf("Erros de transmissao, repeat please bro!\n");

        break;
      case 5:
        // FAZER RETRANS
        res = write(fd, input, 5);
        printf("%d Bytes written\n", res);
        state = 6;
        break;
      }
    }
    printf("Finished everything \n");

    sleep(1);
    return 0;
  }
}




int len(char *packet)
{
  if (!packet)
    return -1;
  int fd, length, i = 0;
  char data, *buf;

  for (i = 1; data != Flag; i++)
  {
    buf[i] = data;
  }

  length = strlen(data);

  return length;
}

int llread(char *packet)
{
  int i, tam, fd, length = 0;

  tam = len(packet);
  if (tam < 0)
    return -1;

  char Read_buf[tam], new_buf[tam];

  while (length < 0)
  {
    length = read(fd, Read_buf, tam);
  }
  printf("packet lenght is %d \n", length);

  int j=0;
  for (i = 0; i < length; i++)
  {
    //???????????????????
    if (Read_buf[i] == 0x7d)
    {
      if (Read_buf[i + 1] == 0x05e)
      {
        new_buf[j]
      }
    }
  }
  if (i == length)
  {
    printf("Nao recebmos flags, tentar outra vez\n");
    return 0;
  }
}

int llwrite(char *buf, int bufSize)
{

  int cnt = 0, i = 0, aux, j = 0;
  int inputSize = (bufSize * % 8);
  char *input = malloc(sizeof(char) * (bufSize * 2 + 8));
  char stuff = 0, bcc2[256];
  input[0] = Flag;
  input[1] = A;
  input[2] = C;
  input[3] = 0;

  for (j = 0; j < 2; j++)
  {
    for (i = 0; i < 8; i++)
    {
      aux = (input[1 + j] >> i) & 1;
      printf("aux=%d\n", aux); //reading just fine
      if (aux)
      {
        cnt++;
      }
    }
    if (!(cnt % 2))
    {
      input[3] = input[3] | (1 << (7 - j));
    }
    cnt = 0;
  }
  printf("BCC1=%d", (int)input[3]);

  for (j = 0; j < bufSize; j++)
  {
    for (i = 0; i < 8; i++)
    {
      //stuff=(stuff<<1)^buf[j]>>i
      aux = (input[4 + j] >> i) & 1;
      if (aux)
      {
        cnt++;
      }
    }
    if (!(cnt % 2))
    {
      input[4 + bufSize] = input[4 + bufSize] | (1 << (7 - j));
    }
  }
}
*/
int main(int argc, char **argv)
{

  llopen1(11, 0);
  return 0;
}