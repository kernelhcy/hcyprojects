package com.ezsong.shape{
	import flash.display.Shape;
	import com.ruochi.graphic.drawArc;
	public class NextShape extends Shape {
		private var _color:uint = 0xffffff;
		private var _conner:Number;
		private var _sideLength:Number;
		public function NextShape(sideLength:Number = 20, conner:Number = 2,c:uint=0xffffff) {
			_color = c;
			_sideLength = sideLength;
			_conner = conner;
			draw();
		}
		private function draw():void {
			var firstTriangleWidth:int = _sideLength * .9;
			
			var secondSideLength:int = _sideLength  *.9;
			var firstTriangleRightY:int = (1 - Math.sin(Math.PI / 6)) * _conner + (firstTriangleWidth - (1 + Math.cos(Math.PI/6)) * _conner) * Math.tan(Math.PI / 6);
			graphics.clear();
			graphics.beginFill(_color);
			graphics.moveTo(0, _conner);
			graphics.lineTo(0, _conner + _sideLength);
			drawArc(this, _conner, -_conner - _sideLength, _conner, 120, -180);
			//graphics.lineTo(firstTriangleWidth, _sideLength+_conner*2-firstTriangleWidth*Math.tan(Math.PI/6));
			graphics.lineTo(firstTriangleWidth, _sideLength + _conner*2 - firstTriangleRightY);
			graphics.lineTo(firstTriangleWidth, _sideLength/2 + secondSideLength/2 + _conner);
			drawArc(this, firstTriangleWidth + _conner, -_conner - secondSideLength/2 - _sideLength/2, _conner, 120, -180);
			graphics.lineTo(firstTriangleWidth + secondSideLength  * Math.cos(Math.PI / 6)+ (Math.sin(Math.PI / 6)+1) * _conner, _conner + _sideLength/2 + _conner * Math.cos(Math.PI / 6));
			drawArc(this, firstTriangleWidth + secondSideLength * Math.cos(Math.PI / 6) + _conner, - _conner - _sideLength/2 , _conner, 120, -60);
			graphics.lineTo(firstTriangleWidth +_conner + _conner * Math.sin(Math.PI / 6), _conner - _conner*Math.cos(Math.PI / 6) + _sideLength/2 - secondSideLength/2);
			drawArc(this, firstTriangleWidth+_conner, -_conner - _sideLength/2 + secondSideLength/2, _conner, 120, 60);
			graphics.lineTo(firstTriangleWidth, firstTriangleRightY );
			//graphics.lineTo(_conner + _conner * Math.sin(Math.PI / 6), _conner - _conner * Math.cos(Math.PI / 6));
			graphics.lineTo(_conner + _conner * Math.sin(Math.PI / 6), _conner - _conner * Math.cos(Math.PI / 6));
			drawArc(this, _conner, -_conner, _conner, 120, 60);
			var rectX:int = firstTriangleWidth + _sideLength  * Math.cos(Math.PI / 6) + _conner * 2;
			graphics.drawRoundRect(rectX, 0, int(_sideLength / 4), _sideLength + _conner*2, int(_sideLength / 4), int(_sideLength /4));
			graphics.endFill();
		}
		public function set color(c:uint):void {
			_color = c;
			draw()
		}
		public function get color():uint {
			return _color;
		}
		
		
	}
}