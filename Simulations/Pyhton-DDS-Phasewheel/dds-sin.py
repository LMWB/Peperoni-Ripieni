import matplotlib.pyplot as plt
import numpy as np

n = 9     
N = 2**n    # number of total points in phase wheel
dc = 1000   # duty cycle of pwm

t = np.arange(0, N)
x = np.linspace(0, 2 * np.pi, N)
y = dc/2 + np.sin(x) * dc/2
y = np.int32(y)

fig, ax = plt.subplots()
ax.plot(x, y)
#plt.show()

outputstring = 'static uint32_t DDS_LUT[N_LUT] = {\n'
linebreak = 0;
for i in y:
    outputstring += (str(i)+ ', ')
    linebreak += 1
    if(linebreak == 20):
        outputstring += '\n'
        linebreak = 0
outputstring += '\n};'

file = open("lut.txt", "w")
file.write(outputstring)
file.close()

#print(t)
#print(x)
#print(y)