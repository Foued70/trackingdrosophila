/*
 * Kalman.cpp
 *
 *  Created on: 18/11/2011
 *      Author: german
 */


#include "Kalman.hpp"


// Se genera un track por cada blob ( un track por cada nueva etiqueta).
// En la literatura en general primero se hace la predicción para frame t ( posible estado del blob en el frame t) en base
// al modelo y luego se incorpora la medida de t con la cual se hace la corrección. En nuestro caso el proceso es:
// Se incorpora la medida del frame t. Se hace la corrección y se predice para el frame t+1, de modo que en cada track y para cada
// frame disponemos por un lado de la predicción para el frame t hecha en el t-1, con la cual se hace la corrección, y por otro lado de
// la predicción para el frame t+1.

CvMat* CoordReal;

//Imagenes
IplImage* ImKalman = NULL;

CvMat* Kalman(STFrame* frameData,STFrame* frameData_sig,tlcde* lsIds,tlcde* lsTracks) {

	tlcde* Flies;
	tlcde* Flies_sig;
	STTrack* Track;
	STTrack* Track_caca;
	STFly* Fly = NULL;
	STFly* FlyNext = NULL;



	// cargar datos del frame
	Flies=frameData->Flies;
	Flies_sig = frameData_sig->Flies;


	// Inicializar imagenes y parámetros
	if( !ImKalman ){
		ImKalman = cvCreateImage(cvGetSize(frameData->ImKalman),8,3);

		cvZero(ImKalman);


		CoordReal = cvCreateMat(2,1,CV_32FC1);
	}
	cvZero(frameData->ImKalman);



	////////////////////// CORRECCION ////////////////////////


	if(lsTracks->numeroDeElementos>0){
		for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
			// para cada track, buscar su id en el frame de trabajo //
			Track = (STTrack*)obtener(i, lsTracks);
			int j ;
			if( Flies->numeroDeElementos>0){
				for(j = 0; j< Flies->numeroDeElementos ;j++ ){
					Fly = (STFly*)obtener(j, frameData->Flies);
					// si se encuentra la id (caso general)
					if(Fly->etiqueta == Track->id){
						// Añadimos la nueva medida
						generarMedida( Track, Fly );
						// actualizamos kalman
						cvKalmanCorrect( Track->kalman, Track->z_k);
						// actualizamos parámetros
						Fly->dir_filtered =Track->x_k_Pos->data.fl[4] ;
						break;
					}
					// llegamos al último elemento y no se encuentra la id.
					if( (j == Flies->numeroDeElementos)){

						//si se asigna a una id0 junto con otros tracks ( caso normal de solapamiento de trayectorias):
						//generarMedida2( Track, Fly );
						//cvKalmanCorrect( Track->kalman, Track->z_k);
						//Track->x_k_Pos = Track->kalman->state_post;
						//Track->P_k_Pos = Track->kalman->error_cov_post;

						// si no se encuentra su id el frame y no hay ninguna asignación válida corregimos con la predicción anterior de kalman.
						//podría tratarse de un trackdead. Kalman toma el control ( Se genera la nueva medida a partir de las predicciones).
						kalmanControl( Track );
						cvKalmanCorrect( Track->kalman, Track->z_k);
						Fly->dir_filtered =Track->x_k_Pos->data.fl[4] ;
					}
				}
			}
			// si no hay elementos en el frame ( frame erróneo o perdido) , corregimos con la predicción anterior de kalman.
			else{
				kalmanControl( Track );
				cvKalmanCorrect( Track->kalman, Track->z_k);
				Fly->dir_filtered =Track->x_k_Pos->data.fl[4] ;
			}
		}
	}

	////// Comprobar si es necesario crear nuevos tracks ( nuevas ids ). En caso afirmativo
	// crearlos e inicializar filtro de kalman para cada nuevo track ( blob )
	initNewsTracks( frameData, lsTracks );

	////////////////////// PREDICCION ////////////////////////
	//Recorremos cada track y hacemos la predicción para el siguiente frame
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		Track = (STTrack*)obtener(i, lsTracks);
		if( Track->x_k_Pre != NULL ){
			cvCopy(Track->x_k_Pre,Track->x_k_Pre_);
			cvCopy(Track->P_k_Pre,Track->P_k_Pre_);
//			memcpy( Track->x_k_Pre_->data.fl, Track->x_k_Pre->data.fl, sizeof(Track->x_k_Pre));
//			memcpy( Track->P_k_Pre_->data.fl, Track->P_k_Pre->data.fl, sizeof(Track->P_k_Pre));
		}

		Track->x_k_Pre = cvKalmanPredict( Track->kalman, 0);
	}

	// ESTABLECER LA MATRIZ DE PESOS CON LA PROBABILIDAD ENTRE LA PREDICCION Para T+1 Y EL VALOR OBSERVADO EN T + 1

	CvMat* Matrix_Hungarian = cvCreateMat(lsTracks->numeroDeElementos,Flies_sig->numeroDeElementos,CV_32FC1);

	if(lsTracks->numeroDeElementos>0 && Flies_sig->numeroDeElementos >0){
//	CvMat* Matrix_Hungarian = cvCreateMat(lsTracks->numeroDeElementos,Flies_sig->numeroDeElementos,CV_32FC1);
	cvZero(Matrix_Hungarian);

	double Hungarian_Matrix [lsTracks->numeroDeElementos][Flies_sig->numeroDeElementos];

		int p=0;

		for(int d=0;d < lsTracks->numeroDeElementos;d++){

			Track=(STTrack*)obtener(d,lsTracks);


				for(int f=0;f < Flies_sig->numeroDeElementos;f++){


				FlyNext=(STFly*)obtener(f,Flies_sig);

					CoordReal->data.fl[0]=FlyNext->posicion.x;
					CoordReal->data.fl[1]=FlyNext->posicion.y;

					double Peso = PesosKalman(Track->Measurement_noise_cov,Track->x_k_Pre,CoordReal);
					Matrix_Hungarian->data.fl[p] = Peso;
					Hungarian_Matrix[d][f]=Peso;
					p++;
				}
		}

				printf("\n***************************************************************");
				for(int m=0;m<lsTracks->numeroDeElementos;m++){
					printf("\n");
					for(int n=0;n<Flies_sig->numeroDeElementos;n++){
						printf("\t %f",Hungarian_Matrix[m][n]);
					}
				}

	}


	///////////////////// VISUALIZAR RESULTADOS ////////////////

	visualizarKalman( frameData, lsTracks );

	return Matrix_Hungarian;

}// Fin de Kalman

