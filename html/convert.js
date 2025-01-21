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
//import './build/MediaConvert.js'; 

Module.onRuntimeInitialized = function() 
{
	console.log('onRuntimeInitialized:'); 
}

var oConvert=null

class convert 
{
    constructor() 
    {
        this.m_MediaSource = new MediaSource();
        this.m_MediaElement = null;
		this.m_DstName = null;
		this.m_SrcName = null;
		this.m_SrcType = null;
		this.m_SrcBuffer = null;
        oConvert=this;
    }

    attachMediaElement(mediaElement) 
    {
        this.m_MediaElement = mediaElement;
    }

    detachMediaElement() 
    {
        if (this.m_MediaElement) 
        {
            this.m_MediaElement = null;
        }
    }

    process(SrcBuffer,SrcName,SrcType,DstName) 
    {
		this.m_SrcName = SrcName;
		this.m_SrcType = SrcType;
		this.m_DstName = DstName;
		this.m_SrcBuffer = SrcBuffer;
		if(null != this.m_MediaElement)
			this.m_MediaElement.src = URL.createObjectURL(this.m_MediaSource); 
		this.m_MediaSource.addEventListener('sourceopen', this.MediaSourceOpen);//静态方法
    }
	MediaSourceOpen() 
    {//静态方法
        oConvert.MediaSourceHandle();
    }
	MediaSourceHandle()
	{  
		var sourceBuffer=null;  
		const typedArray = this.convert(this.m_SrcBuffer,this.m_SrcType,this.m_DstName);
		if (typedArray) 
		{ // 成功创建，输出内容
			if(this.m_DstName == ".mp4")
			{
				if(null == sourceBuffer)
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
					if(null == VideoCodec || null ==AudioCodec)
					{
						console.log('GetMediaDstEnc err v '+VideoCodec+' a '+AudioCodec);    
					}
					else
					{
						sourceBuffer = this.m_MediaSource.addSourceBuffer('video/mp4; codecs="'+VideoCodec+','+AudioCodec+'"');
						localVideoElement.style.display = 'block'; // 显示视频 = 'none'; // 隐藏视频  
					}

				}
				if(null != sourceBuffer)
					sourceBuffer.appendBuffer(typedArray);    
			}   
			DownloadMedia(typedArray,this.m_SrcName+this.m_DstName);
		} 
		else
		{// 处理未成功创建的情况 
			console.log('FormatConvert err'); 
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
		let bufSrctype = Module._malloc(arrSrctype.length);
		Module.HEAPU8.set(arrSrctype, bufSrctype);
		const uint8View = new Uint8Array(srcBuffer);//arrayBuffer
		let bufSrcFrame = Module._malloc(uint8View.byteLength);
		Module.HEAPU8.set(uint8View,bufSrcFrame);//bufSrcFrame,arrayBuffer,要Uint8Array才可以设置进去拷贝，arrayBuffer不能拷贝
		//const lengthPtr = Module._malloc(arrayBuffer.byteLength);
		//const uint8View = new Uint8Array(Module.HEAPU8.buffer, bufSrcFrame, arrayBuffer.byteLength).set(arrayBuffer);
		Module._InputData(bufSrcFrame,uint8View.byteLength,bufSrctype,bufDstType);
		//const buffer = new ArrayBuffer(arrayBuffer.byteLength); 
		//const uint8View = new Uint8Array(buffer); 
		var bufDstMaxLen=uint8View.byteLength+(10*1024*1024);
		let bufDstFrame = Module._malloc(bufDstMaxLen);
		var ret=0;
		var len=0; 
		do {  
			len+=ret;   
			ret = Module._GetData(bufDstFrame+len,bufDstMaxLen-len);//arrayBuffer.buffer arrayBuffer.byteOffset
		} while (ret>0); 
		let copiedArray = new Uint8Array(len);
		copiedArray.set(new Uint8Array(Module.HEAPU8.buffer, bufDstFrame, len)); // 将C中的数组拷贝到JS数组中
		Module._free(bufDstType);
		Module._free(bufSrctype);
		Module._free(bufSrcFrame);
		Module._free(bufDstFrame);
		if(len>0)    
			return copiedArray;
		return null;            
	} 

    destroy() 
    {
        this.detachMediaElement();
        oConvert=null;
    }


    
}

export default convert;