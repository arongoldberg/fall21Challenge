#include <mbed.h>

//Look for max and min(max negative) and that is one stride
//Take a lot of samples and do a moving average filter and/or low pass filter, eg of 10 samples and make sure to wait for sensor to be ready - statusreg
//One sample is the average of 10-20 samples. Perform calculations after taking the average
//Conversion using range(250 dps) is necessary to get dps
//look up libraries for 429 for lcd screen video or use usbserial to output
//Use average velo of every 1 or 2 seconds to find distance?
//Valid approach is to just find time between strides and use a given stride length to find distance
//Set block data update?

#define NOISE_SAMPLE_SIZE 100
#define NUM_SAMPLES 1000

SPI spi(PF_9, PF_8, PF_7); // mosi, miso, sclk
DigitalOut cs(PC_1);
DigitalOut led(LED1);
DigitalOut led2(LED2);

// Documents
// Manual for dev board: https://www.st.com/resource/en/user_manual/um1670-discovery-kit-with-stm32f429zi-mcu-stmicroelectronics.pdf
// gyroscope datasheet: https://www.mouser.com/datasheet/2/389/dm00168691-1798633.pdf


void setupGyro(){
  // Chip must be deselected
  cs = 1;
 
  // Setup the spi for 8 bit data, high steady state clock,
  // second edge capture, with a 1MHz clock rate
  spi.format(8,3);
  spi.frequency(1000000);
  
  // Select chip
  cs = 0;
  
  // Write to control register 1
  spi.write(0x20);

  // Mode = normal, enable x,y,z axes, default output data rate and bandwidth
  spi.write(0x0f);
  
  cs = 1;
}


void readAxes(uint16_t index, int x_low[], int x_hi[], int y_low[], int y_hi[], int z_low[], int z_hi[]){
  cs = 0;
  spi.write(0xa8);
  x_low[index] = spi.write(0x00);
  cs = 1;
  wait_us(10);
  cs = 0;
  spi.write(0xa9);
  x_hi[index] = spi.write(0x00);
  cs = 1;
  wait_us(10);
  cs = 0;
  spi.write(0xaa);
  y_low[index] = spi.write(0x00);
  cs = 1;
  wait_us(10);
  cs = 0;
  spi.write(0xab);
  y_hi[index] = spi.write(0x00);
  cs = 1;
  wait_us(10);
  cs = 0;
  spi.write(0xac);
  z_low[index] = spi.write(0x00);
  cs = 1;
  wait_us(10);
  cs = 0;
  spi.write(0xad);
  z_hi[index] = spi.write(0x00);
  cs = 1;
}