CvMat* Kalman2(STFrame* frameData,STFrame* frameData_sig,tlcde* lsIds,tlcde* lsTracks) {


	tlcde* Flies;
	tlcde* Flies_sig;
	STTrack* Track;
	STTrack* Track_caca;
	STFly* Fly = NULL;
	STFly* FlyNext = NULL;


	// cargar datos del frame
	Flies=frameData->Flies;
	Flies_sig = frameData_sig->Flies;

	// Inicializar imagenes y parámetros
	if( !ImKalman ){
		ImKalman = cvCreateImage(cvGetSize(frameData->ImKalman),8,3);
		cvZero(ImKalman);
		CoordReal = cvCreateMat(2,1,CV_32FC1);
	}
	cvZero(frameData->ImKalman);

	////////////////////// CORRECCION ////////////////////////
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		// obtener Track
		Track = (STTrack*)obtener(i, lsTracks);
		// genera la nueva medida
		generarZ_k( Track );
		// corregir kalman
		cvKalmanCorrect( Track->kalman, Track->z_k);
		// actualizamos parámetros
		Fly->dir_filtered =Track->x_k_Pos->data.fl[4] ;
	}

	// ANYADIR NUEVOS TRACKS, si es necesario.
	// si hay mosca/s sin asignar a un track/s (id = 0), se crea un nuevo track para esa mosca
	for(int i = 0; i< Flies->numeroDeElementos ;i++ ){
		Fly = (STFly*)obtener(i, Flies );
		if( Fly->etiqueta == 0) anyadirAlFinal(initTrack( Fly, lsIds , frameData->Stats->fps), lsTracks );
	}

	////////////////////// PREDICCION ////////////////////////
	//Recorremos cada track y hacemos la predicción para el siguiente frame
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		Track = (STTrack*)obtener(i, lsTracks);
		if( Track->x_k_Pre != NULL ){
			// guardamos los datos de la predicción anterior
			cvCopy(Track->x_k_Pre,Track->x_k_Pre_);
			cvCopy(Track->P_k_Pre,Track->P_k_Pre_);
		}
		// nueva predicción
		Track->x_k_Pre = cvKalmanPredict( Track->kalman, 0);
	}

	// ESTABLECER LA MATRIZ DE PESOS CON LA PROBABILIDAD ENTRE LA PREDICCION Para T+1 Y EL VALOR OBSERVADO EN T + 1

	CvMat* Matrix_Hungarian = cvCreateMat(lsTracks->numeroDeElementos,Flies_sig->numeroDeElementos,CV_32FC1);
	if(lsTracks->numeroDeElementos>0 && Flies_sig->numeroDeElementos >0){
	cvZero(Matrix_Hungarian);

	double Hungarian_Matrix [lsTracks->numeroDeElementos][Flies_sig->numeroDeElementos];

		int p=0;

		for(int d=0;d < lsTracks->numeroDeElementos;d++){

			Track=(STTrack*)obtener(d,lsTracks);


				for(int f=0;f < Flies_sig->numeroDeElementos;f++){


				FlyNext=(STFly*)obtener(f,Flies_sig);

					CoordReal->data.fl[0]=FlyNext->posicion.x;
					CoordReal->data.fl[1]=FlyNext->posicion.y;

					double Peso = PesosKalman(Track->Measurement_noise_cov,Track->x_k_Pre,CoordReal);
					Matrix_Hungarian->data.fl[p] = Peso;
					Hungarian_Matrix[d][f]=Peso;
					p++;
				}
		}

				printf("\n***************************************************************");
				for(int m=0;m<lsTracks->numeroDeElementos;m++){
					printf("\n");
					for(int n=0;n<Flies_sig->numeroDeElementos;n++){
						printf("\t %f",Hungarian_Matrix[m][n]);
					}
				}

	}


	///////////////////// VISUALIZAR RESULTADOS ////////////////
	visualizarKalman( frameData, lsTracks );

	return Matrix_Hungarian;
}// Fin de Kalman

