#include "image_preprocessing.h"

static const char *TAG = "IMGPROC";

void preprocess_image(uint8_t **imgPtr, uint32_t *imgSz) {
	// Decode JPEG in memory
	Mat imgBuf = Mat(1, *imgSz, CV_8UC3, (unsigned char *)*imgPtr);
	Mat imgMat = imdecode(imgBuf, CV_LOAD_IMAGE_COLOR);
	if(imgMat.data == NULL) {
		log_error(TAG, "Unable to decode JPEG frame");
		return;
	}

	// Do the processing needed
	if(get_rotate_cw_90deg()) {
		transpose(imgMat, imgMat);
		flip(imgMat, imgMat, 1);
	}

	// Mat to JPEG
	vector<uchar> buf;
	imencode(".jpg", imgMat, buf, std::vector<int>());

	// Clear current buffer
	free(*imgPtr);
	*imgPtr = (uint8_t *)malloc(buf.size());
	if(*imgPtr == NULL) {
		*imgSz=0;
		log_error(TAG, "Unable to allocate memory for JPEG frame");
		return;
	}

	// Output to original buffer
	memcpy(*imgPtr, &buf[0], buf.size());
	*imgSz = buf.size();
}

void shrink_image(uint8_t **imgPtr, uint32_t *imgSz, uint8_t percentage) {
	// Decode JPEG in memory
	Mat imgBuf = Mat(1, *imgSz, CV_8UC3, (unsigned char *)*imgPtr);
	Mat imgMat = imdecode(imgBuf, CV_LOAD_IMAGE_COLOR);
	if(imgMat.data == NULL) {
		log_error(TAG, "Unable to decode JPEG frame");
		return;
	}

	// Do the processing needed
	resize(imgMat, imgMat, Size(), (float)percentage/100.0f, (float)percentage/100.0f);

	// Mat to JPEG
	vector<uchar> buf;
	imencode(".jpg", imgMat, buf, std::vector<int>());

	// Clear current buffer
	free(*imgPtr);
	*imgPtr = (uint8_t *)malloc(buf.size());
	if(*imgPtr == NULL) {
		*imgSz=0;
		log_error(TAG, "Unable to allocate memory for JPEG frame");
		return;
	}

	// Output to original buffer
	memcpy(*imgPtr, &buf[0], buf.size());
	*imgSz = buf.size();
}
