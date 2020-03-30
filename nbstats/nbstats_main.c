/*!	\file		nbstats_main.c
	\author		Jimin Park
	\date		2019-02-18
	\version	0.1

	This  C console application that compiles a set of statistics on a list of numbers, then performs a NewcombBenford (abbr. NB) analysis of the data set.
	- range [minimum value ... maximum value]
	- arithmetic mean; • statistical median value
	- variance (of a discrete random variable)
	- standard deviation (of a finite population)
	- mode (including multi-modal lists)
	- frequency table
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <stdbool.h>

HANDLE hStdin;
DWORD fdwSaveOldMode;

// struct declaration
struct output {
	bool existData;				// terminate null or have data in array 
	bool placeChk;				// table chart places check - if frequency is 100% , the value is true
	bool exceed50;				// exceed scale 50%
	size_t arrSize;
	int stdOrFile;				// terminate get datas from stdin(1) or file(2)
	long double arithmeticMean;
	long double statisticalMedian;
	long double variance;
	long double standardDeviation;
	long double rangeMin;
	long double rangeMax;
	long int modeFH;
	long int numModes;
	long double *modeNums;
	long double NBVariance;
	long double NBDeviation;
	long int fre_array[9];		// Raw frequency table
	double expected_array[9];	// Expected frequencies
	double actual_array[9];		// Actual frequencies
};
struct output data = { 0 };

long double* getNumbers(int argc, char* argv[]);
int sortNumbers(long double a[], size_t size);
int compare_num(void const* pA, void const* pB);
void calculateArraySize(long double p[]);
void calculateRange(long double a[], size_t size);
void calArithmeticMean(long double a[], size_t size);
void calStatisticalMedian(long double a[], size_t size);
void calVariance(long double a[], size_t size, long double mean);
void calStandardDeviation(long double variance);
int calMode(long double a[], size_t size);
void frequencyTable(long double a[], size_t size);
void calNB(double P[], double A[]);
void printOutput();

int main(int argc, char* argv[]) {
	// 1. print program info 
	printOutput(data);

	// 2. get numbers from file or console
	long double* numsArray = getNumbers(argc, argv);
	if (numsArray == NULL) { // if didn't get any number, program will terminate
		return EXIT_FAILURE;
	}
	else { // Succeed in getting numbers
		data.existData = true;
	}

	// 3. calculate size that how many numbers we got
	calculateArraySize(numsArray);
	// 4. sort numbers
	sortNumbers(numsArray, data.arrSize);
	// 5. calculate Arithmetic Mean
	calArithmeticMean(numsArray, data.arrSize);
	// 6. calculate Statistical Median
	calStatisticalMedian(numsArray, data.arrSize);
	// 7. calculate Variance
	calVariance(numsArray, data.arrSize, data.arithmeticMean);
	// 8. calculate Standard Deviation
	calStandardDeviation(data.variance);
	// 9. calculate range
	calculateRange(numsArray, data.arrSize);
	// 10. calculate mode
	calMode(numsArray, data.arrSize);
	// 11. calculate  raw frequency, expected frequencies , actual frequencies 
	frequencyTable(numsArray, data.arrSize);
	// 12. calculate NB Deviation
	calNB(data.expected_array, data.actual_array);
	// 13. print all the statistics on a list of numbers and table/graph
	printOutput(data);

	free(numsArray);
	free(data.modeNums);

	return 0;
}

/*!	 \fn long double* getNumbers
	 \return numbers
	 \param int argc, char* argv[]

	 if argc is 1 get numbers from console, if argc is 2 get numbers from file
	 If get any character(non-digits), program will terminated.				*/