// OBSOLETA Compara la lista tracks y la lista flies. Si se ha asignado una nueva id, se inicia un nuevo track,
void initNewsTracks( STFrame* frameData, tlcde* lsTracks ){

	STTrack* Track = NULL;
	STFly* Fly = NULL;

	// En primera iteración iniciar un track para cada id
	if( lsTracks->numeroDeElementos == 0){
		// INICIAR TRACK
		for(int i = 0; i < frameData->Flies->numeroDeElementos;i++){
			Fly = (STFly*)obtener(i, frameData->Flies);
			Track = initTrack( Fly ,NULL,frameData->Stats->fps );
			anyadirAlFinal(Track, lsTracks );
		}
	}
	else{
		// recorrer lista flies y comprueba si cada id tiene su correspondiente track. Si no lo tiene, se crea.
		for(int i = 0; i < frameData->Flies->numeroDeElementos ;i++){
			Fly = (STFly*)obtener(i, frameData->Flies);

			int j ;
			for( j = 0; j < lsTracks->numeroDeElementos ; j++){
				Track = (STTrack*)obtener(j, lsTracks);
				if(Fly->etiqueta == Track->id) break;
			}// si la j llega al final quiere decir que no se ha encontrado coincidencia
			if (j == lsTracks->numeroDeElementos){
				// INICIAR TRACK
				Track = initTrack( Fly ,NULL,frameData->Stats->fps );
				anyadirAlFinal(Track, lsTracks );
			}
		}
	}
}

// etiquetar fly e iniciar nuevo track

