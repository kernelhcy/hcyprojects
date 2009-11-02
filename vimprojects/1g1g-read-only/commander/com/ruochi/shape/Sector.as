package com.ruochi.shape{
	import flash.display.Shape;
	import com.ruochi.utils.drawArc;
	public class Sector extends Shape {
		public function Sector(r1:Number=180,r2:Number=200,angle1:Number=0,angle2:Number=120,_color:uint=0xff6600) {			
			this.graphics.beginFill(_color);
			this.graphics.lineStyle(.1, 0, 1);
			this.graphics.moveTo(r1 * Math.cos(angle1 / 180 * Math.PI), -r1 * Math.sin(angle1 / 180 * Math.PI));
			drawArc(this, 0, 0, r1, angle2 - angle1, angle1);			
			this.graphics.lineTo(r2 * Math.cos(angle2 / 180 * Math.PI), -r2 * Math.sin(angle2 / 180 * Math.PI));
			drawArc(this, 0, 0, r2, angle1 - angle2, angle2);
			this.graphics.lineTo(r1 * Math.cos(angle1 / 180 * Math.PI), -r1 * Math.sin(angle1 / 180 * Math.PI));
			this.graphics.endFill();
			super();
		}
	}
}