long double* getNumbers(int argc, char* argv[]) {
	FILE* stream;
	errno_t err;
	bool chInNum = false; // determin the number include non-digit
	int ch = 0;
	size_t capacity_c = 4; // character
	size_t capacity_l = 4; // numbers
	size_t size = 0;
	size_t sizeCh = 0;
	char* chars = (char*)malloc(capacity_c + 1);
	long double* numbers = malloc((sizeof(long double) *capacity_l));

	if (argc > 2) { // error
		printf("Error: too many command-line arguments (%d)\n", argc);
		printf(
			"Error: invalid command line.\n"
			"Usage: nbstats [filename]\n"
		);
		exit(EXIT_FAILURE);
	}
	else if (argc == 1) { //stdin
		stream = stdin;
		data.stdOrFile = 1;
	}
	else if (argc == 2) { // file
		data.stdOrFile = 2;
		// Open the file in binary read mode.
		if ((err = fopen_s(&stream, argv[1], "rb")) != 0) {
			printf("error <%s> ", argv[1]);
			perror(" ");
			exit(EXIT_FAILURE);
		}
	}

	while ((ch = fgetc(stream)) != EOF) {
		if (ch == '-') { // reject the negative number
			chars[sizeCh++] = ch;
			ch = fgetc(stream);

			if (isdigit(ch) || ch == '.') {  // reject the negative number
				while (isdigit(ch) || ch == '.') {
					if (sizeCh == capacity_c) {
						char* charsDouble = (char*)realloc(chars, (capacity_c *= 2) + 1);
						if (charsDouble == NULL) {
							free(chars);
							return NULL;
						}
						chars = charsDouble;
					}
					chars[sizeCh++] = ch;
					ch = fgetc(stream);
				}
			}
			else {  // terminate when input special characters
				while (!isspace(ch)) { // characters
					if (sizeCh == capacity_c) {
						char* charsDouble = (char*)realloc(chars, (capacity_c *= 2) + 1);
						if (charsDouble == NULL) {
							free(chars);
							return NULL;
						}
						chars = charsDouble;
					}
					chars[sizeCh++] = ch;
					ch = fgetc(stream);
				}

				chars[sizeCh] = '\0';
				printf("Error: failure reading element %u \n", size);
				printf("\tLength = %u \n", sizeCh);
				printf("\tValue = \"%s\" \n", chars);
				exit(EXIT_FAILURE);
			}
			chars[sizeCh] = '\0';
			printf("Error: rejected #%u <%s>\n", size, chars);

			// initialize chars array
			sizeCh = 0;
			chars[sizeCh] = '\0';
		} // negativer number or character
		else if (ch == '0') { // reject 0 BUT accept 0.00 float number
			chars[sizeCh++] = ch;
			ch = fgetc(stream);

			while (ch != EOF && !isspace(ch)) {
				if (sizeCh == capacity_c) {
					char* charsDouble = (char*)realloc(chars, (capacity_c *= 2) + 1);
					if (charsDouble == NULL) {
						free(chars);
						return NULL;
					}
					chars = charsDouble;
				}
				chars[sizeCh++] = ch;
				ch = fgetc(stream);
			}
			// end of array
			chars[sizeCh] = '\0';

			if ((size + 1) == capacity_l) {
				long double* numbersDouble = (long double*)realloc(numbers, (sizeof(long double) * (capacity_l *= 2)));
				if (numbersDouble == NULL) {
					free(numbers);
					return NULL;
				}
				numbers = numbersDouble;
			}

			if (atof(chars) > 0)
				sscanf_s(chars, "%Lf", &numbers[size++]);
			else
				printf("Error: rejected #%u <%s>\n", size, chars);

			sizeCh = 0;
			chars[sizeCh] = '\0';
		}
		else if (isdigit(ch) == 0 && !isspace(ch)) { // ALPHBET or special characters
			while (ch != EOF && !isspace(ch)) {
				if (sizeCh == capacity_c) {
					char* charsDouble = (char*)realloc(chars, (capacity_c *= 2) + 1);
					if (charsDouble == NULL) {
						free(chars);
						return NULL;
					}
					chars = charsDouble;
				}
				chars[sizeCh++] = ch;
				ch = fgetc(stream);
			}
			chars[sizeCh] = '\0';

			if ((size + 1) == capacity_l) {
				long double* numbersDouble = (long double*)realloc(numbers, (sizeof(long double) * (capacity_l *= 2)));
				if (numbersDouble == NULL) {
					free(numbers);
					return NULL;
				}
				numbers = numbersDouble;
			}

			if (atof(chars) > 0) { // float number
				sscanf_s(chars, "%Lf", &numbers[size++]);
				sizeCh = 0;
				chars[sizeCh] = '\0';
			}
			else { // error terminate
				chars[sizeCh] = '\0';
				printf("Error: failure reading element %u \n", size);
				printf("\tLength = %u \n", sizeCh);
				printf("\tValue = \"%s\" \n", chars);
				exit(EXIT_FAILURE);
			}

		}  // if ALPHBET
		else if (isdigit(ch)) { // numbers
			while (isdigit(ch) || ch == '.' || isalpha(ch)) { // int and float number
				if (sizeCh == capacity_c) {
					char* charsDouble = (char*)realloc(chars, (capacity_c *= 2) + 1);
					if (charsDouble == NULL) {
						free(chars);
						return NULL;
					}
					chars = charsDouble;
				}
				if ((isdigit(ch) || ch == '.') && chInNum == false) {
					chars[sizeCh++] = ch;
				}
				else {
					chInNum = true;
				}

				ch = fgetc(stream);
			}//while

			if (isspace(ch))
				ungetc(ch, stream);

			if ((size + 1) == capacity_l) {
				long double* numbersDouble = (long double*)realloc(numbers, (sizeof(long double) * (capacity_l *= 2)));
				if (numbersDouble == NULL) {
					free(numbers);
					return NULL;
				}
				numbers = numbersDouble;
			}

			chars[sizeCh] = '\0';

			if (sizeCh > 0) {
				long double num = 0;
				sscanf_s(chars, "%Lf", &num);

				if (isinf(num)) { // inf = INFINITY skip
					numbers[size] = 0;
					printf("Error: rejected # %u <%s> = INFINITY\n", size++, chars);
				}
				else {
					numbers[size++] = num;
				}

				sizeCh = 0;
				capacity_c = 4;
				chars[sizeCh] = '\0';
			}
		} // numbers
		else {
			// Initialize
			chInNum = false;
		}
	}

	if (ch == EOF && size == 0) { // If didn'y get any numbers
		printf("Data set is empty! \n");
		exit(EXIT_FAILURE);
	}

	fclose(stream);
	free(chars);
	numbers[size] = 0; // end of array
	return numbers;
}

