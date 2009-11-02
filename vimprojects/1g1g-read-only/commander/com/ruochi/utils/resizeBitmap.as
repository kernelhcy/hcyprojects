package com.ruochi.utils{
	import flash.display.Bitmap;
	import flash.display.BitmapData;
	import flash.utils.ByteArray;
	public function resizeBitmap(_bmp:BitmapData,_w:int,_h:int):BitmapData {
		var i:int;
		var tempId:int;
		var returnBitmapData:BitmapData = new BitmapData(_w, _h);
		var tempAlpha:uint
		var tempRed:uint
		var tempGreen:uint
		var tempBlue:uint
		var tempA:uint
		var tempR:uint
		var tempG:uint
		var tempB:uint;
		var pixelByteArray:ByteArray = _bmp.getPixels(_bmp.rect);
		var xGapArray:Array = new Array();
		var yGapArray:Array = new Array();
		var tempByteArray:ByteArray = new ByteArray();
		var returnByteArray:ByteArray = new ByteArray();
		var lineByteArray:ByteArray = new ByteArray();
		var pixelNum:int = _bmp.width * _bmp.height;
		var xGap:Number;
		var yGap:Number;
		var temp:Number; 
		var ratio:Number;
		var divid:Number;
		xGap=_bmp.width/_w;
		yGap=_bmp.height/_h;
		var area:Number = xGap*yGap;
		for (i = 1; i<=_w; i++) {
			xGapArray.push(xGap*i);
		}
		for (i = 1; i<=_h; i++) {
			yGapArray.push(yGap*i);
		}
		temp=0;
		tempId=0;
		tempAlpha=0;
		tempRed=0;
		tempGreen=0;
		tempBlue=0;
		var tempi:int=0;
		var tracei:int =0;
		pixelByteArray.position=0;
		for (i=0; i<pixelNum; i++) {
			if (tempi+1==Math.floor(xGapArray[tempId])) {
				tempA=pixelByteArray.readUnsignedByte();
				tempR=pixelByteArray.readUnsignedByte();
				tempG=pixelByteArray.readUnsignedByte();
				tempB=pixelByteArray.readUnsignedByte();
				ratio=xGapArray[tempId]-tempi-1;
				tempByteArray.writeInt(tempAlpha+tempA*ratio);
				tempByteArray.writeInt(tempRed+tempR*ratio);
				tempByteArray.writeInt(tempGreen+tempG*ratio);
				tempByteArray.writeInt(tempBlue+tempB*ratio);
				tempId++;
				ratio=1-ratio;
				tempAlpha=tempA*ratio;
				tempRed=tempR*ratio;
				tempGreen=tempG*ratio;
				tempBlue=tempB*ratio;
			} else {
				tempAlpha+=pixelByteArray.readUnsignedByte();
				tempRed+=pixelByteArray.readUnsignedByte();
				tempGreen+=pixelByteArray.readUnsignedByte();
				tempBlue+=pixelByteArray.readUnsignedByte();
			}
			if (tempi+1==_bmp.width) {
				tempi=0;
				tempId=0;
			} else {
				tempi++;
			}
		}
		temp=0;
		tempByteArray.position=0;
		tempId=0;
		tempAlpha=0;
		tempRed=0;
		tempGreen=0;
		tempBlue=0;
		tempi=0;
		var tempLine:int=0;
		pixelNum = _w*_bmp.height;
		lineByteArray.length=_w*1024*8;
		returnByteArray.position=0;
		for (i=0; i<pixelNum; i++) {
			if (tempLine+1==Math.floor(yGapArray[tempId])) {
				tempA=tempByteArray.readUnsignedInt();
				tempR=tempByteArray.readUnsignedInt();
				tempG=tempByteArray.readUnsignedInt();
				tempB=tempByteArray.readUnsignedInt();
				ratio=yGapArray[tempId]-tempLine-1;
				returnByteArray.writeByte((tempA*ratio+lineByteArray.readUnsignedInt())/area);
				returnByteArray.writeByte((tempR*ratio+lineByteArray.readUnsignedInt())/area);
				returnByteArray.writeByte((tempG*ratio+lineByteArray.readUnsignedInt())/area);
				returnByteArray.writeByte((tempB*ratio+lineByteArray.readUnsignedInt())/area);
				if (tempi+1==_w) {
					tempId++;
				}
				lineByteArray.position-=16;
				ratio=1-ratio;
				lineByteArray.writeInt(tempA*ratio);
				lineByteArray.writeInt(tempR*ratio);
				lineByteArray.writeInt(tempG*ratio);
				lineByteArray.writeInt(tempB*ratio);
			} else {
				tempA=lineByteArray.readUnsignedInt();
				tempR=lineByteArray.readUnsignedInt();
				tempG=lineByteArray.readUnsignedInt();
				tempB=lineByteArray.readUnsignedInt();
				lineByteArray.position-=16;
				lineByteArray.writeInt(tempA+tempByteArray.readUnsignedInt());
				lineByteArray.writeInt(tempR+tempByteArray.readUnsignedInt());
				lineByteArray.writeInt(tempG+tempByteArray.readUnsignedInt());
				lineByteArray.writeInt(tempB+tempByteArray.readUnsignedInt());
			}
			if (tempi+1==_w) {
				lineByteArray.position=0;
				tempi=0;
				tempLine++;
			} else {
				tempi++;
			}
		}
		returnByteArray.position=0;
		returnBitmapData.setPixels(returnBitmapData.rect, returnByteArray);
		return returnBitmapData;
	}
}