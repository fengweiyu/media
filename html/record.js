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
//import './common.js'; 
import { customLog, DownloadMedia } from './common.js';

var oRecord=null


class record 
{
    constructor() 
    {
		this.frameRate = 12;
		this.frameWidth = 240;
		this.frameHeight = 320;
		// 设置视频编码器参数
		this.options = {
			mimeType: 'video/mp4; codecs="avc1.42E01E, mp4a.40.2"',
			//videoBitsPerSecond : 2500000, // 设置视频比特率
			frameRate: { ideal: 25, max: 30 }, // 设置视频帧率
			keyFrameInterval: 50, // 设置关键帧间隔（单位为帧数）
		};
		this.mediaRecorder = null;
		this.mediaChunks = []; 
        oRecord=this;
    }

    GetMedia() 
    {
		if (window.orientation === 0 || window.orientation === 180) 
		{  
			console.log("竖屏");  
			frameWidth = 320;//手机端竖屏宽高和实际出流的相反
			frameHeight = 240;
		} 
		else if (window.orientation === 90 || window.orientation === -90) 
		{  
			console.log("横屏");  
		}
		navigator.mediaDevices.getUserMedia({ video: {
			width: oRecord.frameWidth,//frameHeight//手机端宽高和实际出流的相反
			height: oRecord.frameHeight,//frameWidth
			//frameRate: 30,
			frameRate: { ideal: oRecord.frameRate, max: oRecord.frameRate },//就算设置25，webrtc rtp时间戳间隔还是按照30的帧率
			facingMode: 'user',//前置摄像头facingMode: user ,'environment'表示要使用后置摄像头
			// 设置I帧间隔为60（大多数浏览器仅支持设置I帧间隔）
			video: {mandatory: {maxKeyFrameInterval: 60}}
			}, audio: true 
		}).then((stream)=>{//箭头函数不会改变 this 的上下文，因此可以直接使用对象的方法。继承外面
			this.mediaRecorder = new MediaRecorder(stream, this.options);
			// 监听MediaRecorder的数据可用事件，将录制的数据存储到recordedChunks数组
			this.mediaRecorder.ondataavailable = (event) => {
				console.log('ondataavailable ',event.data.size);
				if (event.data.size > 0) {
					this.mediaChunks.push(event.data); // 保存 Blob 数据  
				}
			};
			this.mediaRecorder.start(40);// 40ms间隔调用ondataavailable，实测8s
		}) // .bind(this)) 绑定上下文
		.catch(function(error) {
			console.log('getUserMedia audio video err',error);
		});
    }

    HandleMedia() 
    {
		if(null != this.mediaRecorder)
		{
			this.mediaRecorder.stop();//mediaRecorder.start(); stop才会调用ondataavailable
			const mediaBlob = new Blob(this.mediaChunks, { type: this.options });  
			// 将 Blob 转换为 Uint8Array  
			const reader = new FileReader();  
			reader.onload = function() 
			{  
				const arrayBuffer = reader.result;//这样才能读完
				console.log('onload ',arrayBuffer.byteLength);
				const typedArray = new Uint8Array(arrayBuffer);  
				DownloadMedia(typedArray,"user.mp4");//直接typedArray=new Uint8Array(mediaChunks);读取不到
			};  
			// 读取 Blob 内容  
			reader.readAsArrayBuffer(mediaBlob);
		}
    }
	HandleUserMedia()
	{//外部当静态方法使用
		oRecord.HandleMedia()
	} 
	GetUserMedia()
	{//外部当静态方法使用
		oRecord.GetMedia();
	}
    destroy() 
    {
        this.detachMediaElement();
        oRecord=null;
    }
}


export default record;