/*!	 \fn sortNumbers
	 \return 0
	 \param long double a[] - numbers array, size_t size - how many numbers in array

	 Sort numbers array what get from file or console using a qsort */
int sortNumbers(long double a[], size_t size) {

	//built-in sort
	qsort(a, size, sizeof(long double), compare_num);

	return 0;
}

/*!	 \fn compare_num
	 \return
	 \param void const* pA, void const* pB

	 This is the function that compares two elements. */
int compare_num(void const* pA, void const* pB) {
	//Gernal approach
	long double a = *(long double const*)pA;
	long double b = *(long double const*)pB;

	if (a > b)
		return 1;
	else if (a < b)
		return -1;
	else
		return 0;

}

/*!	 \fn calculateArraySize
	 \return none
	 \param long double a[] - numbers array

	 Calculate how many elements in numbers array */
void calculateArraySize(long double a[]) {
	size_t i = 0;
	while (a[i] != 0) { // 0 is end of array in numbers array
		i++;
	}
	//assign size of array to struct's variable arrSize
	data.arrSize = i;
}

/*!	 \fn calculateRange
	 \return none
	 \param long double a[] - numbers array, size_t size - how many numbers in array

	 Determin min value and max value of numbers array(a[]) */
void calculateRange(long double a[], size_t size) {
	data.rangeMin = a[0];
	data.rangeMax = a[size - 1];
}

/*!	 \fn calArithmeticMean
	 \return none
	 \param long double a[] - numbers array, size_t size - how many numbers in array

	 Calculate Arithmetic Mean
	 \note The sum of all the values divided by the number of values. */
void calArithmeticMean(long double a[], size_t size) {
	long double sum = 0;
	for (size_t i = 0; i < size; i++) {
		sum += a[i];
	}
	data.arithmeticMean = sum / size;
}

/*!	 \fn calStatisticalMedian
	 \return none
	 \param long double a[] - numbers array, size_t size - how many numbers in array

	 Calculate Statistical Median
	 \note The middle value of a sorted data set of odd length,
		   or the arithmetic mean of the two closest values to the middle of a sorted data set of even length.  */
void calStatisticalMedian(long double a[], size_t size) {
	long int index1 = 0;
	long int index2 = 0;

	if (size % 2 == 0) { //even
		index1 = size / 2 - 1;
		index2 = size / 2 + 1 - 1;

		data.statisticalMedian = (a[index1] + a[index2]) / 2;
	}
	else { //odd
		index1 = size / 2 + 1 - 1;

		data.statisticalMedian = a[index1];
	}
}

/*!	 \fn calVariance
	 \return none
	 \param long double a[] - numbers array, size_t size - how many numbers in array, long double mean - arithmetic mean

	 Calculate Variance
	 \note The mean of the squared differences of each sample from the arithmetic mean. */
void calVariance(long double a[], size_t size, long double mean) {
	long double variance = 0;
	for (size_t i = 0; i < size; i++) {
		variance += pow(a[i] - mean, 2);
	}

	data.variance = variance / size;
}
// 
/*!	 \fn calStandardDeviation
	 \return none
	 \param long double variance

	 Calculate Variance
	 \note The square root of the variance.*/
void calStandardDeviation(long double variance) {
	data.standardDeviation = sqrt(variance);
}

/*!	 \fn calMode
	 \return 0
	 \param long double a[] - numbers array, size_t size - how many numbers in array

	 Calculate Mode or Multi Mode*/
