/**
 * sigmoid函数
 * @param {} size 
 */
function sigmoid(size) {
    var e = 2.718281828459;
    var x = size / 1024 / 1024 / 1024;
    if (x < 1) {
        x = 0;
    } else {
        x = -0.1 * x;
    }
    var width = (1 / (1 + Math.exp(x))) * 160 - 60;
    return width;
}
/**
 * 进制转换函数
 * @param {*} num 
 */
function transform(num) {
    if (num < 1024) {
        return num + "B";
    } else if (num < 1024 * 1024) {
        str = (num / 1024).toFixed(2);
        return str + "KB";
    } else if (num < 1024 * 1024 * 1024) {
        str = (num / 1024 / 1024).toFixed(2);
        return str + "MB";
    } else if (num < 1024 * 1024 * 1024 * 1024) {
        str = (num / 1024 / 1024 / 1024).toFixed(2);
        return str + "GB";
    } else if (num < 1024 * 1024 * 1024 * 1024 * 1024) {
        str = (num / 1024 / 1024 / 1024 / 1024).toFixed(2);
        return str + "TB";
    }
}
/**
 * 获取URL中的参数
 */
function getUrlParam(name) {
    var reg = new RegExp("(^|&)" + name + "=([^&]*)(&|$)"); //构造一个含有目标参数的正则表达式对象
    var r = window.location.search.substr(1).match(reg);  //匹配目标参数
    if (r != null) return unescape(r[2]); 
    return null; //返回参数值
}

/**
 * 进度条显示工具
 * realProgress：实际进度
 * showProgress：显示进度(通过当前显示进度+固定增量[fixedSpeed]来显示)
 * example：<1>fixedSpeed:0.4 ;thresholdList:[50,90,99]
 *          <2>fixedSpeed:0.15 ;thresholdList:[25,50,75,99]
 */
function realProgressBarShowRate(realProgress, showProgress, fixedSpeed,thresholdList){
	if(realProgress==100){
		return realProgress
	}
    thresholdList.unshift(0);//在阈值表格的头部添加0防止越界操作
    var len=thresholdList.length;
    for(let i=1;i<len;i++){
        if(showProgress<thresholdList[i]){
            if(realProgress>=thresholdList[i]){
                return thresholdList[i];
            }else if(realProgress>=thresholdList[i-1] && realProgress<thresholdList[i]){
                return showProgress + fixedSpeed; 
            }else{
                return showProgress;
            }
        }
    }
    return realProgress;
}



 /**
  * 分页工具
  */
function pageTool(){
    this.totalPageNum=0;
    this.defaultTRNum=10;//每页默认数据条数
    this.minPage=1;
    this.currentButtomNum=0;
    this.currentPage=1;//当前展示的页面
    this.initial=(_totalPageNum) => {
        this.totalPageNum=_totalPageNum;
    }
    this.changePage=() => {

    }
    this.getCurrentPage=() => {

    }
    
}















