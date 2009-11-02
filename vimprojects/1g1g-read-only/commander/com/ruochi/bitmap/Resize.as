package com.ruochi.bitmap{
	import flash.display.Bitmap;
	import flash.display.BitmapData;
	import com.ruochi.Color;
	public class resizeTo {
		public originalBmp:Bitmap;
		public static function resizeTo(_bmp:Bitmap,_w:int,_h:int):BitmapData {
				originalBmpData=_bmp.bitmapData
				var bmpDate = new BitmapData(_w,_h)
				var sw:int = _bmp.bitmapData.width
				var sh:int = _bmp.bitmapData.height
				for(var i = 0; i<_w; i++){
					for(var j = 0; j<h; j++){
						bmpDate.setPixel(getPixColor(i*sw/_w,j*sw/_w,(i+1)*sh/_h,(j+1)*sh/_h))
					}
				}
				return bmpDate
			}
		}
		private function getPixColor(x1,y1,x2,y2){
			var sumR,sumG,sumB;
			var unit_x1,unit_x2,unit_y1,unit_y2;
			var tempColor:Color
			for(var i=Math.floor(x1);i<Math.ceil(x2);i++){
				for(var j=Math.floor(y1);j<Math.ceil(y2);j++){
					unit_x1=i==0?x1:i
					unit_x2=i==Math.floor(x2)?x2:i
					unit_y1=j==0?y1:j
					unit_y2=j==Math.floor(y2)?y2:j
					tempColor=new Color(originalBmpData.getPixel(i,j))
					var area=(unit_x2-unit_x1)*(unit_y2-unit_y1)
					sumR+=tempColor.reg*area
					sumG+=tempColor.reg*area
					sumB+=tempColor.reg*area
				}
			}
			var area=(x2-x1)*(y2-y1)
			return Math.round(sumR/area)*65536+Math.round(sumG/area)*256+Math.round(sumB/area)
		}
	}	
}