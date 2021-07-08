
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "/home/pi/Face_Reco/bytefish-libfacerec-e1b143d/include/facerec.hpp"
#include <opencv2/core/types_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "time.h"

using namespace cv;
using namespace std;

// Definindo as constantes do número de pessoas e determinando o valor de cada pessoa
#define MAX_PEOPLE 		2
#define P_PEDRO			0
#define P_GABRIELA		1

// para depuração e rastreio de bug
#define TRACE 1
#define DEBUG_MODE 0
#define DEBUG if (DEBUG_MODE==1)

// Definindo caminho das pastas onde estão os arquivos .xml
string glasses_cascade_name = "/home/pi/test_1/opencv/data/haarcascades/haarcascade_eye_tree_eyeglasses.xml";
string eyes_cascade_name 	= "/home/pi/test_1/opencv/data/haarcascades/haarcascade_eye.xml";

// Arquivos da OpenCV
CvPoint Myeye_left;
CvPoint Myeye_right;
CascadeClassifier eyes_cascade;
CascadeClassifier glasses_cascade;

// Definindo pessoa
string  people[MAX_PEOPLE];

int bHisto;
int res;

// Número de imagens aprendidas por pessoa. Imagens no banco de dados e buscada pelo arquivo faces.csv
int nPictureById[MAX_PEOPLE];

void trace(string s)
{
	if (TRACE==1)
	{
		cout<<s<<"\n";
	}
}

// Usando a OpenCV para ler o arquivo csv, no caso o arquivo faces.csv com os nomes e caminho das imagens
static void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';') {
    std::ifstream file(filename.c_str(), ifstream::in);
    if (!file) {
        string error_message = "(Erro) Sem arquivo válido.";
        CV_Error(Error::StsBadArg, error_message);
    }
    string line, path, classlabel;
    int nLine=0;
    while (getline(file, line)) {
        stringstream liness(line);
        getline(liness, path, separator);
        getline(liness, classlabel);
        if(!path.empty() && !classlabel.empty()) 
        {
        	// Lendo o arquivo e construindo os dados das imagens para comparação
            images.push_back(imread(path, 0));
            labels.push_back(atoi(classlabel.c_str()));
            nPictureById[atoi(classlabel.c_str())]++;
        	nLine++;
        }
    }
	char sTmp[128];
    sprintf(sTmp,"(init) %d pictures read to train",nLine);
    trace((string)(sTmp));
	for (int j=0;j<MAX_PEOPLE;j++)
	{
		sprintf(sTmp,"(init) %d pictures of %s (%d) read to train",nPictureById[j],people[j].c_str(),j);
   	 	trace((string)(sTmp));
	}
}
	void traceStep(int nStep)
{
	cout<<"("<<nStep<<")"<<clock()<<"\n";
}

