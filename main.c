#include <stdio.h>
#include <stdlib.h>
#include "gpio_sysfs.h"
#include <unistd.h>
#include "gpio_dev_mem.h"
#include <bcm2835.h>
#include <time.h>


int main(int argc, char **argv)
{
	int i = 1;
	int pin = 4;
	pid_t iPid;		// PID do processo filho
	setGPIO_In(pin);	// Definindo GPIO para receber o sinal
	iPid = fork();		// Criando o processo

	if (iPid == 0)		// Iniciando o código filho
	{
		while (i == 1)	
		 {
			if (GPIO_Read(pin) == 0)		// Leitura da GPIO_4, se o valor for = 0, fazer a leitura novamente
			{
				i = 1;
			}
			else if (GPIO_Read(pin) == 1)	// Leitura da GPIO_4, se o valor foi = 1, sair do laço
			{	
				i = 0;
			}	
			else 				// Garantir nova leitura do sensor
			{
				i = 1;
				sleep(0.99);		
			}
		}

		// Entrando na pasta onde está o código do reconhecimento facil e executando o arquivo .cpp e utilizando o aquivo 
		// faces.csv da pasta do banco de dados com as imagens tratadas. 
		// Valor 1 para escolher a escala cinza. Se escolher 0 as imagem ficará com cores e deixará o programa mais lento
		// Valor de confiança do reconhecimento em 2900 do modo Eigenfaces.
		system("cd Face_Reco && cd bytefish-libfacerec-e1b143d && ./reco faces.csv 1 2900");
	}
	else						// Iniciando o código pai
	{
		sleep(5000);				//Aguardando 5 min da realização do código filho
		while (i == 1) 
		{
			if (GPIO_Read(pin) == 0)		// Leitura GPIO_4, se o valor for = 0, finalizar o aplicativo 
			{
				system("chmod u+rx desliga.sh && sh desliga.sh");	// Matando o aplicativo MagicMirror e face_reco
				system("cd && ./main");					// Reinicia a main
			}
			else if (GPIO_Read(pin) == 1)	// Leitura GPIO_4, se o valor for = 1, esperar e 2 min para realizar nova leitura
			{	
				sleep(2000);
				i = 1;
			}	
		}
	}
return 0;
}
