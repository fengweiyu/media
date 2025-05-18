/*
 * Copyright (C) 2020-2025 Hanson Yu All Rights Reserved.
 *
 * @author yu weifeng 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//import DownloadMedia from './common.js'; 
import { customLog, DownloadMedia } from './common.js';
import { PCMPlayer } from './PlayPCM.js';
//import './build/MediaConvert.js'; 

Module.onRuntimeInitialized = async function() 
{
	console.log('onRuntimeInitialized:'); 
	const FontFile='msyh.ttf';
	const fontResponse = await fetch(FontFile);  
	const fontArrayBuffer = await fontResponse.arrayBuffer();
	Module.FS.writeFile('msyh.ttf', new Uint8Array(fontArrayBuffer)); 
}

var oConvert=null

class convert 
{
    constructor() 
    {
        this.m_MediaSource = new MediaSource();
        this.m_MediaElement = null;
		this.m_CavasElement = null;
		this.m_DstName = null;
		this.m_SrcName = null;
		this.m_SrcType = null;
		this.m_SrcBuffer = null;
		this.m_SourceBuffer=null;  
        oConvert=this;
		this.m_DownBuffer = new Uint8Array(0); // 累计缓冲区
		this.m_BufferQueue = []; // 数据队列，用于存储转换后的帧数据与帧信息
		this.m_VideoBufferQueue = []; // 数据队列，用于存储转换后的帧数据与帧信息
		this.m_AudioBufferQueue = []; // 数据队列，用于存储转换后的帧数据与帧信息
		this.m_LastAudioTimestamp = null;
		this.m_LastVideoTimestamp = null;
		//this.audioContext = new(window.AudioContext || window.webkitAudioContext)();//new window.AudioContext;//new (window.AudioContext || window.webkitAudioContext)();  
		/*switch (nSampleFmt) {
			case 0:
				szEncoding = "8bitInt";
				break;
			case 1:
				szEncoding = "16bitInt";
				break;
			case 2:
				szEncoding = "32bitInt";
				break;
			case 3:
				szEncoding = "32bitFloat";
				break;
			case 4:
				szEncoding = "64bitFloat";
				break;
			case 5:
				szEncoding = "8bitInt";
				break;
			case 6:
				szEncoding = "16bitInt";
				break;
			case 7:
				szEncoding = "32bitInt";
				break;
			case 8:
				szEncoding = "32bitFloat";
				break;
			case 9:
				szEncoding = "64bitFloat";
				break;
			case 10:
			case 11:
				szEncoding = "64bitInt";
				break;
			default:
				this.logger.logError("Unsupported audio sampleFmt " + nSampleFmt + "!");
		}*/
		this.m_pPcmPlayer = null;
		this.m_iChannels = 1;
		this.m_iSampleRate = 8000;
		this.m_strSampleFmtEncoding = "16bitInt";
		this.m_CavasCtx = null;
    }

    attachMediaElement(mediaElement,cavasElement,textMark,enableMark) 
    {
        this.m_MediaElement = mediaElement;
		this.m_CavasElement = cavasElement;
		const strFontFile='msyh.ttf';
		const strWaterMark=textMark;	
		this.setWaterMark(enableMark,strFontFile,strWaterMark);
		//setTransCodec(1,'h264',0,null);
		this.m_CavasCtx = this.m_CavasElement.getContext('2d'); 
    }
	
    playAudioPCM(channels,sampleRate,pcmData) 
	{  
		//pcmData = await audioContext.decodeAudioData(arrayBuffer); 

		// 创建 AudioBuffer，并将 Float32Array PCM 数据加载到其中  
		/*const audioBuffer = this.audioContext.createBuffer(channels, pcmData.length,sampleRate);  
		audioBuffer.copyToChannel(pcmData, 0); // 将PCM数据复制到 AudioBuffer 的第一个通道  

		// 创建 AudioBufferSourceNode  
		const source = this.audioContext.createBufferSource();  
		source.buffer = audioBuffer; // 设置要播放的音频缓冲  
		source.connect(this.audioContext.destination); // 连接到音频输出  
		source.start(0); // 开始播放 */
    }
    setWaterMark(Enable,FontFile,WaterMark) 
    {
		let arrFontFile = intArrayFromString(FontFile).concat(0);
		let bufFontFile = Module._malloc(arrFontFile.length);
		Module.HEAPU8.set(arrFontFile, bufFontFile);
		let arrWaterMark = intArrayFromString(WaterMark).concat(0);
		let bufWaterMark = Module._malloc(arrWaterMark.length);
		Module.HEAPU8.set(arrWaterMark, bufWaterMark);
		Module._SetWaterMark(Enable,bufWaterMark,arrWaterMark.length,bufFontFile,arrFontFile.length);	
    }
	setTransCodec(iVideoEnable,iDstVideoEncodec,iAudioEnable,iDstAudioEncodec) 
    {
		var DstVideoEncodecLen=0;
		var DstAudioEncodecLen=0;
		var bufDstVideoEncodec=null;
		var bufDstAudioEncodec=null;
		if(iDstVideoEncodec!=null)
		{
			let arrDstVideoEncodec = intArrayFromString(iDstVideoEncodec).concat(0);
			bufDstVideoEncodec = Module._malloc(arrDstVideoEncodec.length);
			Module.HEAPU8.set(arrDstVideoEncodec, bufDstVideoEncodec);
			DstVideoEncodecLen=arrDstVideoEncodec.length;
		}
		if(iDstAudioEncodec!=null)
		{
			let arrDstAudioEncodec = intArrayFromString(iDstAudioEncodec).concat(0);
			bufDstAudioEncodec = Module._malloc(arrDstAudioEncodec.length);
			Module.HEAPU8.set(arrDstAudioEncodec, bufDstAudioEncodec);
			DstAudioEncodecLen=arrDstAudioEncodec.length;
		}
		Module._SetTransCodec(iVideoEnable,bufDstVideoEncodec,DstVideoEncodecLen,iAudioEnable,bufDstAudioEncodec,DstAudioEncodecLen);	
    }
    detachMediaElement() 
    {
        if (this.m_MediaElement) 
        {
            this.m_MediaElement = null;
        }
        if (this.m_CavasElement) 
		{
			this.m_CavasElement = null;
		}
		if (this.m_pPcmPlayer) 
		{
			this.m_pPcmPlayer.destroy();
			this.m_pPcmPlayer = null;
		}
    }
    appendTypedArray(a, b) 
    {  
        // 创建一个新的 TypedArray，长度为 a 和 b 的长度之和  
        const result = new (a.constructor)(a.length + b.length);  
        
        // 将 a 的内容复制到新数组中  
        result.set(a, 0);  
        
        // 将 b 的内容复制到新数组中，从 a 的末尾开始  
        result.set(b, a.length);  
        
        return result;  
    } 
    process(SrcBuffer,SrcName,SrcType,DstName) 
    {
		this.m_SrcName = SrcName;
		this.m_SrcType = SrcType;
		this.m_DstName = DstName;//"OriginalData"
		this.m_SrcBuffer = SrcBuffer;

		console.log('process v '+SrcName+' a '+DstName);
		if(".OriginalData" == DstName)
		{
			this.convert(this.m_SrcBuffer,this.m_SrcType,this.m_DstName);//OriginalData
			return;
		}

		if(null != this.m_MediaElement)
			this.m_MediaElement.src = URL.createObjectURL(this.m_MediaSource); 
		this.m_MediaSource.addEventListener('sourceopen', this.MediaSourceOpen);//静态方法
    }
	playVideo() 
	{  
		var convertedChunk = null; 
		var timeInterval = 10;
		var dwFrameTimeStamp = 10;
		if (this.m_VideoBufferQueue.length > 0) 
		{	//{ data: convertedChunk, haveKeyFrame,startTime,durationTime,videoCnt,audioCnt,iEncType,iFrameType,dwFrameTimeStamp,width,height}
			const dequeuedObject = this.m_VideoBufferQueue.shift(); // 从队列中取出一个数据块
			convertedChunk=dequeuedObject.data;
			var width = dequeuedObject.width;
			var height = dequeuedObject.height;
			dwFrameTimeStamp = dequeuedObject.dwFrameTimeStamp;
			if(this.m_CavasElement && width>0 && height>0)//video rgba
			{
				/*if( this.m_DownBuffer.length<=0)
				{
					this.m_CavasElement.style.display = 'block'; // 显示视频 = 'none'; // 隐藏视频  
					this.m_CavasElement.width = width; //如果不设置会出现宽高不一致的问题，导致画面被缩放
					this.m_CavasElement.height = height; //这三句只要执行一次即可，条件换成flag标记来做也行

					this.m_DownBuffer=convertedChunk;
					//this.rgbaToBMP(convertedChunk,width,height);//bmp不会渲染a通道即透明度不会被处理
				}
				else
				{
					//this.m_DownBuffer=this.appendTypedArray(this.m_DownBuffer,convertedChunk);
				}
				if(this.m_DownBuffer.length>=1)//0*1024*1024
				{ 
					//DownloadMedia(this.m_DownBuffer,this.m_SrcName+this.m_DstName);
					//this.m_DownBuffer=null;
				}*/					

				if(this.m_CavasElement.style.display=='none')
				{
					this.m_CavasElement.style.display = 'block'; // 显示视频 = 'none'; // 隐藏视频  
					this.m_CavasElement.width = width; //如果不设置会出现宽高不一致的问题，导致画面被缩放
					this.m_CavasElement.height = height; //这三句只要执行一次即可，条件换成flag标记来做也行

					//this.rgbaToBMP(convertedChunk,width,height);//bmp不会渲染a通道即透明度不会被处理
				}
				// 将Uint8Array封装为ImageData  
				const imageData = new ImageData(new Uint8ClampedArray(convertedChunk.buffer), width, height);  
				// 绘制到Canvas  
				this.m_CavasCtx.putImageData(imageData, 0, 0); //drawImage putImageData

				/*var imageData = this.m_CavasCtx.createImageData(width, height);
				var data = imageData.data;
				for(var i=0; i < width*height; i++) 
				{
					var index = i*4;
					data[index + 0] = convertedChunk[index + 0];//255; // R值，例如红色
					data[index + 1] = convertedChunk[index + 1];   // G值，例如绿色
					data[index + 2] = convertedChunk[index + 2];   // B值，例如蓝色
					data[index + 3] = 255;//convertedChunk[index + 3]; // A值，例如半透明度，这里设置255不透明
				}*/
				/*for (var y = 0; y < height; y++) {
					for (var x = 0; x < width; x++) {
						var index = (y * width + x) * 4; // 计算当前像素在data数组中的索引
						data[index + 0] = convertedChunk[index + 0];//255; // R值，例如红色
						data[index + 1] = convertedChunk[index + 1];   // G值，例如绿色
						data[index + 2] = convertedChunk[index + 2];   // B值，例如蓝色
						data[index + 3] = 255;//convertedChunk[index + 3]; // A值，例如半透明度，这里设置255不透明
					}
				}*/
				/*this.m_CavasCtx.putImageData(imageData, 0, 0); // 绘制图像数据到Canvas上*/

				
				/*if(dequeuedObject.iFrameType==10)
				{
					// 转换为PNG格式的Base64 URL  
					const pngDataUrl = this.m_CavasElement.toDataURL('image/png');
					const link = document.createElement('a');  
					link.href = pngDataUrl;  
					link.download = 'image.png'; // 设置下载后文件名  
					document.body.appendChild(link); // 必须加入DOM  
					link.click(); // 触发下载  
					document.body.removeChild(link); // 下载后移除  
				}*/			
			}
		} 
		if(null == this.m_LastVideoTimestamp)
		{
			this.m_LastVideoTimestamp=dwFrameTimeStamp;
		}
		timeInterval=dwFrameTimeStamp-this.m_LastVideoTimestamp;
		this.m_LastVideoTimestamp=dwFrameTimeStamp;
		console.log('playVideo timeInterval '+timeInterval+' dwFrameTimeStamp '+dwFrameTimeStamp);
		setTimeout(this.playVideo.bind(this), timeInterval);//解码转码耗时所以得不到调用会卡(单线程)
	}
	playAudio() 
	{  
		var convertedChunk = null; 
		var timeInterval = 10;
		var dwFrameTimeStamp = 10;
		if (this.m_AudioBufferQueue.length > 0) 
		{	//{ data: convertedChunk, haveKeyFrame,startTime,durationTime,videoCnt,audioCnt,iEncType,iFrameType,dwFrameTimeStamp,width,height}
			const dequeuedObject = this.m_AudioBufferQueue.shift(); // 从队列中取出一个数据块
			convertedChunk=dequeuedObject.data;
			dwFrameTimeStamp = dequeuedObject.dwFrameTimeStamp;
			if (!this.m_pPcmPlayer) 
			{
				this.m_pPcmPlayer = new PCMPlayer({
					encoding: this.m_strSampleFmtEncoding,
					channels: this.m_iChannels,
					sampleRate: this.m_iSampleRate,
					flushingTime: 5000,
					volume: 1
				});//change 音量（将音量（增益值）设置为 1，表示全音量播放。如果需要调低音量，可以将 value 设置为一个小于 1 的值。）
			}
			const subBuffer = convertedChunk.slice(44);//偏移wav头
			this.m_pPcmPlayer.play(subBuffer, 1); //new Uint8Array(subBuffer)  固定用1倍速
		}
		if(null == this.m_LastAudioTimestamp)
		{
			this.m_LastAudioTimestamp=dwFrameTimeStamp;
		}
		timeInterval=dwFrameTimeStamp-this.m_LastAudioTimestamp;
		this.m_LastAudioTimestamp=dwFrameTimeStamp;
		console.log('playAudio timeInterval '+timeInterval+' dwFrameTimeStamp '+dwFrameTimeStamp);
		setTimeout(this.playAudio.bind(this), timeInterval);//解码转码耗时所以得不到调用会卡(单线程)
	}
	downWAV(convertedChunk) 
	{  
		if( this.m_DownBuffer.length<=0)
			{
				this.m_DownBuffer=convertedChunk.slice(44);
			}
			else
			{
				this.m_DownBuffer=this.appendTypedArray(this.m_DownBuffer,convertedChunk.slice(44));
			}
			if(this.m_DownBuffer.length>=1*1024*1024)//0*1024*1024
			{ 
				const buffer = new ArrayBuffer(44);  
				const view = new DataView(buffer);  						
				// RIFF identifier 'RIFF'  
				this.writeString(view, 0, 'RIFF');  	
				// file length minus first 8 bytes  
				view.setUint32(4, 36 + this.m_DownBuffer.length, true);  
				// RIFF type 'WAVE'  
				this.writeString(view, 8, 'WAVE');  
				// format chunk identifier 'fmt '  
				this.writeString(view, 12, 'fmt ');  
				view.setUint32(16, 16, true); // chunk size  
				view.setUint16(20, 1, true); // format (PCM)  
				view.setUint16(22, 1, true);  
				view.setUint32(24, 8000, true);  
				view.setUint32(28, 8000 * 1 * 16 / 8, true); // byte rate  
				view.setUint16(32, 1 * 16 / 8, true); // block align  
				view.setUint16(34, 16, true); // bits per sample  
				// data chunk header  
				this.writeString(view, 36, 'data');  
				view.setUint32(40, this.m_DownBuffer.length, true);  
				const uint8View = new Uint8Array(buffer);  
				const DownBuffer=this.appendTypedArray(uint8View,this.m_DownBuffer);	
				DownloadMedia(DownBuffer,this.m_SrcName+this.m_DstName);
				this.m_DownBuffer=null;
			}	
	}
	writeString(view, offset, string) 
	{  
		for (let i = 0; i < string.length; i++) {  
		  view.setUint8(offset + i, string.charCodeAt(i));  
		}  
	}
	MediaSourceOpen() 
    {//静态方法
        oConvert.MediaSourceHandle();//或者使用绑定方法
    }
	MediaSourceHandle()
	{  
		this.convert(this.m_SrcBuffer,this.m_SrcType,this.m_DstName);//OriginalData
        var convertedChunk = null; 
        while (this.m_BufferQueue.length > 0) 
        {//{ data: convertedChunk, haveKeyFrame,startTime,durationTime,videoCnt,audioCnt,iEncType,iFrameType,dwFrameTimeStamp}
            const dequeuedObject = this.m_BufferQueue.shift(); // 从队列中取出一个数据块
            convertedChunk=dequeuedObject.data;

			if (convertedChunk) 
			{ // 成功创建，输出内容
				if(this.m_DstName == ".mp4")
				{
					if(null == this.m_SourceBuffer)
					{// codecs="avc1.42E01E, mp4a.40.2" h264 aac   
						var VideoCodec=null;
						var AudioCodec=null;
						let mMedia = this.GetMediaDstEnc();
						let v = mMedia[0];
						let a = mMedia[1];
						if (v.includes("h264"))//video/webm; codecs="avc1.64001E"：用于 WebM 容器中的 H.264 视频（通常则使用 VP8 或 VP9）。
						{//video/mp4; codecs="avc1.64001E"：用于 MP4 容器中的 H.264 视频。
							VideoCodec="avc1.42E01E";//avc1.42E01E
						}//video/mp2t; codecs="avc1.42E01E"：用于 MPEG-TS 容器中的 H.264 视频
						else if (v.includes("h265"))
						{//video/mp4; codecs="hevc"：用于 MPEG-4 容器格式中的 H.265 视频。
							VideoCodec="hvc1.1.6.L93.B0";
						}//用于传输流（如 TS 或 MPEG-TS）编码中的 H.265 视频。
						if (a.includes("aac"))
						{
							AudioCodec="mp4a.40.2";//audio/mp4; codecs="mp4a.40.2"：这是一般用于 MP4 容器中的 AAC 音频格式。
						}//audio/aac：用于裸 AAC 数据。
						else if(a.includes("g711a"))
						{
							AudioCodec="alaw";//audio/mp4; codecs="law"：表示使用 G.711 A-law 编码的音频数据。
						}//audio/g711：用于直接表示 G.711 编码的数据。
						if(null != VideoCodec && null !=AudioCodec)
						{
							this.m_SourceBuffer = this.m_MediaSource.addSourceBuffer('video/mp4; codecs="'+VideoCodec+','+AudioCodec+'"');
							localVideoElement.style.display = 'block'; // 显示视频 = 'none'; // 隐藏视频  
						}
						else if(null != VideoCodec)
						{
							this.m_SourceBuffer = this.m_MediaSource.addSourceBuffer('video/mp4; codecs="'+VideoCodec+'"');
							localVideoElement.style.display = 'block'; // 显示视频 = 'none'; // 隐藏视频  
						}
						else if(null != AudioCodec)
						{
							this.m_SourceBuffer = this.m_MediaSource.addSourceBuffer('video/mp4; codecs="'+AudioCodec+'"');
							localVideoElement.style.display = 'block'; // 显示视频 = 'none'; // 隐藏视频  
						}
						else
						{
							console.log('GetMediaDstEnc err v '+VideoCodec+' a '+AudioCodec);    
						}
					}
  
				}   
				if( this.m_DownBuffer==null)
					this.m_DownBuffer=convertedChunk;
				else
				{
					this.m_DownBuffer=this.appendTypedArray(this.m_DownBuffer,convertedChunk);
				}
			} 
			else
			{// 处理未成功创建的情况 
				console.log('FormatConvert err'); 
			} 			
        }
		if(this.m_DownBuffer.length>=1)
		{
			if(null != this.m_SourceBuffer)
				this.m_SourceBuffer.appendBuffer(this.m_DownBuffer);  
			DownloadMedia(this.m_DownBuffer,this.m_SrcName+this.m_DstName);
			//this.m_DownBuffer=null;
		}
		/*sourceBuffer.addEventListener('updateend', function() {  
			if (!sourceBuffer.updating) {  
				//mediaSource.endOfStream();  
				//localVideoElement.play();  
			}  
		}); */
	}


	GetMediaDstEnc()
	{
		var bufDstMaxLen=10;
		let bufDstVideoEnc = Module._malloc(bufDstMaxLen);
		let bufDstAudioEnc = Module._malloc(bufDstMaxLen);
		var ret = Module._GetEncodeType(bufDstVideoEnc,bufDstMaxLen,bufDstAudioEnc,bufDstMaxLen);
		let VideoArray = new Uint8Array(bufDstMaxLen);
		VideoArray.set(new Uint8Array(Module.HEAPU8.buffer, bufDstVideoEnc, 6)); // 将C中的数组拷贝到JS数组中
		let AudioArray = new Int8Array(bufDstMaxLen);
		AudioArray.set(new Uint8Array(Module.HEAP8.buffer, bufDstAudioEnc,6)); // 将C中的数组拷贝到JS数组中
		const stringVideo = String.fromCharCode.apply(null, VideoArray);  
		const stringAudio = String.fromCharCode.apply(null, AudioArray);  
		Module._free(bufDstVideoEnc);
		Module._free(bufDstAudioEnc);
		return [stringVideo,stringAudio];
	}

	convert(srcBuffer,filetype,dstName) 
	{ 
		if (!srcBuffer || srcBuffer.byteLength === 0) 
		{
            console.error('Invalid source buffer');
            return;
        }        
		const dstType = dstName;
		/*const arr = new Float32Array([1.0, 2.0, 3.0]); // 示例数组  
		const size = arr.length * arr.BYTES_PER_ELEMENT; // 计算大小  
		const ptr = instance._malloc(size); // 在Wasm内存中申请空间  
		instance.HEAPF32.set(arr, ptr / arr.BYTES_PER_ELEMENT); // 拷贝数据到内存  
		instance._your_function(ptr, arr.length); // 调用C函数，传入数据指针和数组长度  
		instance._free(ptr); // 调用完后释放内存  */
		let arrDstType = intArrayFromString(dstType).concat(0);
		let bufDstType = Module._malloc(arrDstType.length);
		Module.HEAPU8.set(arrDstType, bufDstType);

		let arrSrctype = intArrayFromString(filetype).concat(0);
		let bufSrcType = Module._malloc(arrSrctype.length);
		Module.HEAPU8.set(arrSrctype, bufSrcType);

		const uint8View = new Uint8Array(srcBuffer);//arrayBuffer
		let bufSrcFrame = Module._malloc(uint8View.byteLength);
		Module.HEAPU8.set(uint8View,bufSrcFrame);//bufSrcFrame,arrayBuffer,要Uint8Array才可以设置进去拷贝，arrayBuffer不能拷贝
		//const lengthPtr = Module._malloc(arrayBuffer.byteLength);
		//const uint8View = new Uint8Array(Module.HEAPU8.buffer, bufSrcFrame, arrayBuffer.byteLength).set(arrayBuffer);
		Module._InputData(bufSrcFrame,uint8View.byteLength,bufSrcType,bufDstType);
		//const buffer = new ArrayBuffer(arrayBuffer.byteLength); 
		//const uint8View = new Uint8Array(buffer); 
		var bufDstMaxLen=uint8View.byteLength+(20*1024*1024);
		let bufDstFrame = Module._malloc(bufDstMaxLen);
		const bufDstInfoMaxLen = 80;
        const bufDstInfo = Module._malloc(bufDstInfoMaxLen);
		var ret=0;
		var len=0; 

        do {
            ret = Module._GetData(bufDstFrame, bufDstMaxLen,bufDstInfo,bufDstInfoMaxLen);//arrayBuffer.buffer arrayBuffer.byteOffset
            if(ret <= 0)
            {
                break;                   
            }
            len = ret;
            const convertedChunk = new Uint8Array(len);
            convertedChunk.set(new Uint8Array(Module.HEAPU8.buffer, bufDstFrame, len));
            const convertedInfo = new Int32Array(bufDstInfoMaxLen);
            convertedInfo.set(new Int32Array(Module.HEAP32.buffer, bufDstInfo, bufDstInfoMaxLen));
            const haveKeyFrame = convertedInfo[0]; // 第一个元素表示是否有i帧，第二个元素是开始时间，第三个元素是持续时间
            const startTime = convertedInfo[1]; // 
            const durationTime = convertedInfo[2]; //
            const videoCnt = convertedInfo[3]; // 
            const audioCnt = convertedInfo[4]; //
            var absTimeHigh = BigInt(convertedInfo[5]); // Convert to BigInt  
            var absTimeLow = BigInt(convertedInfo[6]);  // Convert to BigInt  
            const absTimeS = convertedInfo[7]; //
			const iEncType = convertedInfo[8];//当xxxFrameCnt=1是可以用(此时该字段表示原始编码类型不是解码后的,解码后的用GetEncodeType方法)，E_MediaEncodeType h264 1 ,h265 2 ,aac 5 ,g711a 6 ,pcm 11,rgba 15
			const iFrameType = convertedInfo[9];//当xxxFrameCnt=1是可以用，E_MediaFrameType I帧1 P帧2 B帧3 A音频帧4
			const dwFrameTimeStamp = convertedInfo[10];//
			const width = convertedInfo[11];//
			const height = convertedInfo[12];//

            // Combine the two values  
            const absTime = (absTimeHigh << BigInt(32)) | absTimeLow; // Use | for combined value  
            // 创建 Date 对象  
            const absTimeNumber =absTimeS*1000;//Number(absTime);
            const date = new Date(absTimeNumber);  //absTime.toString()
            // 格式化时间  
            const formattedDate = date.toLocaleString('zh-CN', {  
                year: 'numeric',  
                month: '2-digit',  
                day: '2-digit',  
                hour: '2-digit',  
                minute: '2-digit',  
                second: '2-digit',  
                //fractionalSecondDigits: 3, // 显示毫秒  
                hour12: false, // 24小时制
            });  
            
            //console.log('convertedChunk haveKeyFrame '+haveKeyFrame+' startTime '+startTime+' durationTime '+durationTime+' videoCnt '+videoCnt+' audioCnt '+audioCnt+' absTime '+formattedDate); 
			if((iFrameType==1||iFrameType==2||iFrameType==3) && 1==videoCnt)
			{//软解
				this.m_VideoBufferQueue.push({ data: convertedChunk, haveKeyFrame,startTime,durationTime,videoCnt,audioCnt,iEncType,iFrameType,dwFrameTimeStamp,width,height});
			}
			else if((iFrameType==4) && 1==audioCnt)
			{//软解
				this.m_AudioBufferQueue.push({ data: convertedChunk, haveKeyFrame,startTime,durationTime,videoCnt,audioCnt,iEncType,iFrameType,dwFrameTimeStamp,width,height});
			}
			else
			{
				this.m_BufferQueue.push({ data: convertedChunk, haveKeyFrame,startTime,durationTime,videoCnt,audioCnt,iEncType,iFrameType,dwFrameTimeStamp,width,height});
			}
		} while (1);


        Module._free(bufDstType);
        Module._free(bufSrcType);
        Module._free(bufSrcFrame);
        Module._free(bufDstFrame);
        Module._free(bufDstInfo);
	} 

    destroy() 
    {
        this.detachMediaElement();
        oConvert=null;
    }
	/**  
	 * 将 RGBA 数据转换为 BMP (32-bit) 文件  
	 * @param {Uint8Array} rgbaData - RGBA 像素数据 (每像素4字节: R,G,B,A)  
	 * @param {number} width - 图像宽度  
	 * @param {number} height - 图像高度  
	 * @returns {Blob} - 返回 BMP 文件的 Blob 对象  
	 */  
	rgbaToBMP(rgbaData, width, height) 
	{  
		// BMP 文件头结构 (共14字节)  
		const fileHeaderSize = 14;  
		// BMP 信息头结构 (共40字节)  
		const infoHeaderSize = 40;  
		// 每像素字节数 (32位 = 4字节)  
		const bytesPerPixel = 4;  
		// 每行字节数 (32位BMP不需要行填充)  
		const rowSize = width * bytesPerPixel;  
		// 像素数据总大小  
		const pixelDataSize = rowSize * height;  
		
		// 创建 ArrayBuffer 存储整个BMP文件  
		const buffer = new ArrayBuffer(fileHeaderSize + infoHeaderSize + pixelDataSize);  
		const view = new DataView(buffer);  
		
		let offset = 0;  
		
		// --- 写入文件头 (14字节) ---  
		view.setUint16(offset, 0x4D42, true);  // 'BM' 标识  
		offset += 2;  
		view.setUint32(offset, fileHeaderSize + infoHeaderSize + pixelDataSize, true); // 文件总大小  
		offset += 4;  
		view.setUint16(offset, 0, true);       // 保留字段1  
		offset += 2;  
		view.setUint16(offset, 0, true);       // 保留字段2  
		offset += 2;  
		view.setUint32(offset, fileHeaderSize + infoHeaderSize, true); // 像素数据偏移量  
		offset += 4;  
		
		// --- 写入信息头 (40字节) ---  
		view.setUint32(offset, infoHeaderSize, true);  // 信息头大小  
		offset += 4;  
		view.setInt32(offset, width, true);     // 图像宽度  
		offset += 4;  
		view.setInt32(offset, height, true);    // 图像高度 (正数表示倒序)  
		offset += 4;  
		view.setUint16(offset, 1, true);       // 颜色平面数 (必须为1)  
		offset += 2;  
		view.setUint16(offset, 32, true);      // 每像素位数 (32)  
		offset += 2;  
		view.setUint32(offset, 0, true);       // 压缩方式 (0=不压缩)  
		offset += 4;  
		view.setUint32(offset, pixelDataSize, true); // 像素数据大小  
		offset += 4;  
		view.setInt32(offset, 0, true);        // 水平分辨率 (像素/米)  
		offset += 4;  
		view.setInt32(offset, 0, true);        // 垂直分辨率 (像素/米)  
		offset += 4;  
		view.setUint32(offset, 0, true);      // 调色板颜色数 (0=不使用)  
		offset += 4;  
		view.setUint32(offset, 0, true);      // 重要颜色数 (0=全部重要)  
		offset += 4;  
		
		// --- 写入像素数据 (BGRA顺序) ---  
		const pixels = new Uint8Array(buffer, offset);  
		for (let y = height - 1; y >= 0; y--) 
		{ // BMP是倒序存储  
			const srcRow = y * width * 4;  
			const dstRow = (height - 1 - y) * width * 4;  
			for (let x = 0; x < width; x++) 
			{  
				const srcPos = srcRow + x * 4;  
				const dstPos = dstRow + x * 4;  
				// RGBA -> BGRA  
				pixels[dstPos]     = rgbaData[srcPos + 2]; // B  
				pixels[dstPos + 1] = rgbaData[srcPos + 1]; // G  
				pixels[dstPos + 2] = rgbaData[srcPos];     // R  
				pixels[dstPos + 3] = rgbaData[srcPos + 3]; // A  
			}  
		}  

		// 创建下载链接  
		const url = URL.createObjectURL(new Blob([buffer], { type: 'image/bmp' }));  
		const a = document.createElement('a');  
		a.href = url;  
		a.download = 'output_32bit.bmp';  
		document.body.appendChild(a);  
		a.click();  
		document.body.removeChild(a);  
		URL.revokeObjectURL(url); 
	} 

    
}

export default convert;