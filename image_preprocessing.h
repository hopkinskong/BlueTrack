#ifndef IMAGE_PREPROCESSING_H_
#define IMAGE_PREPROCESSING_H_




#ifdef __cplusplus
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#endif /* __cplusplus */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "app_settings.h"
void preprocess_image(uint8_t **imgPtr, uint32_t *imgSz);
void shrink_image(uint8_t **imgPtr, uint32_t *imgSz, uint8_t percentage);
#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* IMAGE_PREPROCESSING_H_ */
