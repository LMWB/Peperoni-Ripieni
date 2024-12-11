import matplotlib.pyplot as plt
import numpy as np

n = 128     # number of total points in phase wheel
dc = 1000   # duty cycle of pwm

t = np.arange(0, n)
x = np.linspace(0, 2 * np.pi, n)
y = dc/2 + np.sin(x) * dc/2
y = np.round(y)

fig, ax = plt.subplots()
ax.plot(x, y)
plt.show()

print(t)
print(x)
print(y)