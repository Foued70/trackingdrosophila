/*!
 * BGModel.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef BGMODEL_H_
#define BGMODEL_H_

#include "VideoTracker.hpp"
#include "Libreria.h"

#define FRAMES_TRAINING 20
#define ALPHA 0.5

// VARIABLES GLOBALES DE PROGRAMA //

	// Float 1-Channel
	IplImage *IdiffF;
	IplImage *IvarF; /// Varianza
	IplImage *IhiF; /// La mediana mas x veces la desviación típica
	IplImage *IlowF; /// La mediana menos x veces la desviación típica


	//Byte 1-Channel
	IplImage *ImGray;
	IplImage *Imaskt;

// PROTOTIPOS DE FUNCIONES //



//! \brief Inicializa el modelo de fondo como una Gaussiana con una estimación
//! de la mediana y de la de desviación típica según:
//! Para el aprendizaje del fondo toma un número de frames igual a FRAMES_TRAINING
/*!
	  \param t_capture : Imagen fuente de 8 bit de niveles de gris preprocesada.
	  \param BG : Imagen fuente de 8 bit de niveles de gris. Contiene la estimación de la mediana de cada pixel
	  \param ImMask : Máscara  de 8 bit de niveles de gris para el preprocesdo (extraccion del plato).
	*/
int initBGGModel( CvCapture* t_capture, IplImage* BG, IplImage* ImMask);

IplImage* getBinaryImage(IplImage * image);
//! \brief Recibe una imagen en escala de grises preprocesada. En la primera ejecución inicializa el modelo. Estima a la mediana
//!	en BGMod y la varianza en IvarF según:
/*! Mediana:
//!	\f[
//!		mu(p)= median\I_t(p)
//!		\f]
    /*
      \param ImGray : Imagen fuente de 8 bit niveles de gris.
      \param BGMod : Imagen de fondo sobre la que se estima la mediana
    */

void accumulateBackground( IplImage* ImGray, IplImage* BGMod);
//! \brief Recibe una imagen en escala de grises preprocesada. estima a la mediana
//!	en BGMod y la varianza en IvarF según:
/*! Mediana:
//!	\f[
//!		mu(p)= median\I_t(p)
//!		\f]
    /*
      \param ImGray : Imagen fuente de 8 bit niveles de gris.
      \param BGMod : Imagen de fondo sobre la que se estima la mediana
    */
void UpdateBackground(IplImage * tmp_frame, IplImage* bg_model, CvRect DataROI);

//! \brief Crea una mascara binaria (0,255) donde 255 significa primer  plano
/*!
      \param ImGray : Imagen fuente de 8 bit de niveles de gris preprocesada.
      \param bg_model : Imagen fuente de 8 bit de niveles de gris. Contiene la estimación de la mediana de cada pixel
      \param fg : Imagen destino ( máscara ) de 8 bit de niveles de gris.
      \param HiF: : Umbral alto.
      \param LowF: : Umbral bajo.
    */

void RunningBGGModel( IplImage* Image, IplImage* median, IplImage* IdesvT, double alpha, CvRect dataroi );
//! \brief Crea una mascara binaria (0,255) donde 255 significa primer  plano
/*!
      \param ImGray : Imagen fuente de 8 bit de niveles de gris preprocesada.
      \param bg_model : Imagen fuente de 8 bit de niveles de gris. Contiene la estimación de la mediana de cada pixel
      \param fg : Imagen destino ( máscara ) de 8 bit de niveles de gris.
      \param HiF: : Umbral alto.
      \param LowF: : Umbral bajo.
    */


void BackgroundDifference( IplImage* ImGray, IplImage* bg_model,IplImage* fg, int HiF, int LowF );
//! \brief Establece el Umbral alto como la mediana mas HiF veces la desviación típica
/*!
      \param BG : Imagen fuente de 8 bit de niveles de gris que contine la estimación de la mediana de cada píxel.
      \param HiF: : Umbral alto.
    */
void setHighThreshold( IplImage* BG, int HiF );
//! \brief Establece el Umbral bajo como la mediana menos LowF veces la desviación típica
/*!
      \param BG : Imagen fuente de 8 bit de niveles de gris que contine la estimación de la mediana de cada píxel.
      \param LowF: : Umbral bajo.
    */
void setLowThreshold( IplImage* BG, int LowF );
void error(int err);
void AllocateImagesBGM( IplImage *I );
void DeallocateImagesBGM();


#endif /* BGMODEL_H_ */
