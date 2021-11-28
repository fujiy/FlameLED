#include "Fluctuation.h"


bool Fluctuation::init(unsigned X_) {
    if (X_ < 0 || XMAX < X_ || X_ % 2 != 0) return false;
    X = X_;
    MAGNITUDE = T * X_;

    if (!realloc()) return false;

    double cX = X / 2.0 - 0.5;
    double cT = T / 2.0 - 0.5;
    for (unsigned t = 0; t < T; t++) {
        for (unsigned x = 0; x < X; x++) {
            double u = x - cX;
            double v = t - cT;
            double r = sqrt(v * v + u * u);
            double mag = MAGNITUDE / r;

            if (u < 0) u += X;
            if (v < 0) v += T;
            unsigned t_ = t >= T/2 ? t - T/2 : t + T/2;
            unsigned x_ = x >= X/2 ? x - X/2 : x + X/2;
            mags[t_*X+x_] = mag;
            // mags[t*X+x] = mag;
            // mags[t*X+x]   = t_;
            // angles[t*X+x] = x_;
        }
    }
    log_d("initialized table");

    t_now = 0;

    for (unsigned x = 0; x < X; x++) computeT(0, x);
    log_d("computed first Ts");
    for (unsigned t = 0; t < T; t++) next();
    log_d("computed first Xs");
    return true;
}

bool Fluctuation::realloc() {
    mags   = (double*) ps_realloc(mags,   sizeof(double) * T * X);
    reals  = (double*) ps_realloc(reals,  sizeof(double) * 2 * T * X);
    imags  = (double*) ps_realloc(imags,  sizeof(double) * 2 * T * X);
    values = (double*) ps_realloc(values, sizeof(double) * 2 * T * X);
    /* imags  = (double**) ps_realloc(imags, sizeof(double) * 2 * T * X); */
    log_d("%d %d %d %d", mags, reals, imags, values);
    return mags && reals && imags && values;
}

const double* Fluctuation::next() {
    unsigned t_start = ((t_now / T) + 1) % 2 * T;
    if (t_now % T < X) computeT(t_start, t_now % T);
    computeX(t_now);

    double* valuesT = values+t_now*X;
    double variance = 0;
    for (unsigned x = 0; x < X; x++) {
        valuesT[x] = constrain(valuesT[x], -deviation * 4.0, deviation * 4.0);
        variance += valuesT[x] * valuesT[x];
        valuesT[x] /= deviation;
    }
    variance /= X;

    deviation = deviation * 0.99 + sqrt(variance) * 0.01;

    // Serial.println(deviation);

    unsigned t_div = t_now / 2;
    double ratio = (double)(t_now % T) / T;
    unsigned period = t_now / T;
    switch (period) {
    case 0:
        for (unsigned x = 0; x < X; x++) {
            value[x] =
                values[t_div*X+x] * ratio +
                values[(T*3/2 + t_div)*X+x] * (1 - ratio);
        }
        break;
    case 1:
        for (unsigned x = 0; x < X; x++) {
            value[x] =
                values[t_div*X+x] * (1 - ratio) +
                values[(T/2 + t_div)*X+x] * ratio;
        }
        break;
    }

    t_now++;
    if (t_now >= 2 * T) t_now = 0;
    return value;
}

void Fluctuation::computeX(unsigned t) {
    double real[XMAX];
    double imag[XMAX];
    for (unsigned x = 0; x < X; x++) {
        real[x] = reals[t*X+x];
        imag[x] = imags[t*X+x];
    }

    FFT.Compute(real, imag, X, FFT_REVERSE);
    FFT.ComplexToMagnitude(real, imag, X);
    FFT.DCRemoval(real, X);

    for (unsigned x = 0; x < X; x++)
        values[t*X+x] = values[t*X+x] * 0.5 + real[x];
}

void Fluctuation::computeT(unsigned t_start, unsigned x) {
    double real[T];
    double imag[T];
    for (unsigned t = 0; t < T; t++) {
        double angle = random(3141592) / 500000.0;
        real[t] = mags[t*X+x] * cos(angle);
        imag[t] = mags[t*X+x] * sin(angle);
    }
    FFT.Compute(real, imag, T, FFT_REVERSE);

    for (unsigned t = 0; t < T; t++){
        reals[(t+t_start)*X+x] = real[t];
        imags[(t+t_start)*X+x] = imag[t];
    }
}

void Fluctuation::printVec(const double* vec, unsigned N) {
    for (unsigned n = 0; n < N; n++) {
        if (vec[n] >= 0) Serial.print(" ");
        Serial.print(vec[n], 4);
        Serial.print(" ");
    }
    Serial.print("\n");
}

void Fluctuation::printMags() {
    for (unsigned t = 0; t < T; t++) printVec(mags+t*X, X);
}
