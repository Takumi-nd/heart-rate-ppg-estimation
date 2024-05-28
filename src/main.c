#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

int countLines(char *filePath);
void readCSVFile(char *path, int ir1[], int ir2[], int countLine);
void writeCSVFile(char *path, float out_ir1[], float out_ir2[], int countLine, char *column1, char *column2);
void writeIndexFile(char *path, int out_ir1[], int out_ir2[], int countLine, char *column1, char *column2);
void dcRemove(int signal[], int signalSize);
void filter(int signal[], float b[], float a[], float out[], int signal_size, int b_size, int a_size);
float skewness(float signal[],int startIDX, int endIDX);
void evalBeatSkew(float filteredSig[], int trough[], float skew[], int lenTrough);
void movingAvg(float signal[], float result[], int signalSize, int windowSize);
int findMax(float signal[], int startIDX, int endIDX);
int findMin(float signal[], int startIDX, int endIDX);
void getROI(float signal[], float movingAvg[], int startPos[], int endPos[], int signalSize, int *startIDX, int *endIDX);
void detectPeakTroughAdaptiveThreshold(float signal[], float movingAvg[], int peak[], int trough[], int signalSize);
void heartRate(int peak[], int numOfPeak);

int main()
{
    float b[5] = {0.03657484f, 0.0f, -0.07314967f, 0.0f, 0.03657484f};
    float a[5] = {1.0f, -3.33661174f, 4.22598625f, -2.42581874f, 0.53719462f};
    float ir1_filtered[1000] = {0.0f};
    float ir2_filtered[1000] = {0.0f};
    float mab1[1000] = {0.0f};
    float mab2[1000] = {0.0f};
    int peak1[100] = {0.0f};
    int peak2[100] = {0.0f};
    int trough1[100] = {0.0f};
    int trough2[100] = {0.0f};
    float skew1[100] = {0.0f};
    float skew2[100] = {0.0f};
    int ir1[1000];
    int ir2[1000];
    int signalSize = 1000;

    char *readPath = "C:\\Users\\Administration\\Workspace\\PPG-thesis\\Python\\record\\raw_data_7_5_2024.csv";
    char *filterPath = "C:\\Users\\Administration\\Workspace\\PPG-thesis\\Python\\c_data\\filtered.csv";
    char *maBeatPath = "C:\\Users\\Administration\\Workspace\\PPG-thesis\\Python\\c_data\\maBeat.csv";
    char *peak = "C:\\Users\\Administration\\Workspace\\PPG-thesis\\Python\\c_data\\peak.csv";
    char *trough = "C:\\Users\\Administration\\Workspace\\PPG-thesis\\Python\\c_data\\trough.csv";
    char *skew = "C:\\Users\\Administration\\Workspace\\PPG-thesis\\Python\\c_data\\skew.csv";


    //signalSize = countLines(readPath);
    readCSVFile(readPath, ir1, ir2, signalSize);

    // dc remove
    dcRemove(ir1, signalSize);
    dcRemove(ir2, signalSize);

    // filter
    filter(ir1, b, a, ir1_filtered, signalSize, 5, 5);
    filter(ir2, b, a, ir2_filtered, signalSize, 5, 5);
    writeCSVFile(filterPath, ir1_filtered, ir2_filtered, signalSize, "ir1_filtered", "ir2_filtered");
    // moving avg
    movingAvg(ir1_filtered, mab1, signalSize, 65);
    movingAvg(ir2_filtered , mab2, signalSize, 65);
    writeCSVFile(maBeatPath, mab1, mab2, signalSize, "mab_ir1", "mab_ir2");

    // find peak;
    detectPeakTroughAdaptiveThreshold(ir1_filtered, mab1, peak1, trough1, signalSize);
    detectPeakTroughAdaptiveThreshold(ir2_filtered, mab2, peak2, trough2, signalSize);
    writeIndexFile(peak, peak1, peak2, 100, "Index1", "Index2");
    writeIndexFile(trough, trough1, trough2, 100, "Index1", "Index2");

    // skewness
    evalBeatSkew(ir1_filtered, trough1, skew1, 100);
    evalBeatSkew(ir2_filtered, trough2, skew2, 100);

    writeCSVFile(skew, skew1, skew2, 100, "skew1", "skew2");

    // hr
    heartRate(peak1, 51);

    return 0;
}

int countLines(char *filePath) {
    FILE *file = fopen(filePath, "r");
    int count  = 0;
    for (char c = getc(file); c != EOF; c = getc(file)) {
        if (c == '\n')
            count++;
    }
    fclose(file);
    return count--;
}

void readCSVFile(char *path, int ir1[], int ir2[], int countLine) {
    int temp = 0;
    char header[100];
    FILE *read_csv_file;

    read_csv_file = fopen(path, "r");

    fgets(header, sizeof(header), read_csv_file);

    for (int i = 0; i < countLine; i++) {
        fscanf(read_csv_file, "%d,%d", &ir1[i], &ir2[i]);
    }

    fclose(read_csv_file);
}

void writeCSVFile(char *path, float out1[], float out2[], int countLine, char *column1, char *column2) {
    FILE *write_csv_file;
    write_csv_file = fopen(path, "w+");
    fprintf(write_csv_file, "%s,%s\n", column1, column2);
    for (int i = 0; i < countLine; i++) {
        fprintf(write_csv_file, "%f,%f\n", out1[i], out2[i]);
    }
    fclose(write_csv_file);
}