STTrack* initTrack( STFly* Fly ,tlcde* ids, float fps ){

	STTrack* Track = NULL;

	Track = (STTrack*)malloc(sizeof(STTrack));
	if(!Track) {error(4); exit(1);}

//	asignarNuevaId( Fly , ids );

	Track->id = Fly->etiqueta;
	// iniciar kalman
	Track->kalman = initKalman( Fly , fps);

	Track->x_k_Pre_ = cvCreateMat( 5,1, CV_32FC1 );
	Track->P_k_Pre_ = cvCreateMat( 5,5, CV_32FC1 );
	cvZero(Track->x_k_Pre_);
	cvZero(Track->P_k_Pre_);

	Track->x_k_Pos = Track->kalman->state_post ;
	Track->P_k_Pos = Track->kalman->error_cov_post;

	Track->x_k_Pre = NULL; // es creada por kalman
	Track->P_k_Pre = Track->kalman->error_cov_pre;

	Track->Medida = cvCreateMat( 5, 1, CV_32FC1 );
	Track->Measurement_noise = cvCreateMat( 5, 1, CV_32FC1 );
	Track->Measurement_noise_cov = Track->kalman->measurement_noise_cov;
	Track->z_k = cvCreateMat( 5, 1, CV_32FC1 );
	cvZero(Track->Medida);
	cvZero(Track->z_k );

	return Track;
	//		direccionR = (flyData->direccion*CV_PI)/180;
	// Calcular e iniciar estado

}
CvKalman* initKalman( STFly* Fly, float dt ){

	float Vx=0;// componente X de la velocidad.
	float Vy=0;// componente Y de la velociad.

	// Crear el flitro de Kalman

	CvKalman* Kalman = cvCreateKalman(5,5,0);
	dt = 1 / 1 ;

	// Inicializar las matrices parámetros para el filtro de Kalman
	// en la primera iteración iniciamos la dirección con la orientación

	Vx= VELOCIDAD*cos(Fly->orientacion*CV_PI/180);
	Vy=-VELOCIDAD*sin(Fly->orientacion*CV_PI/180);

	const float F[] = {1,0,dt,0,0, 0,1,0,dt,0, 0,0,1,0,0, 0,0,0,1,0, 0,0,0,0,1}; // Matriz de transición F
	// Matriz R inicial. Errores en medidas. ( x, y, Vx, Vy, phi ). Al inicio el error en el angulo es de +-180º
	const float R_inicial[] = {50,0,0,0,0, 0,50,0,0,0, 0,0,50,0,0, 0,0,0,50,0, 0,0,0,0,360};//{50,50,50,50,180};
	const float X_k[] = { Fly->posicion.x, Fly->posicion.y, Vx, Vy, Fly->orientacion };// Matiz de estado inicial
	float initialState[] = {Fly->posicion.x, Fly->posicion.y, Vx, Vy, Fly->orientacion};
//
	CvMat x_k =cvMat(1, 5, CV_32FC1, initialState);

	// establecer parámetros
	memcpy( Kalman->transition_matrix->data.fl, F, sizeof(F)); // F

	cvSetIdentity( Kalman->measurement_matrix,cvRealScalar(1) ); // H Matriz de medida
	cvSetIdentity( Kalman->process_noise_cov,cvRealScalar(1 ) ); // error asociado al modelo del sistema Q = f(F,B y H).
	cvSetIdentity( Kalman->error_cov_pre, cvScalar(1) ); // Incertidumbre en predicción. (P_k' = F P_k-1 Ft) + Q
	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(1)); // Incertidumbre tras añadir medida. P_k = ( I - K_k H) P_k'

	memcpy( Kalman->measurement_noise_cov->data.fl, R_inicial, sizeof(R_inicial)); // Incertidumbre en la medida Vk; Vk->N(0,R)
//	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(1)); // Incertidumbre tras añadir medida. P_k = ( I - K_k H) P_k'
	//copyMat(&x_k, Kalman->state_post);
	memcpy( Kalman->state_post->data.fl, X_k, sizeof(X_k)); // Estado inicial


	return Kalman;
}

// Generamos la medida a partir de los datos obtenidos de la camara añadiendo incertidumbre.
void generarZ_k( STTrack* Track){

	//Caso 0: la medida se genera a partir de las predicciones de kalman
	// La incertidumbre en la medida de posición será muy elevada ( no se ha obtenido nueva medida se los sensores).
	if( !Track->Flysig ){
		kalmanControl( Track );
	}
	//Caso 1: caso normal. Se ha encontrado una asignación para un track
	// La incertidumbre en la medida será baja.
	else if( Track->Flysig->Tracks->numeroDeElementos == 1 ){
		generarMedida( Track, Track->Flysig );
	}
	//Caso2 : hay varios tracks que apuntan al mismo blob.Este estará formado por dos o más moscas. Caso general de
	// cruce de trayectorias
	else  generarMedida2( Track, Track->Flysig );
}
// generar medida para el frame t
// en este caso la medida se genera a partir de las predicciones de kalman añadiendo una incertidumbre elevada
void kalmanControl( STTrack* Track ){

	// Matriz de entrada
	CvMat* H = Track->kalman->measurement_matrix;

	float R[] =  {5,0,0,0,0, 0,5,0,0,0, 0,0,5,0,0, 0,0,0,5,0, 0,0,0,0,180};// Covarianza
	float V[] =  {0,0,0,0,0};// media
	memcpy( Track->kalman->measurement_noise_cov->data.fl, R, sizeof(R)); // R;
	// y en la matriz para aplicarselo a la medida
	memcpy( Track->Measurement_noise->data.fl, V, sizeof(V));
	cvGEMM(H,  Track->x_k_Pre ,1, Track->Measurement_noise, 1, Track->z_k,0 ); // Zk = H Medida + V

}

