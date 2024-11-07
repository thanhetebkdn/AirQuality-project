#define SharpGP2Y10_SAMPLINGTIME 280
#define SharpGP2Y10_DELTATIME 40
#define SharpGP2Y10_SLEEPINGTIME 9680

class SharpGP2Y10
{
private:
  int voPin;
  int ledPin;
  float dustDensity = 0;
  float volMeasured = 0;
  float calcVoltage = 0;
  float vccVol = 5.0;
  void calc();

public:
  float getDustDensityField();
  float getDustDensity();

  float getVotageField();
  float getVotage();

  int getADCField();
  int getADC();

  SharpGP2Y10(int, int);
  SharpGP2Y10(int, int, float);
};