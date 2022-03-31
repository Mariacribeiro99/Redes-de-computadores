#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS0"
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
#define UA 0x07  
#define BCC_R A^UA
#define SET 0x03

//
volatile int STOP=FALSE;

struct applicationLayer {
int fileDescriptor; /*Descritor correspondente à porta série*/
int status; /*TRANSMITTER | RECEIVER*/

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



  int state=0, i, res,fd;
  char *door=malloc(sizeof(char));
  struct applicationLayer app;  
  struct linkLayer layer;
  struct termios oldtio, newtio;
  char buf[255], input[5];

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

    input[0]=Flag;
    input[1]=A;
    input[2]=UA;
    input[3]=BCC_R;
    input[4]=Flag;
 
  //COnfirma se inserimos bem no kernel
    if ((strcmp("/dev/ttyS0", layer.port)!=0) && 
  	      (strcmp("/dev/ttyS1", layer.port)!=0)) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS11\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.*/
  
  {
    fd = open(layer.port, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(layer.port); exit(-1); }

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

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */



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

  }

    printf("antes switch \n");

  //control field definition
  while(1){
   //Receiver
      switch(state){
        case 0:  
	    	printf("ENtramos na funçao read\n");
		while(!state){
		res=read(fd,&input[0],1);
    if(res==1)break;
	}
		printf("saimos do primeiro read\n");
		if(input[0]==Flag){
			printf("Lemos a promeira flag\n");
			state=1;
			}else state=0;
    
    
break;
  //verificação da mensagem recebida, ja paramos o timer de rececao, porque somos fixes e recebemos tudo
  case 1:
  printf("ENtramso em state=1\n");
  while(state){
  		(void)read(fd,&input[1],1);}
  if(input[1]==A){
	 printf("Address very well received!\n");
   state=2;
	  }else state=0;
	  
break;	  
	  
	  case 2:
while(state){
		(void)read(fd,&input[2],1);}
	if(input[2]==SET){
    printf("SET IS HERE WITH US TOOOOO!\n");
    state = 3;
}else state=0;
    
    
break;    
    case 3: 
    while(state){
    (void)read(fd,&input[3],1);}
      if(input[3]==SET^A){
    printf("What, os bits de verificaçao(BCC) tambem estao bem? Crazyy\n");
    state = 4;
  }else state=0;printf("Erros de transmissao, repeat please bro!\n");
    
break;    
    case 4:
    while(state){
    (void)read(fd,&input[4],1);}
    if(input[4]==Flag){
    printf("RECEBEMOS TUDO EM CONDIÇOES, CONGRATULATIONS\n");break;
    state = 5;
  }else state=0;printf("Erros de transmissao, repeat please bro!\n");
    
    break;
     case 5:
        //FAZER RETRANS
        res=write(fd, buf, i+1);;
        break;   
    
  

  }

  }


       /* case 0:
        i=0;
        while (STOP==FALSE) {               // loop for input //
        printf("em narnia \n");
        res = read(fd, &buf[i] ,1);         //lemos só de 1 em 1 para simplificar as coisas, até pq esta função somehow le 8 bytes at once
          if (input[i]==Flag && i > 0) {STOP=TRUE; break;}
          i++;
          printf("%c\n", buf[i-1]);
        }
        printf("fora de narnia \n");
          if(Flag == input[0]){
            printf("recebi primeria flag\n");
            state = 1;
             break;
          }
        case 1: 
         
          if(input[1]==A){
             printf("recebi 2 flag\n");
            state = 2;
            break;
          }
          else{
            state = 0;
          break;
          }
          
        case 2:
          if(input[2] == SET){
             printf("recebi 3 flag\n");
            state = 3;
          break;
          }
          else if(input[2]==Flag){
             printf("recebi ultima flag\n");
            state = 1; 
          break;
          }
          else{
            state = 0;
            break;
          }
        case 3:
          if(BCC_R == A^UA){
              state = 3;
             break;        //se BCC direito passar de estado
            }
          else if(input[0]==Flag){
            state = 1; 
          break;
          }
          else{
            state = 0;
             break;
          }
        case 4:
          if(input[4]==Flag){
              state = 5;
               break;       
            }
        case 5:
        //FAZER RETRANS
        res=write(fd, buf, i+1);;
        break;    
        //fgets(buf,255,stdin);
        // lenght=strlen(buf);
        // buf[lenght-1]='\0';
*/




    //  }
  //}
    printf("dps switch \n");

	  sleep(1);
    close(fd);
    return 0;
}

int main(int argc, char** argv)
{
  
  llopen(0,1);
    return 0;
}

