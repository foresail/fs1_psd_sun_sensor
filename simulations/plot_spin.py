
import ltspice
import matplotlib.pyplot as plt
import numpy as np

l = ltspice.Ltspice("tia.raw")
l.parse() 


fig, ax1 = plt.subplots()
ax2 = ax1.twinx()

time = l.get_time()
I_source = l.get_data('I(I1)') * 1e6
V_out = l.get_data('V(out)')


color = 'tab:blue'
ax1.plot(time, V_out, label="Amplitude", color=color)
ax1.set_ylabel("Output (V)", color=color)
ax1.set_xlabel("Time (s)")
ax1.tick_params(axis='y', labelcolor=color)
ax1.set_ylim(0, 2)
ax1.set_xlim(0, 5)
ax1.grid()


color = 'tab:red'
ax2.plot(time, I_source, label="Phase", color=color)
ax2.set_ylabel("Source current (µA)", color=color)
ax2.tick_params(axis='y', labelcolor=color)
ax2.set_ylim(0, 200)
#ax2.grid()

plt.savefig("spin.png")
plt.show()