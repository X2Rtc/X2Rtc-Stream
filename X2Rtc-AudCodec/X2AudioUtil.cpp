#include <stdio.h>
#include "X2AudioUtil.h"

#define MAX_INT 32767
#define MIN_INT -32767
#define CHECK_MAX_VALUE(value) ((value > 32767) ? 32767 : value)
#define CHECK_MIN_VALUE(value) ((value < -32767) ? -32767 : value)

int X2R_CALL X2MixAudio(int iChannelNum, short* sourceData1, short* sourceData2, float fBackgroundGain, float fWordsGain, short *outputData)
{
	double f = 1.0;
	if (iChannelNum <= 0) {
		return -1;
	}
	if (iChannelNum > 2) {
		return -2;
	}

	if (iChannelNum == 2) {
		float fLeftValue1 = 0;
		float fRightValue1 = 0;
		float fLeftValue2 = 0;
		float fRightValue2 = 0;
		float fLeftValue = 0;
		float fRightValue = 0;
		int output = 0;
		int iIndex = 0;

		fLeftValue1 = (float)(sourceData1[0]);
		fRightValue1 = (float)(sourceData1[1]);
		fLeftValue2 = (float)(sourceData2[0]);
		fRightValue2 = (float)(sourceData2[1]);
		fLeftValue1 = fLeftValue1 * fBackgroundGain;
		fRightValue1 = fRightValue1 * fBackgroundGain;
		fLeftValue2 = fLeftValue2 * fWordsGain;
		fRightValue2 = fRightValue2 * fWordsGain;
		fLeftValue1 = CHECK_MAX_VALUE(fLeftValue1);
		fLeftValue1 = CHECK_MIN_VALUE(fLeftValue1);
		fRightValue1 = CHECK_MAX_VALUE(fRightValue1);
		fRightValue1 = CHECK_MIN_VALUE(fRightValue1);
		fLeftValue2 = CHECK_MAX_VALUE(fLeftValue2);
		fLeftValue2 = CHECK_MIN_VALUE(fLeftValue2);
		fRightValue2 = CHECK_MAX_VALUE(fRightValue2);
		fRightValue2 = CHECK_MIN_VALUE(fRightValue2);
		fLeftValue = fLeftValue1 + fLeftValue2;
		fRightValue = fRightValue1 + fRightValue2;

		for (iIndex = 0; iIndex < 2; iIndex++) {

			if (iIndex == 0) {
				output = (int)(fLeftValue*f);
			}
			else {
				output = (int)(fRightValue*f);
			}
			if (output > MAX_INT)
			{
				f = (double)MAX_INT / (double)(output);
				output = MAX_INT;
			}
			if (output < MIN_INT)
			{
				f = (double)MIN_INT / (double)(output);
				output = MIN_INT;
			}
			if (f < 1)
			{
				f += ((double)1 - f) / (double)32;
			}
			outputData[iIndex] = (short)output;
		}
	}
	else {
		float fValue1 = 0;
		float fValue2 = 0;
		float fValue = 0;

		fValue1 = (float)(*(short*)(sourceData1));
		fValue2 = (float)(*(short*)(sourceData2));
		fValue1 = fValue1 * fBackgroundGain;
		fValue2 = fValue2 * fWordsGain;
		fValue = fValue1 + fValue2;

		fValue = CHECK_MAX_VALUE(fValue);
		fValue = CHECK_MIN_VALUE(fValue);
		*outputData = (short)fValue;
	}
	return 1;
}

int X2R_CALL X2VolAudio(int iChannelNum, short* sourceData1, float fBackgroundGain)
{
	double f = 1.0;
	if (iChannelNum <= 0) {
		return -1;
	}
	if (iChannelNum > 2) {
		return -2;
	}

	if (iChannelNum == 2) {
		float fLeftValue1 = 0;
		float fRightValue1 = 0;
		float fLeftValue = 0;
		float fRightValue = 0;
		int output = 0;
		int iIndex = 0;

		fLeftValue1 = (float)(sourceData1[0]);
		fRightValue1 = (float)(sourceData1[1]);
		fLeftValue1 = fLeftValue1 * fBackgroundGain;
		fRightValue1 = fRightValue1 * fBackgroundGain;
		fLeftValue1 = CHECK_MAX_VALUE(fLeftValue1);
		fLeftValue1 = CHECK_MIN_VALUE(fLeftValue1);
		fRightValue1 = CHECK_MAX_VALUE(fRightValue1);
		fRightValue1 = CHECK_MIN_VALUE(fRightValue1);
		fLeftValue = fLeftValue1;
		fRightValue = fRightValue1;

		for (iIndex = 0; iIndex < 2; iIndex++) {

			if (iIndex == 0) {
				output = (int)(fLeftValue*f);
			}
			else {
				output = (int)(fRightValue*f);
			}
			if (output > MAX_INT)
			{
				f = (double)MAX_INT / (double)(output);
				output = MAX_INT;
			}
			if (output < MIN_INT)
			{
				f = (double)MIN_INT / (double)(output);
				output = MIN_INT;
			}
			if (f < 1)
			{
				f += ((double)1 - f) / (double)32;
			}
			sourceData1[iIndex] = (short)output;
		}
	}
	else {
		float fValue1 = 0;
		float fValue2 = 0;
		float fValue = 0;

		fValue1 = (float)(*(short*)(sourceData1));
		fValue1 = fValue1 * fBackgroundGain;
		fValue = fValue1 + fValue2;

		fValue = CHECK_MAX_VALUE(fValue);
		fValue = CHECK_MIN_VALUE(fValue);
		*sourceData1 = (short)fValue;
	}
	return 1;
}
