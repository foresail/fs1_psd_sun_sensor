
import ltspice
import matplotlib.pyplot as plt
import numpy as np

l = ltspice.Ltspice("tia.raw")
l.parse() 


fig, ax1 = plt.subplots()
ax2 = ax1.twinx()

freq = l.get_frequency()
V_out = l.get_data('V(out)')

Vout_amplitude = 20 * np.log10(np.abs(V_out))
Vout_angle = np.angle(V_out, deg=True)

color = 'tab:blue'
ax1.semilogx(freq, Vout_amplitude - Vout_amplitude[0], label="Amplitude", color=color)
ax1.set_ylabel("Amplitude (dB)", color=color)
ax1.set_xlabel("Frequency (Hz)")
ax1.tick_params(axis='y', labelcolor=color)
ax1.set_ylim(-10, 1)
ax1.set_xlim(0.1, 100)
ax1.grid()


color = 'tab:red'
ax2.semilogx(freq, Vout_angle, label="Phase", color=color)
ax2.set_ylabel("Phase (Deg)", color=color)
ax2.tick_params(axis='y', labelcolor=color)
ax2.set_ylim(90, 200)
#ax2.grid()

plt.savefig("ac.png")
plt.show()