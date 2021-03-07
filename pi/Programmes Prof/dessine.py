import numpy as np
import matplotlib.pyplot as plt

N=16384

x = np.linspace(0,N/1000,N)
y = np.loadtxt('MESURE.txt')
plt.plot(x,y,"-")
plt.title("Signal mesuré")
plt.xlabel("Temps (s)")
plt.ylabel("y(t)")
plt.grid(True)

plt.show()

x = np.linspace(0,1000,N)
y = np.loadtxt('TEST.txt')
plt.plot(x,y,"-")
plt.title("FFT du signal")
plt.xlabel("Frequence (Hz)")
plt.ylabel("|amplitude|")
plt.grid(True)

plt.show()