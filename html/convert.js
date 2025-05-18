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
//import './build/MediaConvert.js'; //export { Module , intArrayFromString };
import { Module , intArrayFromString } from './MediaConvert.js'; //worker创建的时候才加载，所以不会使用全局的，也就需要这么引用，否则会报Module未定义错误



class convert 
{
    constructor() 
    {
		this.m_DstName = null;
		this.m_SrcType = null;
		this.m_SrcBuffer = null;
		this.m_SourceBuffer=null;  
		this.m_DstEnc = null;
		this.m_FontFile = 'msyh.ttf';

		self.onmessage = this.process.bind(this);
		Module.onRuntimeInitialized = this.ModuleInitialized.bind(this);
    }
    async ModuleInitialized() 
    {
		console.log('onRuntimeInitialized:'); 

		const fontResponse = await fetch(this.m_FontFile);  
		const fontArrayBuffer = await fontResponse.arrayBuffer();
		Module.FS.writeFile(this.m_FontFile, new Uint8Array(fontArrayBuffer)); 
		self.postMessage({res:'ModuleInitialized',data:{FontFile:this.m_FontFile}}); 
    }
    setWaterMark(Enable,WaterMark) 
    {
		const FontFile=this.m_FontFile;

		let arrFontFile = intArrayFromString(FontFile).concat(0);
		let bufFontFile = Module._malloc(arrFontFile.length);
		Module.HEAPU8.set(arrFontFile, bufFontFile);
		let arrWaterMark = intArrayFromString(WaterMark).concat(0);
		let bufWaterMark = Module._malloc(arrWaterMark.length);
		Module.HEAPU8.set(arrWaterMark, bufWaterMark);
		Module._SetWaterMark(Enable,bufWaterMark,arrWaterMark.length,bufFontFile,arrFontFile.length);	
    }
	setTransCodec(iVideoEnable,strDstVideoEncodec,iAudioEnable,strDstAudioEncodec,strDstAudioSampleRate) 
    {//mp4硬解必然会设置音频编码为aac 44100否则web端无法播放,软解的时候只针对音频采样率会有处理
		var DstVideoEncodecLen=0;
		var DstAudioEncodecLen=0;
		var DstAudioSampleRateLen=0;
		var bufDstVideoEncodec=null;
		var bufDstAudioEncodec=null;
		var bufDstAudioSampleRate=null;
		
		if(strDstVideoEncodec!=null)
		{
			let arrDstVideoEncodec = intArrayFromString(strDstVideoEncodec).concat(0);
			bufDstVideoEncodec = Module._malloc(arrDstVideoEncodec.length);
			Module.HEAPU8.set(arrDstVideoEncodec, bufDstVideoEncodec);
			DstVideoEncodecLen=arrDstVideoEncodec.length;
		}
		if(strDstAudioEncodec!=null)
		{
			let arrDstAudioEncodec = intArrayFromString(strDstAudioEncodec).concat(0);
			bufDstAudioEncodec = Module._malloc(arrDstAudioEncodec.length);
			Module.HEAPU8.set(arrDstAudioEncodec, bufDstAudioEncodec);
			DstAudioEncodecLen=arrDstAudioEncodec.length;
		}
		if(strDstAudioSampleRate!=null)
		{
			let arrDstAudioSampleRate = intArrayFromString(strDstAudioSampleRate).concat(0);
			bufDstAudioSampleRate = Module._malloc(arrDstAudioSampleRate.length);
			Module.HEAPU8.set(arrDstAudioSampleRate, bufDstAudioSampleRate);
			DstAudioSampleRateLen=arrDstAudioSampleRate.length;
		}
		Module._SetTransCodec(iVideoEnable,bufDstVideoEncodec,DstVideoEncodecLen,iAudioEnable,bufDstAudioEncodec,DstAudioEncodecLen,bufDstAudioSampleRate,DstAudioSampleRateLen);	
    }


    process(e) 
    {
		// 进行耗时操作  
		
		const message = e.data; // 接收传入的对象 
		// 根据命令处理  
		if (message.cmd === 'convert') //data: { Buffer:this.m_SrcBuffer,SrcType:this.m_SrcType,DstName:this.m_DstName } 
		{  
			const data = message.data;  
			this.m_SrcType = data.SrcType;
			this.m_DstName = data.DstName;//"OriginalData"
			this.m_SrcBuffer = data.Buffer;
			this.convert(this.m_SrcBuffer,this.m_SrcType,this.m_DstName);//OriginalData
		}
		else if (message.cmd === 'setWaterMark') //{cmd:'setWaterMark',data: { enable:enableMark,text:textMark} } 
		{  
			const data = message.data;  
			const enable = data.enable;
			const text = data.text;
			this.setWaterMark(enable,text);//
		} 
		else if (message.cmd === 'setTransCodec') //({cmd:'setTransCodec',data: { VideoEnable:iVideoEnable,DstVideoEncodec:strDstVideoEncodec,AudioEnable:iAudioEnable,DstAudioEncodec:strDstAudioEncodec,DstAudioSample:strDstAudioSample} });  
		{  
			const data = message.data;  
			this.setTransCodec(data.VideoEnable,data.DstVideoEncodec,data.AudioEnable,data.DstAudioEncodec,data.DstAudioSample);//
		} 
		else if (message.cmd === 'close')
		{
			self.postMessage({res:'close',data: message.data}); 
		}
		else
		{
			console.error('Invalid cmd '+message.cmd);
		}
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
			const dwSampleRate = convertedInfo[13];//

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
			if(null == this.m_DstEnc)
			{
				this.m_DstEnc = this.GetMediaDstEnc();
			}
			let vEncode = this.m_DstEnc[0];
			let aEncode = this.m_DstEnc[1];
			const obj = { data: convertedChunk,vEncode,aEncode,haveKeyFrame,startTime,durationTime,videoCnt,audioCnt,iEncType,iFrameType,dwFrameTimeStamp,width,height,dwSampleRate};

			// 反馈结果  
			self.postMessage({res:'convert',data: obj}); 
		} while (1);


        Module._free(bufDstType);
        Module._free(bufSrcType);
        Module._free(bufSrcFrame);
        Module._free(bufDstFrame);
        Module._free(bufDstInfo);
	} 

    destroy() 
    {
        
    }

    
}

// 实例化 
const oConvert = new convert();  //在 Worker 脚本中（convert.js），如果你想让其中的对象自动处理消息，需要在文件中 实例化。否则，onmessage 不会响应任何消息。
// 这样就绑定了事件  

 


//在外部文件（或通过导入）执行了任何实例化操作(创建对象)，才会绑定 onmessage(因为构造数中设置接收消息)
//export default convert; //默认导出 //这个js文件要作为worker使用，所以就不需要导出了，统一使用消息交互
//export {convert}; //命名导出