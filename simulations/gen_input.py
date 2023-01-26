"""
	Script to generate "half rectified" sinewave for the Spice simulator current input
"""

import numpy as np
import math, random

f = open("current_input.txt", "w")

frequency = 1 # Hz
amplitude = 150e-6 # Amps

for t in np.linspace(0, 10, 10000):
	v = amplitude * math.sin(2 * math.pi * frequency * t)
	#v += 20e-6 * random.random()
	v = max(min(v, 1e-3), 0)
	
	f.write(f"{t} {v}\n")

f.close()
