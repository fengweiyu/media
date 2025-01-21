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

function DownloadMedia(typedArray,downName)
{
	// 创建 Blob 对象  
	const blob = new Blob([typedArray]);  
	// 创建下载链接  
	const url = URL.createObjectURL(blob);  
	const a = document.createElement('a');  
	a.href = url;  
	a.download = downName; // 指定下载文件名  
	document.body.appendChild(a);  
	a.click(); // 自动点击下载链接  
	document.body.removeChild(a); // 下载后移除临时链接  
	// 释放 Blob URL  
	URL.revokeObjectURL(url); 
}

function formatDate(date) 
{  
	const currentDate = new Date();  
	const year = currentDate.getFullYear();  
	const month = String(currentDate.getMonth() + 1).padStart(2, '0'); // 月份从0开始  
	const day = String(currentDate.getDate()).padStart(2, '0');  
	const hours = String(currentDate.getHours()).padStart(2, '0');  
	const minutes = String(currentDate.getMinutes()).padStart(2, '0');  
	const seconds = String(currentDate.getSeconds()).padStart(2, '0');  
	const milliseconds = String(currentDate.getMilliseconds()).padStart(3, '0');

	const formattedDate = `${year}-${month}-${day} ${hours}:${minutes}:${seconds}.${milliseconds}`; 
	return formattedDate;  
} 
/*//自执行函数：我们使用一个自执行函数来封装代码，确保不会污染全局命名空间
(function() {  
	const originalLog = console.log;  

	console.log = function(...args) {  
		const now = new Date();  
		const timeStamp = formatDate(now);  //now.toISOString(); // 或者使用自定义格式  
		originalLog.apply(console, [`[${timeStamp}]`, ...args]);  
	};  
})(); */
const originalLog = console.log;  //必须先保存原始的，否则会发生递归调用
function customLog(...args)
{  
	const now = new Date();  
	const timeStamp = formatDate(now);  //now.toISOString(); // 或者使用自定义格式  
	originalLog.apply(console, [`[${timeStamp}]`, ...args]);  
} 
console.log = customLog;  // 替换默认的 console.log  
export { customLog,DownloadMedia };//命名导出   import { functionName } from '...' 必须导出，否则就算外部包含了所以，还是会报错未定义
//export default customLog; // 默认导出  import anyName from '...'

/*export { DownloadMedia }; 
// 封装 console.log  
function customLog(...args) {  
    console.log('[Custom Log]', ...args); // 添加自定义前缀  
}  
// 替换默认的 console.log  
console.log = customLog;  
// 导出自定义的 console.log (可选)  
export { customLog };  */