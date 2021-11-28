#include <arduinoFFT.h>

class Fluctuation {
public:

    bool init(unsigned X_);
    const double* next();

    unsigned GetT() { return T; };
    unsigned GetX() { return X;};
    bool SetX(unsigned x) { return init(x); }


    void printMags();
    void printVec(const double* vec, unsigned N);
private:
    arduinoFFT FFT;

    const unsigned T = 128;
    const unsigned XMAX = T;
    unsigned X = 32;
    double MAGNITUDE = X * T;

    double* mags   = NULL; // X * T
    double* reals  = NULL; // X * 2T
    double* imags  = NULL; // X * 2T
    double* values = NULL; // X * 2T

    double value[256];

    unsigned t_now = 0;

    double deviation = 2.0;

    bool realloc();

    void computeX(unsigned t);
    void computeT(unsigned t_start, unsigned x);


};
