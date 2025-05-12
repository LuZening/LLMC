import numpy as np
import matplotlib.pyplot as plt
from scipy import signal

class Resampler:
    def __init__(self, src_rate=44100, target_rate=48000):
        self.src_rate = src_rate
        self.target_rate = target_rate
        
        # 计算最小公倍数
        self.grand_LCM = np.lcm(src_rate, target_rate)
        # 计算重采样比
        self.input_phase_step = (self.grand_LCM // self.src_rate)  # 相位分辨率
        self.output_phase_step = (self.grand_LCM // self.target_rate)
        self.LCM = self.input_phase_step * self.output_phase_step        # 44100和48000的相位最小公倍数
        self.SINC_TAPS = 62     # 增加抽头数以提高精度
        self.HISTORY_SIZE = self.SINC_TAPS -1  # 历史数据大小
        self.coeff_row_idx = 0
        # 初始化参数
        self.phase_pos = 0
        # self.phase_step = int(self.LCM / self.target_rate * self.src_rate)
        
        # 输入缓存
        self.history = np.zeros(self.HISTORY_SIZE, dtype=np.float32)
        self.hist_count = 0
        
        # 生成加窗sinc系数
        self.coeffs = self._design_kaiser_filter()
    
    def _design_kaiser_filter(self):
        """设计多相滤波器组（使用Kaiser窗）"""
        # 计算截止频率（源采样率的Nyquist频率）
        # cutoff = 21050 * 2 / self.src_rate
        cutoff  = 1
        
        # 设计参数
        beta = 8.0  # 控制旁瓣衰减
        coeffs = np.zeros((self.input_phase_step, self.SINC_TAPS), dtype=np.float32)
        
        
        # for phase in range(self.input_phase_step):
        for idx in range(self.input_phase_step):    
            phase = (idx * self.output_phase_step) % self.input_phase_step
            # 计算分数延迟 (0.0~1.0)
            frac_delay = phase / self.input_phase_step
            
            # 生成时间序列
            t = (np.arange(-self.SINC_TAPS//2 + 1, self.SINC_TAPS//2 + 1) - frac_delay)
            # * self.input_phase_step / self.output_phase_step
            self.t = t
            # 计算加窗sinc
            sinc = np.sinc(cutoff * t)
            # sinc = np.sinc(t)
            window = np.kaiser(self.SINC_TAPS, beta)
            self.window = window
            # window = np.ones(len(sinc))
            coeff = sinc * window
            
            # 归一化系数
            coeff /= np.sum(coeff)
            # coeffs[phase] = coeff
            coeffs[idx] = coeff
            
        return coeffs

    def process(self, input_signal, output_buffer):
        # 合并历史数据与新输入
        if(self.phase_pos == 0): # very first output
            self.phase_pos = self.HISTORY_SIZE * self.input_phase_step # give a initial phase
            self.history = np.zeros(self.HISTORY_SIZE)
            self.hist_count = self.HISTORY_SIZE
            buffer = np.concatenate([self.history[:self.hist_count], input_signal])
        else:
            buffer = np.concatenate([self.history[:self.hist_count], input_signal])
        total_avail = len(buffer)
        
        gen = 0
        max_gen = len(output_buffer)
        phase_pos_0 = self.phase_pos
        right = 0
        expected_out_length = (self.input_phase_step * (total_avail - self.SINC_TAPS // 2) -1 - phase_pos_0) //self.output_phase_step + 1
        while gen < max_gen:

            # 计算当前相位
            phase = (self.phase_pos % self.input_phase_step)
            
            # 计算输入位置
            input_pos = self.phase_pos // self.input_phase_step # including the left-most history 
            base = input_pos - self.SINC_TAPS//2 + 1 # 左边会多1个，如果输入输出相位重合
            
            # 检查边界
            if base < 0:
                raise "exhausted"
            elif (base + self.SINC_TAPS) > total_avail:
                break
            right = base + self.SINC_TAPS
            # 执行卷积
            window = buffer[base : right]
            coeff = self.coeffs[self.coeff_row_idx]
            output_buffer[gen] = np.dot(window, coeff) # sinc resampling
            # output_buffer[gen] = buffer[input_pos] # nearest resampling
            # output_buffer[gen] = buffer[input_pos] * (1 - phase / self.phase_steps)+ buffer[input_pos + 1] * (phase / self.phase_steps) # linear resampling
            gen += 1 
            self.phase_pos += self.output_phase_step
            self.coeff_row_idx+=1
            if(self.coeff_row_idx >= self.input_phase_step):
                self.coeff_row_idx = 0
        assert gen <= max_gen, 'output overflow'
        assert expected_out_length == gen, 'expected length unmatched'
        # 更新历史数据
        input_used = right - self.HISTORY_SIZE # also used half of SINC taps to the right
        assert input_used == len(input_signal), 'input not exhausted'
        roll_back = input_used # + N_HISTORY - N_HISTORY cancelled
        self.history = input_signal[-self.HISTORY_SIZE:]
        # rewind phase_pos,
        self.phase_pos -= roll_back * self.input_phase_step
        # remaining = len(input_signal) - input_used 
        # if remaining > 0:
        #     self.history[:min(remaining, self.HISTORY_SIZE)] = input_signal[-min(self.HISTORY_SIZE, remaining):]
        # self.hist_count = max(0, remaining)
        
        return input_used, gen

# 验证代码
SRC_RATE = 44100
TARGET_RATE = 48000
TEST_FREQ = 20000
DURATION = 1.
CHUNK_MS = 10

# 生成测试信号
t = np.linspace(0, DURATION, int(SRC_RATE*DURATION), endpoint=False)
input_signal = np.sin(2 * np.pi * TEST_FREQ * t + 0.5)

# 初始化重采样器
resampler = Resampler(SRC_RATE, TARGET_RATE)

# 输出参数表

sOut = f'float32_t coeffs[{resampler.input_phase_step}][{resampler.SINC_TAPS}]='+'{\n' 
for i in range(resampler.input_phase_step):
    for j in range(resampler.SINC_TAPS):
        s = f'{resampler.coeffs[i][j]}, '
        sOut += s
    sOut += '\n'
sOut += '\n};\n'
with open(f'sinc_{SRC_RATE}_{TARGET_RATE}.txt', 'w') as f:
    f.write(sOut)

# 流式处理
output_buffer = []
chunk_size = int(SRC_RATE * CHUNK_MS / 1000)
read_pos = 0

while read_pos < len(input_signal):
    chunk = input_signal[read_pos : read_pos+chunk_size]
    out_chunk = np.zeros(int(TARGET_RATE*CHUNK_MS/1000) , dtype=np.float32)
    
    consumed, generated = resampler.process(chunk, out_chunk)
    output_buffer.append(out_chunk[:generated])
    read_pos += consumed
    if consumed == 0:
        break

output_signal = np.concatenate(output_buffer)
output_signal = output_signal[resampler.HISTORY_SIZE + 1:int(TARGET_RATE*DURATION)]

# 频谱分析
def analyze_spectrum(signal, rate, ax):
    n = len(signal)
    freq = np.fft.rfftfreq(n, d=1/rate)
    fft = np.abs(np.fft.rfft(signal))/n
    fft /= fft.max()
    ax.semilogy(freq, fft)
    ax.set_xlim(0, rate//2)
    ax.grid()
    ax.set_title(f"{rate/1000:.1f}kHz采样率频谱")

# 绘图
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

# 原始信号频谱
analyze_spectrum(input_signal, SRC_RATE, ax1)
analyze_spectrum(output_signal, TARGET_RATE, ax2)



plt.tight_layout()
plt.show()

# 窗函数
plt.plot(resampler.t, resampler.window)
plt.show()

# 信号
N = 200
plt.plot(np.arange(0, N),input_signal[:N], label='in')
plt.plot(np.arange(0, N),[output_signal[int(i * 160 / 147)] for i in range(N)], label='out')
plt.legend()
plt.grid()
plt.show()
# 性能指标
input_energy = np.mean(input_signal**2)
output_energy = np.mean(output_signal**2)
snr = 10 * np.log10(output_energy/(input_energy - output_energy))

print(f"输入能量: {input_energy:.2e}")
print(f"输出能量: {output_energy:.2e}")
print(f"信噪比: {snr:.1f} dB")
print(f"重采样率: {len(output_signal)/DURATION/1000:.1f}k samples/s")