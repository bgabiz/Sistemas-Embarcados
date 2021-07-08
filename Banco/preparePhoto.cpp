#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include <opencv2/core/types_c.h>

#define TRACE 1					
#define DEBUG_MODE 0				
#define DEBUG if (DEBUG_MODE==1)	
using namespace std;
using namespace cv;

int detectAndDisplay( Mat frame );

// Caminho para os arquivos .xml do OpenCV
string face_cascade_name = "/home/pi/test_1/opencv/data/haarcascades/haarcascade_frontalface_alt.xml";
string glasses_cascade_name = "/home/pi/test_1/opencv/data/haarcascades/haarcascade_eye_tree_eyeglasses.xml";
string eyes_cascade_name = "/home/pi/test_1/opencv/data/haarcascades/haarcascade_eye.xml";

CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;
CascadeClassifier glasses_cascade;

CvPoint Myeye_left;
CvPoint Myeye_right;
int bEqHisto;

void trace(string s)
{
	if (TRACE==1)
	{
		cout<<s<<"\n";
	}
}
 
// Compitando distância entre 2 pontos
float Distance(CvPoint p1, CvPoint p2)
{
	int dx = p2.x - p1.x;
	int dy = p2.y - p1.y;
	return sqrt(dx*dx+dy*dy);
}

// Girar a imagem para alinhar os olhos
Mat rotate(Mat& image, double angle, CvPoint centre)
{
    Point2f src_center(centre.x, centre.y);
	// Conversão em graus
	angle = angle*180.0/3.14157;
 	DEBUG printf("(D) rotate : rotating : %fÂ° %d %d\n",angle, centre.x, centre.y);
    Mat rot_matrix = getRotationMatrix2D(src_center, angle, 1.0);

    Mat rotated_img(Size(image.size().height, image.size().width), image.type());

    warpAffine(image, rotated_img, rot_matrix, image.size());
    return (rotated_img);
   }


 
// Cortar a foto
int CropFace(Mat &MyImage, 
	CvPoint eye_left,
	CvPoint eye_right,
	CvPoint offset_pct,
	CvPoint dest_sz)
{

	// Calcular deslocamento da imagem original
	int offset_h = (offset_pct.x*dest_sz.x/100);
	int offset_v = (offset_pct.y*dest_sz.y/100);
	DEBUG printf("(D) CropFace : offeth=%d, offsetv=%d\n",offset_h,offset_v);
	
	CvPoint eye_direction;
	eye_direction.x = eye_right.x - eye_left.x;
	eye_direction.y = eye_right.y - eye_left.y;
	
	float rotation = atan2((float)(eye_direction.y),(float)(eye_direction.x));
	
	// Distância os olhos
	float dist = Distance(eye_left, eye_right);
	DEBUG printf("(D) CropFace : dist=%f\n",dist);
	
	// Calcular a largura do olho de referência
	int reference = dest_sz.x - 2*offset_h;
	
	// Fator de escala
	float scale = dist/(float)reference;
	 DEBUG printf("(D) CropFace : scale=%f\n",scale);
	
	// Girar a imagem original em torno do olho esquerdo
	char sTmp[16];
	sprintf(sTmp,"%f",rotation);
	trace("-- rotate image "+string(sTmp));
	MyImage = rotate(MyImage, (double)rotation, eye_left); 
	
	// Corta a imagem editada
	CvPoint crop_xy;
	crop_xy.x = eye_left.x - scale*offset_h;
	crop_xy.y = eye_left.y - scale*offset_v;
	
	CvPoint crop_size;
	crop_size.x = dest_sz.x*scale; 
	crop_size.y = dest_sz.y*scale;
	
	// Cortar a imagem no tamanho do retângulo definido
	trace("-- crop image");
	DEBUG printf("(D) CropFace : crop_xy.x=%d, crop_xy.y=%d, crop_size.x=%d, crop_size.y=%d",crop_xy.x, crop_xy.y, crop_size.x, crop_size.y);
	
	cv::Rect myROI(crop_xy.x, crop_xy.y, crop_size.x, crop_size.y);
	if ((crop_xy.x+crop_size.x<MyImage.size().width)&&(crop_xy.y+crop_size.y<MyImage.size().height))
		{MyImage = MyImage(myROI);}
	else
		{
			trace("-- error cropping");
			return 0;
		}
  
  // Redimensionando a imagem
  trace("-- resize image");
  cv::resize(MyImage, MyImage, Size(dest_sz));
  
  return 1;
}
void resizePicture(Mat& src, int coeff) 
{
	// Resize src to img size 
	Size oldTaille = src.size(); 
	Size newTaille(coeff,oldTaille.height*coeff/oldTaille.width);
	cv::resize(src, src, newTaille); 
} 

