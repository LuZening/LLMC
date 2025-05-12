import numpy as np
from scipy.signal import firwin, freqz
import matplotlib.pyplot as plt

# 设置采样频率
fs = 96000

# 截止频率
sname = 'decimator'
fc = 22050

# sname = 'interpolator'
# fc = 22050

beta = 8


# 设计 FIR 滤波器
taps = firwin(numtaps = 128, cutoff = fc, window=('kaiser', beta), pass_zero='lowpass', fs=fs)
print('even symmetric', np.all(taps == taps[::-1]))
# 绘制滤波器频率响应
w, h = freqz(taps, worN=20000)
mag = np.abs(h)
db = 20 * np.log10(mag)
plt.plot(w * fs / (2 * np.pi), db)
plt.grid(True)
plt.xlabel('Frequency (Hz)')
plt.ylabel('Magnitude (dB)')
plt.title('FIR Filter Frequency Response')
plt.show()

# 输出C参数
sOut = f'float32_t coeffs[{len(taps)}]='+'{\n' 
for i in range(len(taps)):
    s = f'{taps[i]}, '
    sOut += s
sOut += '\n};\n'
with open(f'fir_{sname}.txt', 'w') as f:
    f.write(sOut)
print(sOut)
pass