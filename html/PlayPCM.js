//（初始化）播放器对象
export function PCMPlayer(option) {
	this.init(option);
}
//初始化（params）
PCMPlayer.prototype.init = function(option) {
	let defaults = {
		encoding: '16bitInt', //编码格式
		channels: 1, //声道数
		sampleRate: 8000, //采样率
		flushingTime: 50, //刷新时间
		volume: 0 //音量
	};
	this.option = Object.assign({}, defaults, option); //option配置
	this.samples = new Float32Array(); //采样
	this.flush = this.flush.bind(this); //刷新
	this.interval = setInterval(this.flush, this.option.flushingTime); //间隔（刷新事件）
	this.maxValue = this.getMaxValue(); //最大编码值 ，例如：128
	this.typedArray = this.getTypedArray(); //编码类型 ，例如：Int8Array
	this.createContext(this.option);
};
//获取最大编码 （根据配置的编码格式返回最大编码值，用于音频数据归一化处理）
PCMPlayer.prototype.getMaxValue = function() {
	let encodings = {
		'8bitInt': 128,
		'16bitInt': 32768,
		'32bitInt': 2147483648,
		'32bitFloat': 1
	};

	return encodings[this.option.encoding] ? encodings[this.option.encoding] : encodings['16bitInt'];
};
//获取二进制数组类型
PCMPlayer.prototype.getTypedArray = function() {
	let typedArrays = {
		'8bitInt': Int8Array,
		'16bitInt': Int16Array,
		'32bitInt': Int32Array,
		'32bitFloat': Float32Array
	};

	return typedArrays[this.option.encoding] ? typedArrays[this.option.encoding] : typedArrays['16bitInt'];
};
//创建context（上下文）
PCMPlayer.prototype.createContext = function(option) {
	this.audioCtx = new(window.AudioContext || window.webkitAudioContext)();
	this.gainNode = this.audioCtx.createGain();
	this.gainNode.gain.value = option.volume || 0;
	this.gainNode.connect(this.audioCtx.destination);
	this.startTime = this.audioCtx.currentTime; //时间基准
};
//判断类型数据
PCMPlayer.prototype.isTypedArray = function(data) {
	return (data.byteLength && data.buffer && data.buffer.constructor == ArrayBuffer);
};
//采样 (用于将外部的 PCM 数据添加到 samples 中以供后续播放) (该方法会对输入数据进行类型检查和格式化，然后将新数据追加到 samples 数组中)
PCMPlayer.prototype.feed = function(data) {
	if (!this.isTypedArray(data)) return;
	data = this.getFormatedValue(data);
	let tmp = new Float32Array(this.samples.length + data.length);
	tmp.set(this.samples, 0);
	tmp.set(data, this.samples.length);
	this.samples = tmp;
};
//获取格式化值
PCMPlayer.prototype.getFormatedValue = function(dt) {
	let data = new this.typedArray(dt.buffer),
		float32 = new Float32Array(data.length),
		i;

	for (i = 0; i < data.length; i++) {
		float32[i] = data[i] / this.maxValue;
	}
	return float32;
};
//用于动态调整音量，通过修改 gainNode 的增益值来实现
PCMPlayer.prototype.volume = function(volume) {
	if (this.gainNode) {
		this.gainNode.gain.value = volume;
	}
};
//销毁(用于销毁播放器对象，清理资源。它会清空 samples 数组并关闭音频上下文)
PCMPlayer.prototype.destroy = function() {
	if (this.interval) {
		clearInterval(this.interval);
		this.interval = null;
	}
	this.samples = null;
	this.audioCtx.close();
	this.audioCtx = null;
};
//刷新 (将 samples 中的数据转换为音频缓冲区，并在合适的时间点播放该缓冲区) (方法还包括对音频数据进行淡入和淡出处理，以避免声音突然开始或结束)
PCMPlayer.prototype.flush = function() {
	if (!this.samples.length) return;
	let bufferSource = this.audioCtx.createBufferSource(),
		length = this.samples.length / this.option.channels,
		audioBuffer = this.audioCtx.createBuffer(this.option.channels, length, this.option.sampleRate),
		audioData,
		channel,
		offset,
		i,
		decrement;

	for (channel = 0; channel < this.option.channels; channel++) {
		audioData = audioBuffer.getChannelData(channel);
		offset = channel;
		decrement = 50;
		for (i = 0; i < length; i++) {
			audioData[i] = this.samples[offset];
			/* fadein */
			if (i < 50) {
				audioData[i] = (audioData[i] * i) / 50;
			}
			/* fadeout*/
			if (i >= (length - 51)) {
				audioData[i] = (audioData[i] * decrement--) / 50;
			}
			offset += this.option.channels;
		}
	}

	if (this.startTime < this.audioCtx.currentTime) {
		this.startTime = this.audioCtx.currentTime;
	}
	bufferSource.buffer = audioBuffer;
	bufferSource.connect(this.gainNode);
	bufferSource.start(this.startTime);
	this.startTime += audioBuffer.duration;
	this.samples = new Float32Array();
};
//获取时间戳
PCMPlayer.prototype.getTimestamp = function() {
	if (this.audioCtx) {
		return this.audioCtx.currentTime * 1000;
	} else {
		return 0;
	}
};
//播放（方法用于播放传入的 PCM 数据。它会根据数据长度、声道数和采样率创建音频缓冲区，并设置播放速度 (speed)）（此方法与 flush 方法类似，也包含了淡入淡出处理，并会更新 startTime 以确保连续播放）
PCMPlayer.prototype.play = function(data, speed) {
	if (!this.isTypedArray(data)) {
		return;
	}

	data = this.getFormatedValue(data);
	if (!data.length) {
		return;
	}
	//状态判断
	if (this.audioCtx.state != "running") {
		// console.log("audio state",this.audioCtx.state);
		return;
	}

	let bufferSource = this.audioCtx.createBufferSource(),
		length = data.length / this.option.channels,
		audioBuffer = this.audioCtx.createBuffer(this.option.channels, length, this.option.sampleRate),
		audioData,
		channel,
		offset,
		i,
		decrement;

	//淡入，淡出
	for (channel = 0; channel < this.option.channels; channel++) {
		audioData = audioBuffer.getChannelData(channel);
		offset = channel;
		decrement = 50;
		for (i = 0; i < length; i++) {
			audioData[i] = data[offset];
			/* fadein */
			if (i < 50) {
				audioData[i] = (audioData[i] * i) / 50;
			}
			/* fadeout*/
			if (i >= (length - 51)) {
				audioData[i] = (audioData[i] * decrement--) / 50;
			}
			offset += this.option.channels;
		}
	}

	// let complexFilter = this.createComplexFilter();
	bufferSource.buffer = audioBuffer;

	if (this.startTime < this.audioCtx.currentTime) {
		this.startTime = this.audioCtx.currentTime;
	}

	// 连接复杂滤波器到音频输出 --使用此模块会导致cpu增长
	// bufferSource.connect(complexFilter);
	// complexFilter.connect(this.gainNode);
	bufferSource.connect(this.gainNode);
	bufferSource.playbackRate.value = speed; // 正常播放速度
	bufferSource.detune.value = 0; // 保持音调不变
	bufferSource.start(this.startTime);

	// 动态计算 speed
	let diff = this.startTime - this.audioCtx.currentTime;
	let syncThreshold = 0.1; // 100ms
	if (Math.abs(diff) > syncThreshold) {
		if (diff > 0) {
			speed *= 1 + diff / (audioBuffer.duration * speed);
		}
	}

	this.startTime += audioBuffer.duration / speed;

};
//获取音频当前基准时间
PCMPlayer.prototype.getCurrentTime = function() {
	if (this.audioCtx) {
		return this.audioCtx.currentTime;
	}
	return 0;
}
//暂停音频
PCMPlayer.prototype.pause = function() {
	if (this.audioCtx) {
		this.audioCtx.suspend()
	}
}
//恢复音频
PCMPlayer.prototype.resume = function() {
	if (this.audioCtx) {
		this.audioCtx.resume()
	}
}
//高，低通滤波器
PCMPlayer.prototype.createComplexFilter = function() {
	// 创建低通滤波器
	let lowPassFilter = this.audioCtx.createBiquadFilter();
	lowPassFilter.type = "lowpass";
	lowPassFilter.frequency.value = 3000;

	// 创建高通滤波器
	let highPassFilter = this.audioCtx.createBiquadFilter();
	highPassFilter.type = "highpass";
	highPassFilter.frequency.value = 100; // 去除100Hz以下的噪音

	// 将低通滤波器和高通滤波器级联
	lowPassFilter.connect(highPassFilter);

	return lowPassFilter;
};


 