// Read a register on the gyroscope
int main() {

  //setupGyro();
  // Chip must be deselected
  cs = 1;
  led = 1;
  led2 = 1;
 
  // Setup the spi for 8 bit data, high steady state clock,
  // second edge capture, with a 1MHz clock rate
  spi.format(8,3);
  spi.frequency(1000000);
  led = 1;
  // Select chip
  cs = 0;
  
  // Write to control register 1
  spi.write(0x20);

  // Mode = normal, enable x,y,z axes, default output data rate and bandwidth
  spi.write(0x0f);
  
  cs = 1;


  int x_noise_low[NOISE_SAMPLE_SIZE];
  int x_noise_hi[NOISE_SAMPLE_SIZE];
  int y_noise_low[NOISE_SAMPLE_SIZE];
  int y_noise_hi[NOISE_SAMPLE_SIZE];
  int z_noise_low[NOISE_SAMPLE_SIZE];
  int z_noise_hi[NOISE_SAMPLE_SIZE];

  int x_low[NUM_SAMPLES];
  int x_hi[NUM_SAMPLES];
  int y_low[NUM_SAMPLES];
  int y_hi[NUM_SAMPLES];
  int z_low[NUM_SAMPLES];
  int z_hi[NUM_SAMPLES];

  while(1){

    int index = 0;

    while(index < NOISE_SAMPLE_SIZE){
      readAxes(index, x_noise_low, x_noise_hi, y_noise_low, y_noise_hi, z_noise_low, z_noise_hi);
      index++;
      wait_us(15000);
    }
    

    int16_t xNoise[NOISE_SAMPLE_SIZE];
    int16_t yNoise[NOISE_SAMPLE_SIZE];
    int16_t zNoise[NOISE_SAMPLE_SIZE];

    int16_t xAvgNoise = 0;
    int16_t yAvgNoise = 0;
    int16_t zAvgNoise = 0;

    for(int i = 0; i < NOISE_SAMPLE_SIZE; i++){
      xNoise[i] = (x_noise_hi[i] << 8) + x_noise_low[i];
      xAvgNoise += xNoise[i];
      yNoise[i] = (y_noise_hi[i] << 8) + y_noise_low[i];
      yAvgNoise += yNoise[i];
      zNoise[i] = (z_noise_hi[i] << 8) + z_noise_low[i];
      zAvgNoise += zNoise[i];
    }

    xAvgNoise = xAvgNoise/100;
    yAvgNoise = yAvgNoise/100;
    zAvgNoise = zAvgNoise/100;

    int16_t xValsRaw[NUM_SAMPLES];
    int16_t yValsRaw[NUM_SAMPLES];
    int16_t zValsRaw[NUM_SAMPLES];

    led = 1;

    for(int i = 0; i < NUM_SAMPLES; i++){
      readAxes(i, x_low, x_hi, y_low, y_hi, z_low, z_hi);
      wait_us(15000);
    }

    led = 0;

    for(int i = 0; i < NUM_SAMPLES; i++){
      xValsRaw[i] = (x_hi[i] << 8) + x_low[i];
      xValsRaw[i] = xValsRaw[i] - xAvgNoise;
      xValsRaw[i] = xValsRaw[i]*0.00875;
      yValsRaw[i] = (y_hi[i] << 8) + y_low[i];
      yValsRaw[i] = yValsRaw[i] - yAvgNoise;
      yValsRaw[i] = yValsRaw[i]*0.00875;
      zValsRaw[i] = (z_hi[i] << 8) + z_low[i];
      zValsRaw[i] = zValsRaw[i] - zAvgNoise;
      zValsRaw[i] = zValsRaw[i]*0.00875;
    }

    int16_t xVals[NUM_SAMPLES/10];
    int16_t yVals[NUM_SAMPLES/10];
    int16_t zVals[NUM_SAMPLES/10];

    int i = 0;

    while(i < NUM_SAMPLES){
      int16_t xVal = 0;
      for(int j = 0; j < 9; j++){
        xVal += xVals[i+j];
      }
      xVal = xVal/10;
      xVals[i] = xVal; 

      int16_t yVal = 0;
      for(int j = 0; j < 9; j++){
        yVal += yVals[i+j];
      }
      yVal = yVal/10;
      yVals[i] = yVal;

      int16_t zVal = 0;
      for(int j = 0; j < 9; j++){
        zVal += zVals[i+j];
      }
      zVal = zVal/10;
      zVals[i] = zVal;

      i = i + 10;
    }


    printf("x values \n\r[");
    for(int i = 0; i < NUM_SAMPLES; i++){
      printf("%d, ", xValsRaw[i]);
    }
    printf("]\n\rxaverage = %d\n\r", xAvgNoise);

    printf("y values \n\r[");
    for(int i = 0; i < NUM_SAMPLES; i++){
      printf("%d, ", yValsRaw[i]);
    }
    printf("]\n\ryaverage = %d\n\r", yAvgNoise);

    printf("z values \n\r[");
    for(int i = 0; i < NUM_SAMPLES; i++){
      printf("%d, ", zValsRaw[i]);
    }
    printf("]\n\rzaverage = %d\n\r", zAvgNoise);
    
    cs = 1;


  }
  
  // Deselect the device
  cs = 1;
}