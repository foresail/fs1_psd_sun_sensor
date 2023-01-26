"""
    Generate position to angle look-up table
"""

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import math


ATAN_RATIO = 256
LUT_SIZE = 256
ADC = 1024

RATIO = ADC // LUT_SIZE


def real(x):
    return 10 * math.degrees(math.atan2(x, ATAN_RATIO))

LUT = np.empty(LUT_SIZE)
for i in range(LUT_SIZE):
    LUT[i] = round(real(RATIO * i))


for i in range(128//8):
    print(", ".join("%4d" % x for x in LUT[8*i:8*(i+1)]) )


def lut_tan(x):
    pos = x // RATIO
    if pos >= LUT_SIZE - 1:
        return LUT[-1]
    return round(LUT[pos] + ((x - 4*pos) * (LUT[pos + 1] - LUT[pos])) // RATIO)


if 1:
    fig, ax = plt.subplots()
    ax.step(range(LUT_SIZE), LUT)

    ax.set(xlabel='ADC count', ylabel='Angle', title='Lookup table')
    ax.grid()

else:

    t = np.empty(ADC)
    err = np.empty(ADC)
    for i in range(ADC):

        t[i] = i
        err[i] = abs(real(i) - lut_tan(i))
        #print(real(i), lut_tan(i))


    fig, ax = plt.subplots()
    ax.plot(t, err)

    ax.set(xlabel='ADC count', ylabel='Error', title='Error in degrees (max %.2f, std: %.2f)' % (max(err), np.std(err)))
    ax.grid()

plt.show()