int calMode(long double a[], size_t size) {
	size_t capacity = 4;
	size_t i = 0;
	long int a_index = -1;
	long int modeF = 0; // frequency of num

	data.modeNums = malloc((sizeof(long double) *capacity + 1));
	data.modeFH = 0;
	data.numModes = 0;

	while (i < size) {
		for (i; i < size; i++) {
			if (a[i] == a[i + 1]) {
				modeF++;
			}
			else {
				if (modeF > data.modeFH) { // get mode
					data.modeNums[0] = 0;
					data.modeNums[0] = a[i];
					data.modeFH = modeF;

					// initicalize 
					a_index = 0;
					data.numModes = 1;

					i++;
					modeF = 0;
					break;

				}
				else if ((modeF == data.modeFH) && modeF != 0) { // Multi mode stored
					if (a_index == capacity) {
						long double* modeNumsDouble = (long double*)realloc(data.modeNums, (sizeof(long double) *(capacity *= 2)));
						if (modeNumsDouble == NULL) {
							free(data.modeNums);
							return 1;
						}
						data.modeNums = modeNumsDouble;
					}

					data.modeNums[++a_index] = a[i];
					data.numModes++;
					i++;
					modeF = 0;
					break;
				}
				else { // not mode num
					i++;
					modeF = 0;
					break;
				}
			}
		}
	}

	return 0;
}

/*!	 \fn frequencyTable
	 \return none
	 \param long double a[] - numbers array, size_t size - how many numbers in array

	 Calculate raw frequency, expected frequencies , actual frequencies */
void frequencyTable(long double a[], size_t size) {
	long double digit = 0;

	// calculate Raw frequency
	for (size_t i = 0; i < size; i++) {
		if (a[i] < 1) { // float number < 1
			char str[20];
			snprintf(str, sizeof(str), "%e", a[i]);

			int num = 0;
			sscanf_s(str, "%d", &num);
			data.fre_array[num - 1] += 1;
		}
		else if (a[i] < 10) { // if num is between 0 and 9
			digit = a[i];
			data.fre_array[(int)digit - 1] += 1; // index 0 base
		}
		else { // num > 10
			digit = a[i] / 10;

			if (digit == 0) {
				digit = a[i];
				data.fre_array[(int)digit - 1] += 1;
			}
			else {
				while (digit >= 10) {
					digit = digit / 10;
				}
				data.fre_array[(int)digit - 1] += 1;
			}
		}
	}

	// calculate Expected frequencies
	for (size_t i = 0; i < 9; i++) {
		double expectNum = log10((i + 1) + 1) - log10(i + 1);
		expectNum = expectNum * 100;

		data.expected_array[i] = expectNum;
	}

	// calculte Actual frequencies
	for (size_t i = 0; i < 9; i++) {
		if ((double)data.fre_array[i] / (double)size * 100 > 99) {
			data.placeChk = true; // Actual frequencie is 100
			data.exceed50 = true; // exceed 50%
		}
		else if ((double)data.fre_array[i] / (double)size * 100 >= 50) {
			data.exceed50 = true; // exceed 50%
			data.placeChk = false; // not over 99%
		}
		data.actual_array[i] = (double)data.fre_array[i] / (double)size * 100;
	}
}

/*!	 \fn calNB
	 \return none
	 \param double P[] - expected frequencies array, double A[] - actual frequencies array

	 Calculate NB Variance , Deviation */
void calNB(double P[], double A[]) {

	// calculate NB Variance
	for (int i = 0; i < 9; i++) {
		data.NBVariance += pow((A[i] / P[i] - 1), 2);
	}
	data.NBVariance = data.NBVariance / 9;

	// calculte NB Deviation
	data.NBDeviation = sqrt(data.NBVariance);
}

/*!	 \fn printOutput
	 \return none
	 \param struct output data

	 Print all the statistics on a list of numbers and table/graph */
