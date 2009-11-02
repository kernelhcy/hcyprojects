package com.ruochi.test {
	import com.ruochi.shape.Circle;
	import flash.display.Sprite;
	import gs.*;
	import fl.motion.easing.*;
	public class Ease extends Sprite {	
		var aa:Circle = new Circle(10,0x00ff00)
		var bb:Circle = new Circle(10, 0xff0000)
		public function Ease() {
			bb.y = 30;
			addChild(aa);
			addChild(bb);
			TweenLite.to(aa, 3, { x:500, ease:Sine.easeOut } );
			TweenLite.to(bb, 3, { x:500, ease:Quadratic.easeOut } );
		}
	}
}