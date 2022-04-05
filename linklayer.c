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
  int role;
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

// Inicia comunicaçao entre transmitter(util=0) e receiver(util=1)
// returns data connection id, or negative value in case of failure/error
int llopen(int porta, int util)
{

  int state = 0, i, res, fd;
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

  if (role == 0)
  {
    state=0;
    input[0] = Flag;
    input[1] = A;
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
        while (!read(fd, &output[0], 1))
        {
        }
        printf("saimos do primeiro read\n");
        if (output[0] == Flag)
        {
          printf("Lemos a promeira flag\n");
          state = 2;
        }
        else
          state = 0;

        break;
      // verificação da mensagem recebida, ja paramos o timer de rececao, porque somos fixes e recebemos tudo
      case 2:
        while (!read(fd, &output[1], 1))
        {
        }
        if (output[1] == A)
        {
          printf("Address very well received!\n");
          state = 3;
        }
        else
          state = 0;

        break;

      case 3:
        while (!read(fd, &output[2], 1))
        {
        }
        if (output[2] == UA)
        {
          printf("UA IS HERE WITH US TOOOOO!\n");
          state = 4;
        }
        else
          state = 0;

        break;
      case 4:
        while (!read(fd, &output[3], 1))
        {
        }
        if (output[3] == BCC_R)
        {
          printf("What, os bits de verificaçao(BCC) tambem estao bem? Crazyy\n");
          state = 5;
        }
        else
          state = 0;
        printf("Erros de transmissao, repeat please bro!\n");

        break;
      case 5:
        while (!read(fd, &output[4], 1))
        {
        }
        if (output[4] == Flag)
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
    case 1:
      res = 0;
      printf("ENtramso em state=1\n");
      while (state && (!res))
      {
        res = read(fd, &buf[1], 1);
      }
      printf("%d Bytes Read\n", res);
      if (buf[1] == A)
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
      if (buf[3] == (SET ^ A))
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

int main(int argc, char **argv)
{

  llopen(11, 0);
  return 0;
}