int main( int argc, char** argv )
{
	
	Mat frame;
	
	if (argc!=6) 
	{
		printf("prepare %of_picture_cropped size_of_train_picture image_name_prefix original_picture_new_size equalize_color \nprepare 0.3 100 p 800 1\n");
		return 0;
	}

 	// Lendo os parâmetros
	CvPoint Myoffset_pct;
	Myoffset_pct.x =100.0*atof(argv[1]);
	Myoffset_pct.y = Myoffset_pct.x;
	
	// Tamanho da nova imagem
	CvPoint Mydest_sz;
	Mydest_sz.x = atoi(argv[2]);	
	Mydest_sz.y = Mydest_sz.x;

	// Qualidade e tipo da imagem salva
	std::vector<int> qualityType;
	qualityType.push_back(cv::IMWRITE_JPEG_QUALITY);
	qualityType.push_back(90);

	// Equialização da cor
	bEqHisto = atoi(argv[5]);
	
 	 // Carregando o cascades
	if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };
	if( !eyes_cascade.load( eyes_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };
	if( !glasses_cascade.load( glasses_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };
	
	// Novo tamanho da imagem
	int newSize = atoi(argv[4]);
	
	// Ler diretório atual
	DIR * rep =opendir(".");
	if (rep==NULL) return 0;

	 struct dirent *ent;
	 int index=1;

	while ((ent=readdir(rep)) != NULL)
	{
		 int nLen = strlen(ent->d_name);
		 char * imageName = ent->d_name;
	 
	 	// Ler a extensão e deixar apenas a imagem JPG
	 	if (nLen>4)
	 	if ((imageName[nLen-1]=='g')&&(imageName[nLen-2]=='p')&&(imageName[nLen-3]=='j'))
	 	{
	
		  // Ler fluxo de vídeo
			trace("lecture : "+string(imageName));
		    frame = imread(imageName,1);
		
		 	// Redimensionar imagem
		 	if (frame.size().width>newSize)
		 	{
		 		trace("- image need to be resized");
		 		resizePicture(frame,newSize);
				imwrite(imageName,frame,qualityType);
	
		 	} 
		 
			 if( !frame.empty() )
			 {
			 	trace("- start detect");
				int result = detectAndDisplay( frame );
				if (result==0) trace("- no face detected");
			else 
			{
				// Cortar rosto
				trace ("- start cropFace");	
				if (CropFace(frame, Myeye_left, Myeye_right, Myoffset_pct,Mydest_sz)==1)
				{
				 	char newName[16];
				 	sprintf(newName,"%s%d.jpg",argv[3],index);
				 	string newNameS(newName);
				
					// Convertendo em escala de cinza
				 	Mat grayframe;
				 	trace("- transforme : gray");
				 	cvtColor(frame, grayframe, cv::COLOR_BGR2GRAY);
					
					// Equalizando histograma de cor
					if (bEqHisto==1) 
					{
						trace("- transforme : equalize histo");
						equalizeHist( grayframe, grayframe);        
					}
				 	// Salvando imagem
				 	trace("- save image "+newNameS);
				 	imwrite(newNameS,grayframe,qualityType);
				}
				else
				{
					 trace("- crop face failed");
				}
			}
		}
		index ++;
	}
}
closedir(rep); 
  return 0;
}
int detectAndDisplay( Mat frame )
{
	  std::vector<Rect> faces;
	  Mat frame_gray;
	
	//Convertendo em escala de cinza
	cvtColor( frame, frame_gray, cv::COLOR_BGR2GRAY );
	if (bEqHisto==1) 
	{
	  		equalizeHist( frame_gray, frame_gray );
	}   
	// Detectar face
	face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|cv::CASCADE_SCALE_IMAGE, Size(50, 50) );
	
	
	// Limitando o número de uma face por foto
	DEBUG printf("(D) detectAndDisplay : nb face=%d\n",faces.size());
	if (faces.size()==0) return 0;
	else
	for( size_t i = 0; i < 1; i++ ) // Apenas o primeiro rosto identificado
	{	
	    Point center( faces[i].x + faces[i].width/2, faces[i].y + faces[i].height/2 );

     	Mat faceROI = frame_gray( faces[i] );
     	std::vector<Rect> eyes;

    	// Detectar os olhos em cada face
    	eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |cv::CASCADE_SCALE_IMAGE, Size(30, 30) );
	
	// Para o caso de não ter óculos
	 if (eyes.size()==2)
	   {
	   	  trace("-- face without glasses");
	   	  // Detectar olhos
	      for( size_t j = 0; j < 2; j++ )
	       {
	         Point eye_center( faces[i].x + eyes[1-j].x + eyes[1-j].width/2, faces[i].y + eyes[1-j].y + eyes[1-j].height/2 );
		
	         if (j==0) // Olho esquerdo
	         { 
	         	Myeye_left.x =eye_center.x;
	         	Myeye_left.y =eye_center.y;
	         }
	         if (j==1) // Olho direito
	         { 
	         	Myeye_right.x =eye_center.x;
	         	Myeye_right.y =eye_center.y;
	         }
	  	   }
	   } 
	else
	{
		// Terte com óculos
		glasses_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |cv::CASCADE_SCALE_IMAGE, Size(20, 20) );
	    if (eyes.size()!=2) return 0;
	    else 
	    {
	    	
	   	  trace("-- face with glasses");
	  
		for( size_t j = 0; j < 2; j++ )
	       {
	         Point eye_center( faces[i].x + eyes[1-j].x + eyes[1-j].width/2, faces[i].y + eyes[1-j].y + eyes[1-j].height/2 );
	  	     if (j==0) // Olho esquerdo
	         { 
	         	Myeye_left.x =eye_center.x;
	         	Myeye_left.y =eye_center.y;
	         }
	         if (j==1) // Olho direito
	         { 
	         	Myeye_right.x =eye_center.x;
	         	Myeye_right.y =eye_center.y;
	         }
	       }
	    }
	}
		
	}
	// Verificando a troca de posição na identificação dos olhos
	if (Myeye_right.x<Myeye_left.x)
	{
		int tmpX = Myeye_right.x;
		int tmpY = Myeye_right.y;
		Myeye_right.x=Myeye_left.x;
		Myeye_right.y=Myeye_left.y;
		Myeye_left.x=tmpX;
		Myeye_left.y=tmpY;
		trace("-- oups, switch eyes");
		
	}
	
	return 1;
}