// Generamos la medida a partir de los datos obtenidos de la cámara añadiendo una incertidumbre baja. En la orientación
// será elevada pues es kalman quien corrige dicho parámetro
void generarMedida( STTrack* Track, STFly* Fly ){

	const CvMat* H = Track->kalman->measurement_matrix;


	int a = 0;
	int b = 0;
	float phi = 0;
	float distancia = 0;
	// generar direcion a partir de la medida anterior y la predicción de la posicion de kalman
	a = Track->x_k_Pre->data.fl[0] - Track->z_k->data.fl[0];
	b = Track->x_k_Pre->data.fl[1] - Track->z_k->data.fl[1];
	//EUDistance( a,  b, &phi, &distancia);

	//Nueva Medida
	const float Medida[] = { Fly->posicion.x, Fly->posicion.y, Fly->Vx, Fly->Vy,Fly->direccion};
	memcpy( Track->Medida->data.fl, Medida, sizeof(Medida)); // Medida;

	// incertidumbre en la medida
	// para el ángulo: Si la dirección del blob difiere en más de 90º de la dirección anterior, aumentar la incertidumbre.
	float R[] =  {3,0,0,0,0, 0,3,0,0,0, 0,0,3,0,0, 0,0,0,3,0, 0,0,0,0,180};//covarianza del ruido
	float V[] = {0,0,0,0,0}; // media del ruido
	// cargamos el error en la medida en el filtro para calcular la ganancia de kalman
	memcpy( Track->kalman->measurement_noise_cov->data.fl, R, sizeof(R)); // V->N(0,R);
	// y en la matriz para aplicarselo a la medida
	memcpy( Track->Measurement_noise->data.fl, V, sizeof(V));
//	cvMatMulAdd( H->data.fl, Track->Medida->data.fl, Track->kalman->measurement_noise_cov->data.fl, Track->z_k->data.fl ); // Zk = H Medida + R
//	cvGEMM(H->data.fl, Track->Medida->data.fl,1, Track->kalman->measurement_noise_cov->data.fl, 1, Track->z_k->data.fl,0 ); // Zk = H Medida + R
	cvGEMM(H, Track->Medida,1, Track->Measurement_noise, 1, Track->z_k,0 ); // Zk = H Medida + V

}

// En este caso la incertidumbre será mayor, pues
// hay varios tracks que apuntan al mismo blob.
void generarMedida2( STTrack* Track, STFly* Fly  ){

	CvMat* H = Track->kalman->measurement_matrix;

	// Medida
	const float Medida[] = { Fly->posicion.x, Fly->posicion.y, Fly->Vx, Fly->Vy, Fly->direccion};
	memcpy( Track->Medida->data.fl, Medida, sizeof(Medida)); // Medida;
	// incertidumbre en la medida
	float R[] =  {5,0,0,0,0, 0,5,0,0,0, 0,0,5,0,0, 0,0,0,5,0, 0,0,0,0,180};// covarianza
	float V[] =  {0,0,0,0,0};//media
	memcpy( Track->kalman->measurement_noise_cov->data.fl, R, sizeof(R)); // V->N(0,R);
	// y en la matriz para aplicarselo a la medida
	memcpy( Track->Measurement_noise->data.fl, V, sizeof(V));
	// Obtener el error en la velocidad y orientación a partir del error en la medida.
	//¿¿  1 asignar la id0 con el centro el asignado en la predicción con un error de posición igual al diametro del blob??
	// o bien  2  ( como el el libro ) ???? o bien usamos EM para estimar los centros?
	//La velocidad la predicha
	// aumentamos la incertidumbre en la medida estableciendo unos valores altos en la matriz de covarianza.
	// Si 1, En caso de la incertidumbre en la velocidad, ésta tenderá a 0 a medida que pasen frames sin que aparezca el blob, de modo que
	// kalman predecirá que la mosca se está parando  o está quieta (V proxima a 0).

	cvGEMM(H, Track->Medida,1, Track->Measurement_noise, 1, Track->z_k,0 ); // Zk = H Medida + V
}