//Função principal do reconhecimento facial
int main(int argc, const char *argv[]) {
	

	// Escrevendo os argumentos e imprimindo 
	cout<<"start\n";
	   if ((argc != 4)&&(argc!=3)) {
	       cout << "usage: " << argv[0] << " ext_files  seuil(opt) \n files.ext histo(0/1) 5000 \n" << endl;
	       exit(1);
	   }
	   int PREDICTION_SEUIL ;
	// Definindo o valor mínimo de para reconhecer
	if (argc==3) { trace("(init) limite de previsão = 3400.0 por padrão");PREDICTION_SEUIL = 3400.0;}
	if (argc==4) PREDICTION_SEUIL = atoi(argv[3]);
	
	// Abrindo a possibilidade de escolher o histograma de cor, escolhido na chamada do programa, 0 para colorido e 1 para cinza
	bHisto=atoi(argv[2]);
	
	// Carregando os arquivos usando o OpenCV
	if( !eyes_cascade.load( eyes_cascade_name ) ){ printf("--(!)Erro no carregamento\n"); return -1; };
	if( !glasses_cascade.load( glasses_cascade_name ) ){ printf("--(!)Erro no carregamento\n"); return -1; };
	
	// Iniciando as pessoas
	Mat gray, frame,original,face,face_resized;
		
	people[P_PEDRO] 	= "Pedro";
	people[P_GABRIELA] 	= "Gabriela";

	// Iniciar
	for (int i=0;i>MAX_PEOPLE;i++) 
	{
		nPictureById[i]=0;
	}
	int bFirstDisplay	=1;
	trace("(init) Pessoas inicializadas");
	
	// Obtendo caminho para o OpenCV
	string fn_csv = string(argv[1]);
	string fn_haar = "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt.xml";
	DEBUG cout<<"(OK) csv="<<fn_csv<<"\n";
    
    vector<Mat> images;
    vector<int> labels;
    //Lendo dados 
    try {
        read_csv(fn_csv, images, labels);
		DEBUG cout<<"(OK) read CSV ok\n";
    	} 
    catch (cv::Exception& e) 
    {
        cerr << "Erro ao abrir o arquivo \"" << fn_csv << "\". Reason: " << e.msg << endl;
        exit(1);
    }

	// get heigh, witdh of 1st images--> must be the same
    int im_width = images[0].cols;
    int im_height = images[0].rows;
	trace("(init) taille images ok");

    // Criando o FaceRecognizer e treinando a partir das imagens do banco de dados

	// Escolhendo o método matemático para o cálculo do reconhecimento facil	
    	Eigenfaces model;
	
    //	Fisherfaces model; 
    // Treinar o modelo com base no banco de dados	
    trace("(init) start train images");
    model.train(images, labels);
 	trace("(init) train images : ok");
 
	// Carregando modelo de rosto
    CascadeClassifier face_cascade;
    if (!face_cascade.load(fn_haar))
   	{
    			cout <<"(E) face cascade model not loaded :"+fn_haar+"\n"; 
    			return -1;
    }
    trace("(init) Load modele : ok");
    
    // Capturar imagem da webcam
    VideoCapture capture;
 	capture.open(-1);
 		
	// Mensagem em caso de erro ao capturar a imagem
	if (!capture.isOpened())
	{   
		cout << "(E) Capture Device cannot be opened." << endl;
        return -1;
    }
    int nFirst=0;
    // Manter o quadro de vídeo
	for(;;)
	{
		capture.read(original);
		char key;        		
      
   		// Convertendo a escala de cor
        cvtColor(original, gray, cv::COLOR_BGR2GRAY);
        // Equalizar as fotos
        if (bHisto)equalizeHist( gray, gray);        
		
        vector< Rect_<int> > faces;

        // Detectar Faces
		face_cascade.detectMultiScale(gray, faces, 1.1, 3, cv::CASCADE_SCALE_IMAGE, Size(60,60));	
  	  	for(int i = 0; i < faces.size(); i++) 
  	  	{       
            Rect face_i = faces[i];
            face = gray(face_i);
                       	// Redimensionando o rosto e exibindo-o
			cv::resize(face, face_resized, Size(im_width, im_height), 1.0, 1.0, INTER_CUBIC);
			
			if (bFirstDisplay) 
			{
				key = (char) waitKey(100);
				bFirstDisplay = 0;
			}
			else
			{
				key = (char) waitKey(10);
			}
			
			
			nFirst++;
			
			// Rosto detectado
			// Prever de quem é a face detectada
			char sTmp[256];		
			double predicted_confidence	= 0.0;
			int prediction				= -1;
			model.predict(face_resized,prediction,predicted_confidence);
			
			// Criando retangulo no rosto    
			rectangle(original, face_i, CV_RGB(0, 255 ,0), 1);
			i=0;
			
			if (predicted_confidence<PREDICTION_SEUIL)
			{

				sprintf(sTmp,"+ prediction ok = %s (%d) confiance = (%d)",people[prediction].c_str(),prediction,(int)predicted_confidence);
				trace((string)(sTmp));
			
			 	// Escrevendo o nome da pessoa junto ao retângulo
				string box_text;
				switch (prediction) {
					case 0:
					
						box_text = "Pedro";
						res = 0;
						system("chmod u+rx libera.sh && sh libera.sh");
						return res;
						break;
						
					
					case 1:
						box_text = "Gabriela";
						res = 1;
						system("chmod u+rx libera.sh && sh libera.sh");
						return res;
						break;
						
					default:
						trace("Desconhecido");
						res = -1;
						system("cd && ./main");
						return res;
					}
				int pos_x = std::max(face_i.tl().x - 10, 0);
				int pos_y = std::max(face_i.tl().y - 10, 0);			   
				putText(original, box_text, Point(pos_x, pos_y), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0), 1.0);
				
				// Mostrando o resultado
        		imshow("Reco", original);
        		key = (char) waitKey(10);

				}
				else
				{
					string box_text;
					box_text = "Desconhecido";
					sprintf(sTmp,"- prediction too low = %s (%d) confiance = (%d)",people[prediction].c_str(),prediction,(int)predicted_confidence);
					trace((string)(sTmp));
					int pos_x = std::max(face_i.tl().x - 10, 0);
				int pos_y = std::max(face_i.tl().y - 10, 0);			   
				putText(original, box_text, Point(pos_x, pos_y), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0), 1.0);
				
				// Mostrando o resultado
        		imshow("Reco", original);
        		key = (char) waitKey(10);
					
				} 
				
			}
		  
		    imshow("Reco", original);
		    key = (char) waitKey(100);

		}
	return res;
}
