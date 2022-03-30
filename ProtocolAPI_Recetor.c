#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS10"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define ESCBYTE 0x7d
#define MAX_SIZE 255
#define TRANSMITTER 1
#define RECEIVER 0

//novos defines para maquina de estados
#define Flag 0x7e
#define A 0x01 //Receiver
#define C 0x07  
#define BCC A^C


//
volatile int STOP=FALSE;

struct applicationLayer {
int fileDescriptor; /*Descritor correspondente à porta série*/
int status; /*TRANSMITTER | RECEIVER*/
int fd,res;
char buf[255];
};

struct linkLayer {
char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
int baudRate; /*Velocidade de transmissão*/
unsigned int sequenceNumber; /*Número de sequência da trama: 0, 1*/
unsigned int timeout; /*Valor do temporizador: 1 s*/
unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
char frame[MAX_SIZE]; /*Trama*/
};


//Inicia comunicaçao entre transmitter(util=0) e receiver(util=1)
//returns data connection id, or negative value in case of failure/error
int llopen(int porta, int util){

  int state=0, i, res,fd, buf[255];
  char door[2]={0};
  struct applicationLayer app;  
  struct linkLayer layer;

  //define port
  door[0]='0'+porta;
  strcpy(layer.port, "/dev/ttyS");
  if(porta<10){
  strcat(layer.port, door);
  }else if(porta==10){
    strcat(layer.port, "10");
  }else     strcat(layer.port, "11");
  
  layer.baudRate=BAUDRATE;
  layer.timeout=3;

 
  //control field definition
  
   //Receiver
      switch(state){
        case 0:
        i=0;
        while (STOP==FALSE) {               /* loop for input */
        res = read(fd, &buf[i] ,1);         //lemos só de 1 em 1 para simplificar as coisas, até pq esta função somehow le 8 bytes at once
          if (buf[i]=='\0') {STOP=TRUE; break;}
          i++;
          printf("%c\n", buf[i-1]);
        }
          if(Flag){
            state = 1;
            break;
          }
        case 1: 
          if(Flag){
            state = 1; 
            break;
          }
          else if(A){
            state = 2;
            break;
          }
          else{
            state = 0;
            break;
          }
          
        case 2:
          if(C == 0x03){
            state = 3;
            break;
          }
          else if(Flag){
            state = 1; 
            break;
          }
          else{
            state = 0;
            break;
          }
        case 3:
          if(BCC == A^C){
              state = 3;
              break;          //se BCC direito passar de estado
            }
          else if(Flag){
            state = 1; 
            break;
          }
          else{
            state = 0;
            break;
          }
        case 4:
          if(Flag){
              state = 5;
              break;         
            }
        case 5:
        //FAZER RETRANS
        //fgets(buf,255,stdin);
         // lenght=strlen(buf);
         // buf[lenght-1]='\0';
        res=write(fd, buf, i+1);;
        

          state = 0;
          break;
       }

    
  }


int main(int argc, char** argv)
{
  
 // llopen(8, 0);
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

    //COnfirma se inserimos bem no kernel
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS11", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS11\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.*/
  

    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }


    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");




    
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }



    close(fd);
    sleep(1);
    return 0;
}