double PesosKalman(const CvMat* Matrix,const CvMat* Predict,CvMat* CordReal){

	float Matrix_error_cov[] = {Matrix->data.fl[0], Matrix->data.fl[6]};

	float X = CordReal->data.fl[0];
	float Y = CordReal->data.fl[1];
	float EX = Predict->data.fl[0];
	float EY = Predict->data.fl[1];
//	float VarX = sqrt(Matrix_error_cov[0]);
//	float VarY = sqrt(Matrix_error_cov[1]);

	float VarX = sqrt(50);
	float VarY = sqrt(50);

	double ValorX,ValorY;
	double DIVX,DIVY;
	double ProbKalman;

	ValorX=X-EX;
	ValorY=Y-EY;
	DIVX=-((ValorX/VarX)*(ValorX/VarX))/2;
	DIVY=-((ValorY/VarY)*(ValorY/VarY))/2;

	ProbKalman =exp (-abs(DIVX + DIVY));

	ProbKalman = 100*ProbKalman;

	if (ProbKalman < 1) ProbKalman = 1;

	return ProbKalman;

}
CvRect ROIKalman(CvMat* Matrix,CvMat* predict){

	float Matrix_Kalman_Cov[] = {Matrix->data.fl[0], Matrix->data.fl[1], Matrix->data.fl[4], Matrix->data.fl[5]};

	float Valor_X,Valor_Y;

	CvRect ROI;

	Valor_X = sqrt(Matrix_Kalman_Cov[0]);
	Valor_Y = sqrt(Matrix_Kalman_Cov[3]);

	ROI.x=(predict->data.fl[0]-Valor_X);
	ROI.y=(predict->data.fl[1]-Valor_Y);
	ROI.width=2*Valor_X;
	ROI.height=2*Valor_Y;

	return ROI;

}

void visualizarKalman( STFrame* frameData, tlcde* lsTracks) {

	STTrack* Track;

	if( SHOW_KALMAN_DATA || (SHOW_VISUALIZATION && SHOW_KALMAN) ){
		for(int i = 0; i < lsTracks->numeroDeElementos ; i++){
			Track = (STTrack*)obtener(i, lsTracks);
			// EN CONSOLA
			if(SHOW_KALMAN_DATA ){
				printf("\n*********************** FRAME %d; BLOB NUMERO %d ************************",frameData->num_frame,Track->id);
				showKalmanData( Track );
			}
			if(SHOW_VISUALIZATION && SHOW_KALMAN){
				// EN IMAGEN
				cvCircle(ImKalman,cvPoint(cvRound(Track->z_k->data.fl[0]),cvRound(Track->z_k->data.fl[1] )),1,CVX_BLUE,-1,8); // observado ( con el error de medida)
				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pre_->data.fl[0]),cvRound(Track->x_k_Pre_->data.fl[1] )),1,CVX_RED,-1,8); // predicción en t
				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pos->data.fl[0]),cvRound(Track->x_k_Pos->data.fl[1])),1,CVX_WHITE,-1,8); // Corrección en t
				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pre->data.fl[0]),cvRound(Track->x_k_Pre->data.fl[1])),1,CVX_GREEN,-1,8); // predicción t+1

			}
		}
		cvAdd(ImKalman,frameData->ImKalman,frameData->ImKalman );
	}
}
void showKalmanData( STTrack *Track){

	printf("\n\nReal State: ");
	printf("\n\tCoordenadas: ( %f , %f )",Track->Medida->data.fl[0],Track->Medida->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->Medida->data.fl[2],Track->Medida->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->Medida->data.fl[4]);

	printf("\n\n Observed State: "); // sumandole el error de medida.
	printf("\n\tCoordenadas: ( %f , %f )",Track->z_k->data.fl[0],Track->z_k->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->z_k->data.fl[2],Track->z_k->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->z_k->data.fl[4]);

	printf("\n Media Measurement noise:\n\n\t%0.f\n\t%0.f\n\t%0.f\n\t%0.f\n\t%0.f",
			Track->Measurement_noise->data.fl[0],
			Track->Measurement_noise->data.fl[1],
			Track->Measurement_noise->data.fl[2],
			Track->Measurement_noise->data.fl[3],
			Track->Measurement_noise->data.fl[4]);

	printf("\n Error Process noise:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->Measurement_noise_cov->data.fl[0],
			Track->Measurement_noise_cov->data.fl[6],
			Track->Measurement_noise_cov->data.fl[12],
			Track->Measurement_noise_cov->data.fl[18],
			Track->Measurement_noise_cov->data.fl[24]);

	printf("\n Error Process noise:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->kalman->process_noise_cov->data.fl[0],
			Track->kalman->process_noise_cov->data.fl[6],
			Track->kalman->process_noise_cov->data.fl[12],
			Track->kalman->process_noise_cov->data.fl[18],
			Track->kalman->process_noise_cov->data.fl[24]);


	printf("\n\n Predicted State t:");
	printf("\n\tCoordenadas: ( %f , %f )",Track->x_k_Pre_->data.fl[0],Track->x_k_Pre_->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->x_k_Pre_->data.fl[2],Track->x_k_Pre_->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->x_k_Pre_->data.fl[4]);
	printf("\n Error Cov Predicted:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->P_k_Pre_->data.fl[0],
			Track->P_k_Pre_->data.fl[6],
			Track->P_k_Pre_->data.fl[12],
			Track->P_k_Pre_->data.fl[18],
			Track->P_k_Pre_->data.fl[24]);

	printf("\n\n Corrected State:" );
	printf("\n\tCoordenadas: ( %f , %f )",Track->x_k_Pos->data.fl[0],Track->x_k_Pos->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->x_k_Pos->data.fl[2],Track->x_k_Pos->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->x_k_Pos->data.fl[4]);
	printf("\n Error Cov Corrected:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->P_k_Pos->data.fl[0],
			Track->P_k_Pos->data.fl[6],
			Track->P_k_Pos->data.fl[12],
			Track->P_k_Pos->data.fl[18],
			Track->P_k_Pos->data.fl[24]);

	printf("\n\n Predicted State t+1:");
	printf("\n\tCoordenadas: ( %f , %f )",Track->x_k_Pre->data.fl[0],Track->x_k_Pre->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->x_k_Pre->data.fl[2],Track->x_k_Pre->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->x_k_Pre->data.fl[4]);
	printf("\n Error Cov Predicted:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->P_k_Pre->data.fl[0],
			Track->P_k_Pre->data.fl[6],
			Track->P_k_Pre->data.fl[12],
			Track->P_k_Pre->data.fl[18],
			Track->P_k_Pre->data.fl[24]);
	printf("\n\n");
}