void writeIndexFile(char *path, int out1[], int out2[], int countLine, char *column1, char *column2) {
    FILE *write_csv_file;
    write_csv_file = fopen(path, "w+");
    fprintf(write_csv_file, "%s,%s\n", column1, column2);
    for (int i = 0; i < countLine; i++) {
        fprintf(write_csv_file, "%d,%d\n", out1[i], out2[i]);
    }
    fclose(write_csv_file);
}

void dcRemove(int signal[], int signalSize) {
    int sum = 0;
    for (int i = 0; i < signalSize; i++) {
        sum += signal[i];
    }
    float mean = sum / signalSize;
    printf("%f\n", mean);

    for (int i = 0; i < signalSize; i++) {
        signal[i] -= mean;
    }
}

void filter(int signal[], float b[], float a[], float out[], int signalSize, int bSize, int aSize) {
    float sum = 0;
    for (int i = 0; i < signalSize; i++) {
        sum = 0;

        for (int j = 0; j < bSize; j++) {
            if (i-j >= 0)
                sum += b[j] * signal[i-j];
        }

        for (int j = 1; j < aSize; j++) {
            if (i-j>=0)
                sum -= a[j] * out[i-j];
        }

        sum /= a[0];
        out[i] = sum;
    }
}

float skewness(float signal[],int startIDX, int endIDX) {
    int len = endIDX - startIDX + 1;
    float mean = 0.0f;
    float sum = 0.0f;
    float SD = 0.0f;
    float numerator = 0.0f;
    float skew = 0.0f;

    // mean
    for (int i = startIDX; i <= endIDX; i++) {
        sum += signal[i];
    }
    mean = sum / len;

    // sd
    for (int i = startIDX; i <= endIDX; i++) {
        SD += pow(signal[i] - mean, 2);
    }
    SD = sqrt(SD / len);

    // skewness
    for (int i = startIDX; i <= endIDX; i++) {
        numerator += pow((signal[i] - mean) / SD, 3);
    }
    skew = numerator / len;
    return skew;
}

void evalBeatSkew(float filteredSig[], int trough[], float skew[], int lenTrough) {
    for (int i = 0; i < lenTrough; i++) {
        if (trough[i] != 0 && trough[i+1] != 0) {
            skew[i] = skewness(filteredSig, trough[i], trough[i+1]);
        } else {
            return;
        }
    }
}

void movingAvg(float signal[], float result[], int signalSize, int windowSize) {
    int n = 0;
    for (int i = 0; i < signalSize; i++) {
        n = 0;
        for (n = i - (windowSize - 1)/2; n <= i + (windowSize - 1)/2; n++) {
            if (n >= 0 && n < signalSize)
                result[i] += signal[n];
        }
        result[i] /= (float)windowSize;
    }
}

int findMax(float signal[], int startIDX, int endIDX) {
    float maxValue = signal[startIDX];
    int maxIDX = 0;
    for (int i = startIDX; i <= endIDX; i++) {
        if (signal[i] >= maxValue) {
            maxIDX = i;
            maxValue = signal[i];
        }
    }
    return maxIDX;
}

int findMin(float signal[], int startIDX, int endIDX) {
    float minValue = signal[startIDX];
    int minIDX = 0;
    for (int i = startIDX; i <= endIDX; i++) {
        if (signal[i] <= minValue) {
            minIDX = i;
            minValue = signal[i];
        }
    }
    return minIDX;
}

void getROI(float signal[], float movingAvg[], int startPos[], int endPos[], int signalSize, int *startIDX, int *endIDX) {
    for (int i = 0; i < signalSize; i++) {
        if (movingAvg[i] > signal[i] && movingAvg[i+1] < signal[i+1]){
            startPos[*startIDX] = i;
            *startIDX = *startIDX + 1;
        } else if (movingAvg[i] < signal[i] && movingAvg[i+1] > signal[i+1] && *startIDX > *endIDX) {
            endPos[*endIDX] = i;
            *endIDX = *endIDX + 1;
        }
    }
    // if (*startIDX > *endIDX) {
    //     endPos[*endIDX] = signalSize;
    // }
}

void detectPeakTroughAdaptiveThreshold(float signal[], float movingAvg[], int peak[], int trough[], int signalSize) {
    int startROI[100] = {0};
    int endROI[100] = {0};
    int lenStartROI = 0;
    int lenEndROI = 0;
    int startIdx, endIdx;
    getROI(signal, movingAvg, startROI, endROI, signalSize, &lenStartROI, &lenEndROI);

    printf("%d, \n", lenStartROI);

    for (int i = 0; i < lenStartROI; i++) {
        startIdx = startROI[i];
        endIdx = endROI[i];

        peak[i] = findMax(signal, startIdx, endIdx);
        if (i >= 1) {
            trough[i-1] = findMin(signal, peak[i-1], peak[i]);
        }
    }
}

void heartRate(int peak[], int len) {
    int sumPeakInterval = 0;
    float heartRate = 0.0f;
    for (int i = 1; i < len - 1; i++) {
        sumPeakInterval += (peak[i+1] - peak[i]);
    }
    sumPeakInterval /= len;
    heartRate = (25.0f * 60.0f) / (sumPeakInterval);
    printf("%f\n", heartRate);
}