void printOutput() {
	// for Command line variations. Code_page
	int i = 0;
	int xPrint = 0;

	if (data.exceed50 == false || data.placeChk == false)
		xPrint = 62; // not exceed 50% 
	else
		xPrint = 63; // exceed 50% 

	UINT curCP = GetConsoleOutputCP();

	if (!SetConsoleOutputCP(850)) {
		puts("SetConsoleOutputCP");
		EXIT_FAILURE;
	}

	if (data.existData == false) { // first comment
		printf("Newcomb-Benford Stats (v1.0.0), \xB8""2019 Jimin Park\n");
		printf("================================================\n");
		if (data.stdOrFile == 1)
			printf("Enter white-space separated real numbers. Terminate input with ^Z\n");
	}
	else { // after statistic
		printf("\nStandard Analysis\n");
		while (i < xPrint) {
			printf("\xcd");
			i++;
		}
		printf("\n");
		i = 0;

		printf("# elements = %u\n", data.arrSize);
		printf("Range = [%.6g .. %.6g]\n", data.rangeMin, data.rangeMax);
		printf("Arithmetic mean = %.6g\n", data.arithmeticMean);
		printf("Arithmetic median = %.6g\n", data.statisticalMedian);
		printf("Variance = %.6g\n", data.variance);
		printf("Standard Deviation = %.6g\n", data.standardDeviation);

		if (data.numModes == 0 || (data.numModes*(data.modeFH + 1) == data.arrSize)) { //no mode
			printf("Mode = no mode \n");
		}
		else {
			printf("Mode = { ");
			for (int i = 0; i < data.numModes; i++) {
				if (i == 0)
					printf("%.6g ", data.modeNums[i]);
				else
					printf(", %.6g ", data.modeNums[i]);
			}

			printf("}\x9e%u\n\n", data.modeFH + 1);
		}

		// Print Raw Frequency
		for (int i = 0; i < 9; i++) {
			printf(" [%u] = %ld\n", i + 1, data.fre_array[i]);
		}
		printf("\n\n");

		printf("Newcomb-Benford's Law Analysis\n");
		while (i < xPrint) {
			printf("\xcd");
			i++;
		}
		printf("\n");
		i = 0;

		if (data.exceed50 == false) {  // scale 0-50
			printf("    exp dig    freq  0      10      20      30      40      50\n");
			printf("\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4 \xda\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\n");
		}
		else if (data.placeChk == false) {
			printf("    exp dig    freq  0  10  20  30  40  50  60  70  80  90 100\n");
			printf("\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4 \xda\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\n");
		}
		else {
			printf("    exp dig    freq   0  10  20  30  40  50  60  70  80  90 100\n");
			printf("\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4 \xda\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\xc4\xc4\xc4\xc2\n");
		}


		for (int i = 0; i < 9; i++) {
			// printf Expected frequencies 
			if (i < 3)
				printf(" %.2lf%% [%1d] =", data.expected_array[i], i + 1);
			else
				printf("  %.2lf%% [%1d] =", data.expected_array[i], i + 1);

			// printf Actual frequencies
			// Determine space for Command line variations
			if (data.placeChk == true && data.actual_array[i] > 99) { // 100%
				printf(" %.2lf%% \xb3", data.actual_array[i]);
			}
			else if (data.placeChk == true) {
				printf("   %.2lf%% \xb3", data.actual_array[i]);
			}
			else if (data.actual_array[i] >= 10) {
				printf(" %.2lf%% \xb3", data.actual_array[i]);
			}
			else {
				printf("  %.2lf%% \xb3", data.actual_array[i]);
			}

			int ractangle = 0;
			if (data.exceed50 == true)
				ractangle = (int)(data.actual_array[i] / 2.5); // exceed 50%
			else
				ractangle = (int)(data.actual_array[i] / 1.25); // 0~50%
			for (int r = 0; r < ractangle; r++) {
				printf("\xfe");
			}
			printf("\n");
		}

		if (data.exceed50 == false) {  // scale 0-50
			printf("\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4 \xc0\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\n");
		}
		else if (data.placeChk == false) {
			printf("\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4 \xc0\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\n");
		}
		else {
			printf("\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4 \xc0\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\xc4\xc4\xc4\xc1\n");
		}

		printf("Variance = %.5Lf%%\n", data.NBVariance * 100);
		printf("Std. Dev. = %.5Lf%%\n", data.NBDeviation * 100);

		// NB relationship analysis
		if (0 <= data.NBDeviation && data.NBDeviation < 0.1)
			printf("There is a very strong Benford relationship.\n");
		else if (0.1 <= data.NBDeviation && data.NBDeviation < 0.2)
			printf("There is a strong Benford relationship.\n");
		else if (0.2 <= data.NBDeviation && data.NBDeviation < 0.35)
			printf("There is a moderate Benford relationship.\n");
		else if (0.35 <= data.NBDeviation && data.NBDeviation < 0.5)
			printf("There is a weak Benford relationship.\n");
		else if (0.5 <= data.NBDeviation)
			printf("There is not a Benford relationship.\n");

		while (i < xPrint) {
			printf("\xcd");
			i++;
		}
		printf("\n");

	}

	if (!SetConsoleOutputCP(curCP)) {
		puts("SetConsoleOutputCP");
		EXIT_FAILURE;
	}
}