int deadTrack( tlcde* Tracks, int id ){

	STTrack* Track = NULL;
	Track = (STTrack*)borrarEl( id , Tracks);
	if(Track) {
		cvReleaseKalman(&Track->kalman);

		cvReleaseMat( &Track->x_k_Pre_ );

		cvReleaseMat( &Track->P_k_Pre_ );

		cvReleaseMat( &Track->Medida );
		cvReleaseMat(&Track->z_k );
		cvReleaseMat(&Track->Measurement_noise );
		free(Track);
		Track = NULL;
		return 1;
	}
	else return 0;
}

void liberarTracks( tlcde* lista){
	// Borrar todos los elementos de la lista
	STTrack *Track = NULL;
	// Comprobar si hay elementos
	if(lista == NULL || lista->numeroDeElementos<1)  return;
	// borrar: borra siempre el elemento actual

	irAlPrincipio(lista);
	Track = (STTrack *)borrar(lista);
	while (Track)
	{
		cvReleaseKalman(&Track->kalman);

		cvReleaseMat( &Track->x_k_Pre_ );

		cvReleaseMat( &Track->P_k_Pre_ );

		cvReleaseMat( &Track->Medida );
		cvReleaseMat(&Track->z_k );
		cvReleaseMat(&Track->Measurement_noise );


		free(Track); // borrar el área de datos del elemento eliminado
		Track = NULL;
		Track = (STTrack *)borrar(lista);
	}
}

/// Limpia de la memoria las imagenes usadas durante la ejecución
void DeallocateKalman( tlcde* lista ){

	cvReleaseImage( &ImKalman);

	cvReleaseMat( &CoordReal);

	liberarTracks( lista);

}

void copyMat (CvMat* source, CvMat* dest){

	int i,j;

	for (i=0; i<source->rows; i++)
		for (j=0; j<source->cols;j++)
			dest->data.fl[i*source->cols+j]=source->data.fl[i*source->cols+j];

}