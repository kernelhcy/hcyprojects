package com.ruochi.component{
	import flash.filters.GlowFilter;
	import gs.TweenFilterLite;
	import com.ruochi.utils.graphic.setFilter;
	public class GlowCircleProgressBar extends CircleProgressBar {		
		private var _glowColor:uint = 0xff0000;		
		public function GlowCircleProgressBar(w:Number = 0, h:Number = 0) {
			super(w, h);
		}
		public function set glowColor(col:uint) {
			_glowColor = col;
			setFilter(circle, new GlowFilter(_glowColor, 1, 3, 3));
		}
	}
}